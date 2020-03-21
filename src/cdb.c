#include <string.h>
#include <stdlib.h>

#include "cdb.h"


#define DATABASE_VERSION         (1)
#define RECORD_DELIM             "f\b\n"
#define FIELD_DELIM              "f\b"


char *strdup (const char *);

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

void cdb_records_free (char **records)
{
   if (!records)
      return;

   for (size_t i=0; records[i]; i++) {
      free (records[i]);
   }
   free (records);
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

   char *tmp = realloc (*record, record_len +
                                 strlen (newfield) +
                                 strlen (FIELD_DELIM) +
                                 + 1);
   if (!tmp) {
      free (newfield);
      return NULL;
   }

   strcat (tmp, FIELD_DELIM);

   (*record) = tmp;
   strcat (*record, newfield);

   free (newfield);

   return true;
}

static char *find_field (char *record, const char *name)
{
   char *field = record;
   size_t name_len = strlen (name);
   size_t delim_len = strlen (FIELD_DELIM);

   while ((field = strstr (field, FIELD_DELIM))!=NULL) {
      field += delim_len;
      if ((strncmp (field, name, name_len))==0)
         return field;
   }
   return NULL;
}

bool cdb_field_del (char **record, const char *name)
{
   char *newrec = NULL;
   char *start = NULL,
        *end = NULL;

   size_t delim_len = strlen (FIELD_DELIM),
          record_len = 0,
          nchars = 0;


   if (!*record || !name)
      return false;

   record_len = strlen (*record);

   if (!(start = find_field (*record, name)))
      return false;

   start -= delim_len;

   if (!(end = strstr (start + delim_len, FIELD_DELIM)))
      end = &(*record)[record_len];

   while (end[nchars])
      nchars++;

   memmove (start, end, nchars + 1);

   return true;
}

bool cdb_field_mod (char **record, const char *name, const char *value)
{
#if 0
   if (!(cdb_field_add (record, name, value)) ||
       !(cdb_field_del (record, name))) {
      return false;
   }
#endif
   if (!(cdb_field_add (record, name, value))) {
      fprintf (stderr, "Failed to add [%s:%s]\n", name, value);
      return false;
   }
   if (!(cdb_field_del (record, name))) {
      fprintf (stderr, "Failed to remove [%s:%s]\n", name, value);
      return false;
   }
   fprintf (stderr, "Modified\n");
   return true;
}


char *cdb_field_find (char *record, const char *name)
{
   char *ret = NULL;
   const char *fval = find_field (record, name);
   if (!fval)
      return NULL;

   fval += strlen (name) + 1;

   return strdup (fval);
}

void cdb_field_free (char *field)
{
   free (field);
}




