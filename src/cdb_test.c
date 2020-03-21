
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cdb.h"

#define TEST_FILE       ("testfile.sitdb")


static bool write_new_file (void)
{
   bool error = true;
   char **records = NULL;
   char *record = NULL;

   FILE *outf = NULL;

   static const struct {
      const char *name;
      const char *value;
   } fields[] = {
      { "ONE",    "First field"  },
      { "TWO",    "Second field" },
      { "THIRD",  "Third field"  },
      { "FOURTH", "Fourth field" },
      { "FIFTH",  "Fifth field"  },
      { "SIXTH",  "Sixth field"  },
   };
   static const char *test_records[] = {
      "record one",
      "record two",
      "record three",
      "record four",
      "record five",
      "record six",
      "record seven",
      "record eight",
      "record nine",
      "record ten",
   };

   for (size_t i=0; i<sizeof fields/sizeof fields[0];i ++) {
      if (!(cdb_field_add (&record, fields[i].name, fields[i].value))) {
         fprintf (stderr, "Failed to add field to record\n");
         goto errorexit;
      }
   }

   for (size_t i=0; i<sizeof fields/sizeof fields[0];i ++) {

      char newval[200];
      snprintf (newval, sizeof newval, "Field %zu", i);
      if (!(cdb_field_mod (&record, fields[i].name, newval))) {
         fprintf (stderr, "Failed to modify field\n");
         goto errorexit;
      }

      char *field_value = cdb_field_find (record, fields[i].name);
      if (!field_value) {
         fprintf (stderr, "..Failed to find field [%s]\n", fields[i].name);
         goto errorexit;
      }
      printf ("Found field [%s][%s]\n", fields[i].name, field_value);
      cdb_field_free (field_value);
      field_value = NULL;

      if (i % 2) {
         if (!(cdb_field_del (&record, fields[i].name))) {
            fprintf (stderr, "Failed to delete field [%s]\n", fields[i].name);
            goto errorexit;
         }
      }
   }

   if (!(outf = fopen (TEST_FILE, "w"))) {
      fprintf (stderr, "Failed to open [%s] for writing: %m\n", TEST_FILE);
      goto errorexit;
   }

   if (!(cdb_record_add (&records, record))) {
      fprintf (stderr, "Failed to add first record to database\n");
      goto errorexit;
   }

   for (size_t i=0; i<sizeof test_records/sizeof test_records[0]; i++) {
      if (!(cdb_record_add (&records, test_records[i]))) {
         fprintf (stderr, "Failed to add record %zu to db\n", i);
         goto errorexit;
      }
   }

   if (!(cdb_records_save (records, outf))) {
      fprintf (stderr, "Failed to save records to [%s]: %m\n", TEST_FILE);
      goto errorexit;
   }

   error = false;

errorexit:

   free (record);

   if (outf) {
      fclose (outf);
   }

   cdb_records_free (records);

   return !error;
}

static bool read_file (void)
{
   bool error = true;
   char **records = NULL;
   FILE *inf = NULL;

   if (!(inf = fopen (TEST_FILE, "rt"))) {
      fprintf (stderr, "Failed to open [%s] for reading: %m\n", TEST_FILE);
      goto errorexit;
   }

   if (!(records = cdb_records_load (inf))) {
      fprintf (stderr, "Failed to load [%s]: file corrupt\n", TEST_FILE);
      goto errorexit;
   }

errorexit:

   cdb_records_free (records);
   if (inf)
      fclose (inf);

   return !error;
}

int main (int argc, char **argv)
{
   int ret = EXIT_FAILURE;

   if (!(write_new_file ())) {
      fprintf (stderr, "Write test failure\n");
      goto errorexit;
   }

   if (!(read_file ())) {
      fprintf (stderr, "reade test failure\n");
      goto errorexit;
   }

   printf ("Testing CleverDB\n");

   ret = EXIT_SUCCESS;

errorexit:
   return ret;
}

