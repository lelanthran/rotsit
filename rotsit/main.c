
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

int main (void)
{
   size_t num_failures = 0;

   struct {
      char *name;
      bool (*fptr) (void);
   } tests [] = {

#define TESTFUNC(x)      { #x, x }

      TESTFUNC (test_parser),

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
