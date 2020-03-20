#include <string.h>
#include <stdlib.h>

#include "cdb.h"


#define DATABASE_VERSION         (1)
#define RECORD_DELIM             "f\b\n"
#define FIELD_DELIM              "f\b"


bool cdb_records_save (char **records, FILE *outf)
{
   if (!outf)
      return false;

   fprintf (outf, "%i", DATABASE_VERSION);

   for (size_t i=0; records && records[i]; i++) {
      fprintf (outf, "%s" RECORD_DELIM, records[i]);
   }

   return true;
}

char *cdb_record_add (char ***records, const char *record)
{
   bool error = true;
   char *newrec = NULL;
   size_t nrecs = 0;
   char **tmp = NULL;

   if (!(newrec = malloc (strlen (record) + 4)))
      goto errorexit;

   strcpy (newrec, record);
   strcat (newrec, RECORD_DELIM);

   while (*records && (*records)[nrecs])
      nrecs++;

   size_t newlen = nrecs + 1;
   if (!(tmp = realloc (*records, (newlen + 1) * (sizeof *tmp))))
      goto errorexit;

   tmp[nrecs] = newrec;
   tmp[nrecs + 1] = NULL;

   *records = tmp;

   error = false;

errorexit:

   if (error) {
      free (newrec);
      newrec = NULL;
   }

   return newrec;
}

static char *make_field (const char *name, const char *value)
{
   if (!name || !value)
      return NULL;

   size_t name_len = strlen (name);
   size_t value_len = strlen (value);

   char *ret = NULL;

   if (!(ret = malloc (name_len + 1 + value_len + 1)))
      return NULL;

   strcpy (ret, name);
   strcat (ret, ":");
   strcat (ret, value);

   return ret;
}

bool cdb_field_add (char **record, const char *name, const char *value)
{
   char *newfield = NULL;
   size_t record_len = 0;
   if (*record)
      record_len = strlen (*record) + 1;

   if (!(newfield = make_field (name, value)))
      return false;

   char *tmp = realloc (*record, record_len + strlen (newfield) + 1);
   if (!tmp) {
      free (newfield);
      return NULL;
   }
   if (!*record)
      tmp[0] = 0;

   (*record) = tmp;
   strcat (*record, newfield);
   return true;
}

static char *find_field (char *record, const char *name)
{
   char *field = record;
   size_t name_len = strlen (name);
   size_t delim_len = strlen (FIELD_DELIM);

   while ((field = strstr (&field[1], FIELD_DELIM))!=NULL) {
      field += delim_len;
      if ((strncmp (field, name, name_len))==0)
         return field;
   }
   return NULL;
}

bool cdb_field_mod (char **record, const char *name, const char *value)
{
   char *newrec = NULL;
   size_t delim_len = strlen (FIELD_DELIM);
   if (!*record || !name || !value)
      return false;

   char *field = *record;
   while ((field = strstr (field, FIELD_DELIM))!=NULL) {
      field += delim_len;
      char *fname = field
      if ((strncmp (field, name, name_len))==0) {
         if (!(cdb_field_add (&newrec, name, value))) {
            free (newrec);
            return false;
         }
         continue;
      }
   }

   return false;
}

bool cdb_field_del (char **record, const char *name)
{
   return false;
}


char *cdb_field_find (char *record, const char *name)
{
   return false;
}

void cdb_field_free (char *field)
{
   return false;
}




