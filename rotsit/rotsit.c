
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

#include "xvector/xvector.h"
#include "xstring/xstring.h"

#include "rotsit/rotsit.h"

#define RECORD_DELIM       ("f\b\n")
#define FIELD_DELIM        ("f\b")

struct rotsit_t {
   xvector_t *records;  // rotrec_t
};

struct rotrec_t {
   xvector_t *fields;   // char *
};

static void rotrec_del (rotrec_t *rec)
{
   if (!rec)
      return;

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

static xvector_t *split_inplace (char *input, const char *delim)
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
      if (!safe_xvadd (&ret, lhs)) {
         fprintf (stderr, "Out of memory\n");
         goto errorexit;
      }

      lhs = &rhs[dlen];
   }

   error = false;
errorexit:
   if (error) {
      xvector_free (ret);
      ret = NULL;
   }
   return ret;

}

static rotrec_t *rotrec_new (xvector_t *fields)
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

   rotsit_t *ret = malloc (sizeof *ret);
   if (!ret) {
      fprintf (stderr, "Out of memory\n");
      goto errorexit;
   }
   memset (ret, 0, sizeof *ret);

   xvector_t *records = split_inplace (input_buf, RECORD_DELIM);

   for (size_t i=0; i<XVECT_LENGTH (records); i++) {

      char *rec_str = XVECT_INDEX (records, i);

      xvector_t *fields = split_inplace (rec_str, FIELD_DELIM);
      if (!fields) {
         fprintf (stderr, "Failure parsing record [%zu]\n", i);
         goto errorexit;
      }

      rotrec_t *rec = rotrec_new (fields);
      if (!rec) {
         fprintf (stderr, "Out of memory failure\n");
         goto errorexit;
      }

      if (!safe_xvadd (&ret->records, rec)) {
         fprintf (stderr, "Failed to store record\n");
         goto errorexit;
      }
   }

   error = false;
errorexit:
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
