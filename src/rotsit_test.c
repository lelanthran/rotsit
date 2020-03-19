
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "rotsit/rotsit.h"

#include "xstring/xstring.h"


static bool test_parser (void)
{
   size_t num_errors = 0;
   rotsit_t *rs = NULL;
   char *ps[] = {
   "Empty string",                      "",
   "Single record",                     "onef\btwof\bthreef\bf\b\n",
   "Multiple records",                  "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n",
   "Malformed record",                  "onef\btwof\bthreef\b"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n",
   "Malformed field",                   "onef\btwof\threeff\b\n"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n",
   "Record missing final field-delim",  "onef\btwof\bthreef\b\n"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n",
   "Record missing final rec-delim",    "onef\btwof\bthree"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n"
                                        "onef\btwof\bthreef\bf\b\n",
   "NULL input",                        NULL,
   };

   for (size_t i=0; i<sizeof ps/sizeof ps[0]; i += 2) {

      char *tmp = xstr_dup (ps[i+1]);
      if (ps[i+1] && !tmp) {
         fprintf (stderr, "Out of memory\n");
         num_errors++;
         goto errorexit;
      }

      rs = rotsit_parse (tmp);

      printf ("-----------------------------\n");
      rotsit_dump (rs, ps[i], stdout);

      if (!rs) {
         num_errors++;
      }

      rotsit_del (rs);
      rs = NULL;
      free (tmp);
   }

errorexit:
   rotsit_del (rs);
   return num_errors ? false : true;
}

static bool test_writer (void)
{
   bool error = true;

   rotrec_t *rr = NULL;
   FILE *outf = NULL;

   rotsit_t *rs = rotsit_parse ("");
   if (!rs) {
      fprintf (stderr, "Object creation failed\n");
      goto errorexit;
   }

   rr = rotrec_new ("New issue #1\nWe opened a test issue\n");
   if (!rr) {
      fprintf (stderr, "Out of memory\n");
      goto errorexit;
   }
   if (!rotrec_add_comment (rr, "A comment\nadded to issue #1")) {
      fprintf (stderr, "Failed to add comment\n");
      goto errorexit;
   }

   rotsit_add_record (rs, rr);

   rr = rotrec_new ("Issue #2\nAnother test issue\n");
   if (!rr) {
      fprintf (stderr, "Out of memory\n");
      goto errorexit;
   }
   rotsit_add_record (rs, rr);
   if (!rotrec_add_comment (rr, "A comment\nadded to issue #2")) {
      fprintf (stderr, "Failed to add comment\n");
      goto errorexit;
   }

   rr = rotrec_new ("Issue #3\nAnother test issue\n");
   if (!rr) {
      fprintf (stderr, "Out of memory\n");
      goto errorexit;
   }
   if (!rotrec_add_comment (rr, "A comment\nadded to issue #3")) {
      fprintf (stderr, "Failed to add comment\n");
      goto errorexit;
   }

   rotsit_add_record (rs, rr);

   outf = fopen ("rotsit.sitdb", "wb");
   if (!outf) {
     fprintf (stderr, "Unable to open [%s] for writing: %m\n", "rotsit.sitdb");
     goto errorexit;
   }

   if (!rotsit_write (rs, outf)) {
      fprintf (stderr, "Unable to write to outfile\n");
      goto errorexit;
   }
   
   error = false;

errorexit:
   if (outf)
      fclose (outf);

   rotsit_del (rs);
   return !error;
}

int main (void)
{
   size_t num_failures = 0;

   struct {
      char *name;
      bool (*fptr) (void);
   } tests [] = {

#define TESTFUNC(x)      { #x, x }

      TESTFUNC (test_parser),
      TESTFUNC (test_writer),

#undef TESTFUNC

   };

   for (size_t i=0; i<sizeof tests/sizeof tests[0]; i++) {
      bool r = tests[i].fptr ();
      printf ("XXX %25s: %s\n", tests[i].name, r ? "passed" : "failed");

      if (!r)
         num_failures++;
   }

   printf ("XXX %25s: %zu\n", "Failures", num_failures);

   return num_failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
