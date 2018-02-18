
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "xvector/xvector.h"
#include "xstring/xstring.h"
#include "xerror/xerror.h"
#include "xcrypto/xcrypto.h"

#include "rotsit/rotsit.h"

#ifdef PLATFORM_WINDOWS
#define ENV_USERNAME       ("USERNAME")
#else
#define ENV_USERNAME       ("USER")
#endif

#define RECORD_DELIM       ("f\b\n")
#define FIELD_DELIM        ("f\b")

struct rotsit_t {
   char *buffer;
   xvector_t *records;  // rotrec_t
};

struct rotrec_t {
   xvector_t *fields;   // char *
};

static void rotrec_del (rotrec_t *rec)
{
   if (!rec)
      return;

   xvector_iterate (rec->fields, free);
   xvector_free (rec->fields);
   free (rec);
}

static bool safe_xvadd (xvector_t **xv, void *elm)
{
   xvector_t *tmp = xvector_ins_tail ((*xv), elm);
   if (!tmp) {
      return false;
   }

   (*xv) = tmp;
   return true;
}

static xvector_t *split_alloc (char *input, const char *delim)
{
   bool error = true;
   xvector_t *ret = NULL;
   size_t len = input ? strlen (input) : 0;
   size_t dlen = strlen (delim);
   char *eofs = &input[len];

   char *lhs = input;
   char *rhs = input;
   while (lhs && rhs < eofs) {

      char *rhs = strstr (lhs, delim);
      if (!rhs) {
         break;
      }

      *rhs = 0;

      char *tmp = xstr_dup (lhs);
      if (!tmp) {
         XERROR ("Out of memory\n");
         goto errorexit;
      }

      if (!safe_xvadd (&ret, tmp)) {
         XERROR ("Out of memory\n");
         free (tmp);
         goto errorexit;
      }

      lhs = &rhs[dlen];
   }

   error = false;
errorexit:
   if (error) {
      xvector_iterate (ret, free);
      xvector_free (ret);
      ret = NULL;
   }
   return ret;

}

static rotrec_t *new_rotrec (xvector_t *fields)
{
   rotrec_t *ret = malloc (sizeof *ret);
   if (!ret) {
      return NULL;
   }
   ret->fields = fields;
   return ret;
}

rotsit_t *rotsit_parse (char *input_buf)
{
   bool error = true;
   xvector_t *records = NULL;


   rotsit_t *ret = malloc (sizeof *ret);
   if (!ret) {
      XERROR ("Out of memory\n");
      goto errorexit;
   }
   memset (ret, 0, sizeof *ret);

   ret->buffer = xstr_dup (input_buf);
   if (input_buf && !ret->buffer) {
      XERROR ("Out of memory\n");
      goto errorexit;
   }

   records = split_alloc (ret->buffer, RECORD_DELIM);

   for (size_t i=0; i<XVECT_LENGTH (records); i++) {

      char *rec_str = XVECT_INDEX (records, i);

      xvector_t *fields = split_alloc (rec_str, FIELD_DELIM);
      if (!fields) {
         XERROR ("Failure parsing record [%zu]\n", i);
         goto errorexit;
      }

      rotrec_t *rec = new_rotrec (fields);
      if (!rec) {
         XERROR ("Out of memory failure\n");
         goto errorexit;
      }

      if (!safe_xvadd (&ret->records, rec)) {
         XERROR ("Failed to store record\n");
         goto errorexit;
      }
   }

   error = false;
errorexit:
   xvector_iterate (records, free);
   xvector_free (records);
   if (error) {
      rotsit_del (ret);
      ret = NULL;
   }
   return ret;
}

void rotsit_del (rotsit_t *rs)
{
   if (!rs)
      return;

   xvector_iterate (rs->records, (void (*) (void *))rotrec_del);
   xvector_free (rs->records);
   free (rs->buffer);
   free (rs);
}

void rotsit_dump (rotsit_t *rs, const char *id, FILE *outf)
{
   if (!outf)
      outf = stdout;

   if (!rs) {
      fprintf (outf, "Passed a NULL rotsit_t data object\n");
      return;
   }

   fprintf (outf, "Data [%s] has [%zu] records\n", id,
                                                  XVECT_LENGTH (rs->records));

   for (size_t i=0; i<XVECT_LENGTH (rs->records); i++) {

      rotrec_t *rr = XVECT_INDEX (rs->records, i);
      fprintf (outf, "[(%s):%zu] ", id, i);

      for (size_t j=0; j<XVECT_LENGTH (rr->fields); j++) {
         fprintf (outf, "(%s)", (char *)XVECT_INDEX (rr->fields, j));
      }
      fprintf (outf, "\n");
   }
}

bool rotsit_write (rotsit_t *rs, FILE *outf)
{
   if (!rs || !outf)
      return false;

   uint32_t order_max = 0;
   for (size_t i=0; i<XVECT_LENGTH (rs->records); i++) {
      rotrec_t *rec = XVECT_INDEX (rs->records, i);
      for (size_t j=0; j<XVECT_LENGTH (rec->fields); j++) {
         char *field = XVECT_INDEX (rec->fields, j);
         if (field && j==RF_ORDER) {
            uint32_t order;
            if (sscanf (field, "%x", &order)!=1) {
               XERROR ("Error: [%s] is not a number]\n", field);
               return false;
            }
            if (order > order_max)
               order_max = order;
         }
         bool free_field = false;
         if (!field && j==RF_ORDER) {
            field = malloc (2 + 9 + 1);
            if (!field) {
               XERROR ("Out of memory\n");
               return false;
            }
            free_field = true;
            sprintf (field, "0x%x", order_max++);
         }
         fprintf (outf, "%s%s", field, FIELD_DELIM);
         if (free_field) {
            free (field);
         }
      }
      fprintf (outf, "%s", RECORD_DELIM);
   }
   return true;
}

uint32_t rotsit_count_records (rotsit_t *rs)
{
   if (!rs)
      return 0;
   return XVECT_LENGTH (rs->records);
}

rotrec_t *rotsit_get_record (rotsit_t *rs, uint32_t recnum)
{
   if (!rs || recnum >= rotsit_count_records (rs)) {
      return NULL;
   }

   return XVECT_INDEX (rs->records, recnum);
}

bool rotsit_add_record (rotsit_t *rs, rotrec_t *rr)
{
   if (!rs || !rr)
      return false;

   if (!safe_xvadd (&rs->records, rr)) {
      XERROR ("Failed to store record\n");
      return false;
   }

   return true;
}

static char *make_guid (void)
{
   uint64_t guid;

   char *str_guid = malloc (2 + 16 + 1); // 0x + 16 digits + 0

   if (!str_guid) {
      XERROR ("Out of memory\n");
      return NULL;
   }

   xcrypto_random ((uint8_t *)&guid, sizeof guid);

   sprintf (str_guid, "0x%" PRIx64, guid);
   return str_guid;
}

static char *make_username (void)
{
   char *str_user = NULL;
   char *tmp_user = getenv (ENV_USERNAME);

   str_user = xstr_dup (tmp_user ? tmp_user : "Unknown");

   if (!str_user) {
      XERROR ("Out of memory\n");
      return NULL;
   }

   return str_user;
}

// TODO: Make this function thread-safe. Might need a mutex because
// localtime_r is not available on MingW32.
static char *make_time (uint32_t time_s)
{
   char *str_time = NULL;
   if (!time_s) {
      time_s = time (NULL);
   }

   str_time = xstr_dup (asctime (localtime ((time_t *)&time_s)));
   if (!str_time) {
      XERROR ("Out of memory\n");
   }

   char *tmp = strchr (str_time, '\n');
   if (tmp)
      *tmp = 0;

   return str_time;
}

rotrec_t *rotrec_new (const char *msg)
{
   bool error = true;
   static char *fields[] = {
      NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL,
      NULL, NULL,
   };

   rotrec_t *ret = malloc (sizeof *ret);

   if (!ret) {
      XERROR ("Out of memory\n");
      goto errorexit;
   }
   ret->fields = NULL;

   for (size_t i=0; i<sizeof fields/sizeof fields[0]; i++) {
      xvector_t *tmp = xvector_ins_tail (ret->fields, fields[i]);
      if (!tmp) {
         XERROR ("Out of memory\n");
         goto errorexit;
      }
      ret->fields = tmp;
   }

   // First field/0 must get a GUID. For now simply using a random number -
   // the extra search space by not limiting certain bits to dates
   // decreases the probability of a collision.
   char *str_guid = make_guid ();
   if (!str_guid) {
      goto errorexit;
   }
   XVECT_INDEX (ret->fields, 0) = str_guid;

   // Third field/2 must be set to the current user
   char *str_user = make_username ();
   if (!str_user) {
      goto errorexit;
   }
   XVECT_INDEX (ret->fields, 2) = str_user;

   // Fourth field/3 must be set to the current time
   char *str_time = make_time (0);
   if (!str_time) {
      goto errorexit;
   }
   XVECT_INDEX (ret->fields, 3) = str_time;

   // Fifth field/4 must be set to the message
   char *str_msg = xstr_dup (msg);
   if (!str_msg) {
      XERROR ("Out of memory\n");
      goto errorexit;
   }
   XVECT_INDEX (ret->fields, 4) = str_msg;


   error = false;
errorexit:
   if (error) {
      rotrec_del (ret);
      ret = NULL;
   }
   return ret;
}

bool rotrec_set_field (rotrec_t *rr, uint8_t fieldnum, const char *src)
{
   char *tmp = NULL;

   if (!rr)
      return false;

   if (fieldnum > RF_LAST_FIELD)
      return false;

   if (fieldnum <= RF_ORDER)
      return false;

   tmp = xstr_dup (src);
   if (!tmp) {
      XERROR ("Out of memory\n");
      return false;
   }

   free (XVECT_INDEX (rr->fields, fieldnum));
   XVECT_INDEX (rr->fields, fieldnum) = tmp;

   return true;
}

bool rotrec_add_comment (rotrec_t *rr, const char *comment)
{
   bool error = true;
   char *new_fields[4] = { NULL, NULL, NULL, NULL };
   xvector_t *newxv = NULL;

   new_fields[0] = make_guid ();
   new_fields[1] = make_username ();
   new_fields[2] = make_time (0);
   new_fields[3] = xstr_dup (comment);

   for (size_t i=0; i<sizeof new_fields/sizeof new_fields[0]; i++) {
      if (!new_fields[i]) {
         XERROR ("Out of memory\n");
         goto errorexit;
      }
   }

   for (size_t i=0; i<sizeof new_fields/sizeof new_fields[0]; i++) {
      xvector_t *tmp = xvector_ins_tail (newxv, new_fields[i]);
      if (!tmp) {
         XERROR ("Out of memory\n");
         goto errorexit;
      }
      newxv = tmp;
   }

   xvector_t *tmp = xvector_join (rr->fields, newxv);
   if (!tmp) {
      XERROR ("Out of memory\n");
      goto errorexit;
   }

   xvector_t *swap_tmp = rr->fields;
   rr->fields = tmp;
   xvector_free (swap_tmp);
   xvector_free (newxv);

   error = false;

errorexit:
   if (error) {
      for (size_t i=0; i<sizeof new_fields/sizeof new_fields[0]; i++) {
         free (new_fields [i]);
      }
   }

   return !error;
}

