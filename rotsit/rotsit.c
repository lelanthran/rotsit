
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#include "xvector/xvector.h"
#include "xstring/xstring.h"
#include "xerror/xerror.h"
#include "xcrypto/xcrypto.h"

#include "rotsit/rotsit.h"
#include "pdate/pdate.h"
#include "eval/eval.h"
#include "pdate/pdate.h"

#define RECORD_DELIM       ("f\b\n")
#define FIELD_DELIM        ("f\b")

static void *exec_op (const void *p_op, void const *p_lhs, void const *p_rhs)
{
   const char *s_op = p_op;
   const char *s_lhs = p_lhs;
   const char *s_rhs = p_rhs;

   int result = -1;
   int64_t lhs = 0;
   int64_t rhs = 0;
   char tmp[40];

   bool parsed = false;

   enum pdate_errcode_t d_err_lhs, d_err_rhs;
   time_t tv_lhs, tv_rhs;

   // First try to parse this as a date. If it's a valid date we use it as
   // a large integer.
   d_err_lhs = pdate_parse (s_lhs, &tv_lhs, true);
   d_err_rhs = pdate_parse (s_rhs, &tv_rhs, true);

   XERROR (" [%s] (%s) [%s]\n", s_lhs, s_op, s_rhs);

   if (d_err_lhs==pdate_valid && d_err_rhs==pdate_valid) {
      // At this point the operands could still be numbers and not dates.
      bool is_date = false;
      for (size_t i=1; s_lhs[i]; i++) {
         if (!isxdigit (s_lhs[i])) {
            is_date = true;
            break;
         }
      }
      for (size_t i=1; !is_date && s_rhs[i]; i++) {
         if (!isxdigit (s_rhs[i])) {
            is_date = true;
            break;
         }
      }

      if (is_date) {
         lhs = (int32_t)tv_lhs;
         rhs = (int32_t)tv_rhs;
         XERROR ("Read dates [0x%" PRIx64 "], [0x%" PRIx64 "] \n",
               lhs, rhs);
         parsed = true;
      }
   }

   // Next, try to read lhs/rhs as a number. If both are numbers
   // then we assume that the integer operators apply
   if (!parsed) {
      int i_lhs = sscanf (s_lhs, "%" PRIx64, &lhs);
      int i_rhs = sscanf (s_rhs, "%" PRIx64, &rhs);

      if (i_lhs==1 && i_rhs==1) {
         parsed = true;
      }
   }

   // If none of the above parsings worked, then we treat the operands as
   // strings. Note that not all operators are defined for strings, only
   // the equality and non-equality.
   if (!parsed) {
      if (*s_op == '!') {
         sprintf (tmp, "%i", strstr (s_rhs, s_lhs)==NULL);
      }
      if (*s_op == '=') {
         sprintf (tmp, "%i", strstr (s_rhs, s_lhs)!=NULL);
      }
      sprintf (tmp, "%i", result);
      return xstr_dup (tmp);
   }

   switch (*s_op) {
      case '+':   result = lhs + rhs;  break;
      case '-':   result = lhs - rhs;  break;
      case '*':   result = lhs * rhs;  break;
      case '/':   result = lhs / rhs;  break;
      case '<':   result = lhs < rhs;  break;
      case '>':   result = lhs > rhs;  break;
      case '=':   result = lhs == rhs; break;
      case '!':   result = lhs != rhs; break;
      case '&':   result = lhs && rhs; break;
      case '|':   result = lhs || rhs; break;
   }

   sprintf (tmp, "%i", result);
   XERROR ("Returning [%s]\n", tmp);
   return xstr_dup (tmp);
}

static eval_type_t check_type (void const *token)
{
   const char *s_token = token;
   switch (*s_token) {
      case '*':
      case '/':   return eval_HIGH_OPS;
      case '=':
      case '!':
      case '+':
      case '-':
      case '<':
      case '>':
      case '&':
      case '|':   return eval_LOW_OPS;
      case '(':   return eval_OPEN;
      case ')':   return eval_CLOSE;
      default:    return eval_OPERAND;
   }
   return eval_UNKNOWN;
}

static char **make_tokens (const char *input)
{
   bool error = true;
   char **ret = NULL;
   xvector_t *xv = NULL;
   char *local = xstr_dup (input);
   if (!local) {
      goto errorexit;
   }

   char *start = local;
   while (*start) {
      char *new_token = NULL;
      char *end;
      char tmp_c;
      bool dbl = false;
      static const char *ops = "()+-/*&|<>=!";

      if (start[1]=='=')
         dbl = true;

      switch (*start) {
         case '(':   new_token = xstr_dup ("("); break;
         case ')':   new_token = xstr_dup (")"); break;
         case '+':   new_token = xstr_dup ("+"); break;
         case '-':   new_token = xstr_dup ("-"); break;
         case '/':   new_token = xstr_dup ("/"); break;
         case '*':   new_token = xstr_dup ("*"); break;
         case '&':   new_token = xstr_dup ("&"); break;
         case '|':   new_token = xstr_dup ("|"); break;

         case '<':   new_token = xstr_dup (dbl ? "<=" : "<");  break;
         case '>':   new_token = xstr_dup (dbl ? ">=" : ">");  break;
         case '=':   new_token = xstr_dup (dbl ? "==" : NULL); break;
         case '!':   new_token = xstr_dup (dbl ? "!=" : NULL); break;

         case ' ':
         case '\n':
         case '\r':
         case '\t':  dbl = false;
                     break;

         default: end = start;
                  while (strchr (ops, *end)==NULL)
                     end++;
                  tmp_c = *end;
                  *end = 0;
                  new_token = xstr_dup (start);
                  *end = tmp_c;
                  end--;
                  start = end;
                  break;
      }

      if (new_token) {
         xvector_t *tmp = xvector_ins_tail (xv, new_token);
         if (!tmp) {
            free (new_token);
            goto errorexit;
         }
         xv = tmp;
      }

      if (dbl)
         start++;

      if (*start)
         start++;
   }

   ret = xvector_native (xv);
   error = false;

errorexit:

   free (local);
   xvector_free (xv);

   if (error) {
      xstr_delarray (ret);
      ret = NULL;
   }
   return ret;
}

struct rotsit_t {
   char *buffer;
   xvector_t *records;  // rotrec_t
};

struct rotrec_t {
   xvector_t *fields;   // char *
};

static bool fsubst (char **tokens, rotrec_t *rr)
{
   bool error = true;

   static const struct {
      uint32_t       fnum;
      const char    *name;
   } fields[] = {
      { RF_GUID,           "guid"        },
      { RF_ORDER,          "order"       },
      { RF_OPENED_BY,      "opened_by"   },
      { RF_OPENED_ON,      "opened_on"   },
      { RF_OPENED_MSG,     "message"     },
      { RF_STATUS,         "status"      },
      { RF_ASSIGNED_BY,    "assigned_by" },
      { RF_ASSIGNED_TO,    "assigned_to" },
      { RF_ASSIGNED_ON,    "assigned_on" },
      { RF_CLOSED_BY,      "closed_by"   },
      { RF_CLOSED_ON,      "closed_on"   },
      { RF_CLOSED_MSG,     "closed_msg"  },
      { RF_DUP_BY,         "dup_by"      },
      // TODO: Must also match up comments
      // RF_DUP_GUID
      // RF_DUP_MSG
   };

   for (size_t i=0; tokens && tokens[i]; i++) {

      xstr_trim (tokens[i]);

      for (size_t j=0; j<sizeof fields/sizeof fields[0]; j++) {

         if (strcmp (fields[j].name, tokens[i])==0) {
            char *field = XVECT_INDEX (rr->fields, j);

            free (tokens[i]);
            tokens[i] = xstr_dup (field);
            if (!tokens[i] && field) {
               XERROR ("Out of memory\n");
               goto errorexit;
            }
            break;
         }
      }
   }

   error = false;
errorexit:
   return !error;
}

void rotrec_del (rotrec_t *rec)
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

         char *tmpf = field ? field : "";
         fprintf (outf, "%s%s", tmpf, FIELD_DELIM);
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

void lower_string (char *src)
{
   for (size_t i=0; src[i]; i++) {
      src[i] = tolower (src[i]);
   }
}

static bool istrcmp (const char *lhs, const char *rhs)
{
   if (!lhs && rhs)
      return false;

   if (lhs && !rhs)
      return false;

   while (*lhs && *rhs && (tolower (*lhs++) == tolower (*rhs++)))
      ;

   return *lhs == *rhs;
}

rotrec_t **rotsit_filter (rotsit_t *rs, const char *expr)
{
   bool error = true;
   xvector_t *results = NULL;
   rotrec_t **ret = NULL;
   char **ltokens = NULL;

   eval_t *ev = eval_new ( (void *(*) (const void *))xstr_dup,
                           (void (*) (void *))free,
                           exec_op, check_type);

   char **tokens = make_tokens (expr);
   uint32_t num_records = 0;

   if (!rs) {
      XERROR ("Passed a NULL rotsit database.\n");
      goto errorexit;
   }

   if (!expr || !*expr) {
      XERROR ("Expression is empty.\n");
      goto errorexit;
   }

   num_records = rotsit_count_records (rs);
   if (!num_records) {
      XERROR ("Database is empty, cowardly refusing to search it.\n");
      goto errorexit;
   }

   if (!ev) {
      XERROR ("Failed to create expression execution context.\n");
      goto errorexit;
   }

   if (!tokens) {
      XERROR ("Failed to tokenise input expression.\n");
      goto errorexit;
   }

   for (size_t i=0; i<num_records; i++) {
      int iresult = -1;
      rotrec_t *rr = rotsit_get_record (rs, i);

      ltokens = xstr_cpyarray (tokens);
      if (!fsubst (ltokens, rr)) {
         XERROR ("Error during variable substitution.\n");
         goto errorexit;
      }

      char *sresult = eval_execute (ev, (const void **)ltokens);
      if (!sresult) {
         XERROR ("Internal error during expression evaluation.\n");
         for (size_t i=0; ltokens[i]; i++) {
            XERROR ("Token %i: [%s]\n", i, ltokens[i]);
         }

         goto errorexit;
      }
      int nparams = sscanf (sresult, "%i", &iresult);
      XERROR ("RESULT: [%s]\n", sresult);
      free (sresult);

      if (iresult==1) {
         xvector_t *tmp = xvector_ins_tail (results, rr);
         if (!tmp) {
            XERROR ("Out of memory error.\n");
            goto errorexit;
         }
         results = tmp;
         XERROR ("Matched\n");
      }

      xstr_delarray (ltokens); ltokens = NULL;
   }

   error = false;

errorexit:
   if (error) {
      for (size_t i=0; ret && *ret; i++) {
         rotrec_del (ret[i]);
      }
      free (ret);
      ret = NULL;
   }

   eval_del (ev);
   xstr_delarray (tokens);
   xstr_delarray (ltokens);

   ret = xvector_native (results);

   if (!ret[0]) {
      XERROR ("Warning: filter [%s] matched no records\n", expr);
   }
   xvector_free (results);
   return ret;
}

rotrec_t *rotsit_find_by_id (rotsit_t *rs, const char *id)
{
   if (!rs || !id)
      return NULL;

   for (size_t i=0; i<XVECT_LENGTH (rs->records); i++) {

      rotrec_t *rec = XVECT_INDEX (rs->records, i);

      if (strcmp (XVECT_INDEX (rec->fields, RF_GUID), id)==0)
         return rec;
   }

   return NULL;
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
   XVECT_INDEX (ret->fields, RF_GUID) = str_guid;

   // Third field/2 must be set to the current user
   char *str_user = make_username ();
   if (!str_user) {
      goto errorexit;
   }
   XVECT_INDEX (ret->fields, RF_OPENED_BY) = str_user;

   // Fourth field/3 must be set to the current time
   char *str_time = make_time (0);
   if (!str_time) {
      goto errorexit;
   }
   XVECT_INDEX (ret->fields, RF_OPENED_ON) = str_time;

   // Fifth field/4 must be set to the message
   char *str_msg = xstr_dup (msg);
   if (!str_msg) {
      XERROR ("Out of memory\n");
      goto errorexit;
   }
   XVECT_INDEX (ret->fields, RF_OPENED_MSG) = str_msg;

   // Set the status to OPEN
   char *str_status = xstr_dup ("OPEN");
   if (!str_status) {
      XERROR ("Out of memory\n");
      goto errorexit;
   }
   XVECT_INDEX (ret->fields, RF_STATUS) = str_status;

   error = false;
errorexit:
   if (error) {
      rotrec_del (ret);
      ret = NULL;
   }
   return ret;
}

bool rotrec_add_comment (rotrec_t *rr, const char *comment)
{
   bool error = true;
   char *new_fields[4] = { NULL, NULL, NULL, NULL };
   xvector_t *newxv = NULL;

   if (!rr || !comment)
      return false;

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

bool rotrec_close (rotrec_t *rr, const char *message)
{
   if (!rr || !message)
      return false;

   char *str_status = xstr_dup ("CLOSED");
   char *str_user = make_username ();
   char *str_time = make_time (0);
   char *str_message = xstr_dup (message);

   if (!str_status || !str_user || !str_time || !str_message) {
      free (str_status);
      free (str_user);
      free (str_time);
      free (str_message);
      return false;
   }

   free (XVECT_INDEX (rr->fields, RF_STATUS));
   free (XVECT_INDEX (rr->fields, RF_CLOSED_BY));
   free (XVECT_INDEX (rr->fields, RF_CLOSED_ON));
   free (XVECT_INDEX (rr->fields, RF_CLOSED_MSG));

   XVECT_INDEX (rr->fields, RF_STATUS) = str_status;
   XVECT_INDEX (rr->fields, RF_CLOSED_BY) = str_user;
   XVECT_INDEX (rr->fields, RF_CLOSED_ON) = str_time;
   XVECT_INDEX (rr->fields, RF_CLOSED_MSG) = str_message;

   return true;
}

bool rotrec_dup (rotrec_t *rr, const char *id)
{
   if (!rr || !id)
      return false;

   char *str_message = xstr_cat ("Closed as DUPLICATE of #", id, NULL);
   if (!str_message) {
      XERROR ("Out of memory\n");
      return false;
   }

   bool retval = rotrec_close (rr, str_message);

   free (str_message);

   return retval;
}

bool rotrec_reopen (rotrec_t *rr, const char *message)
{
   if (!rr || !message)
      return false;

   char *str_status = xstr_dup ("OPEN+");
   char *str_user = make_username ();
   char *str_time = make_time (0);
   char *str_message = xstr_cat ("REOPEN due to: ", message, NULL);

   if (!str_status || !str_user || !str_time || !str_message) {
      XERROR ("Out of memory\n");
      free (str_status);
      free (str_user);
      free (str_time);
      free (str_message);
      return false;
   }

   free (XVECT_INDEX (rr->fields, RF_STATUS));
   free (XVECT_INDEX (rr->fields, RF_OPENED_BY));
   free (XVECT_INDEX (rr->fields, RF_OPENED_ON));
   free (XVECT_INDEX (rr->fields, RF_OPENED_MSG));

   XVECT_INDEX (rr->fields, RF_STATUS) = str_status;
   XVECT_INDEX (rr->fields, RF_OPENED_BY) = str_user;
   XVECT_INDEX (rr->fields, RF_OPENED_ON) = str_time;
   XVECT_INDEX (rr->fields, RF_OPENED_MSG) = str_message;

   return true;
}

bool rotrec_dump (rotrec_t *rr, FILE *outf)
{
   if (!outf)
      outf = stdout;

   if (!rr) {
      fprintf (outf, "Cannot print a NULL rotrec_t data bject\n");
      return false;
   }

   fprintf (outf, "------------------------------------------------\n");
   fprintf (outf, "[id: %s] [order: %s] [status: %s]\nOpened by [%s] on [%s]\n",
                  (char *)XVECT_INDEX (rr->fields, RF_GUID),
                  (char *)XVECT_INDEX (rr->fields, RF_ORDER),
                  (char *)XVECT_INDEX (rr->fields, RF_STATUS),
                  (char *)XVECT_INDEX (rr->fields, RF_OPENED_BY),
                  (char *)XVECT_INDEX (rr->fields, RF_OPENED_ON));
   // TODO: Use the assigned/assigned_by/assigned_to fields.
   char *closed = XVECT_INDEX (rr->fields, RF_CLOSED_BY);
   if (closed && *closed) {
      fprintf (outf, "Closed by [%s] on [%s] with message: [%s]\n",
                     (char *)XVECT_INDEX (rr->fields, RF_CLOSED_BY),
                     (char *)XVECT_INDEX (rr->fields, RF_CLOSED_ON),
                     (char *)XVECT_INDEX (rr->fields, RF_CLOSED_MSG));
   }
   char *duped = XVECT_INDEX (rr->fields, RF_DUP_BY);
   if (duped && *duped) {
      fprintf (outf, "Marked DUPLICATE of [%s] by [%s] with message: [%s]\n",
                     (char *)XVECT_INDEX (rr->fields, RF_DUP_GUID),
                     (char *)XVECT_INDEX (rr->fields, RF_DUP_BY),
                     (char *)XVECT_INDEX (rr->fields, RF_DUP_MSG));
   }
   fprintf (outf, "** %s **\n",
                  (char *)XVECT_INDEX (rr->fields, RF_OPENED_MSG));

   fprintf (outf, "----- COMMENTS -----\n");

   size_t comment_num = RF_LAST_FIELD;
   while ((comment_num + 4) <= XVECT_LENGTH (rr->fields)) {
      char *c_guid    = XVECT_INDEX (rr->fields, comment_num++);
      char *c_user    = XVECT_INDEX (rr->fields, comment_num++);
      char *c_time    = XVECT_INDEX (rr->fields, comment_num++);
      char *c_comment = XVECT_INDEX (rr->fields, comment_num++);
      fprintf (outf, "++ [comment: %s] by [%s] on [%s]\n%s\n",
               c_guid, c_user, c_time, c_comment);
   }

   return true;
}

