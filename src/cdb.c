#include <string.h>
#include <stdlib.h>

#include "cdb.h"


#define DATABASE_VERSION         (1)
#define RECORD_DELIM             "f\b\n"
#define FIELD_DELIM              "f\b"

#define MAX_LINE      (1024 * 1024 * 10)   // 10MB lines, max

char *strdup (const char *);

char **cdb_records_load (FILE *inf, uint32_t *app_version)
{
   bool error = true;

   char **ret = NULL;
   size_t nrecs = 0,
          idx = 0;
   uint32_t version = 0;
   char *field = NULL;
   char *tmp = NULL;

   char *line = malloc (MAX_LINE);
   if (!line)
      goto errorexit;

   line[0] = 0;
   if ((fgets (line, MAX_LINE, inf))         && // DB Version
       (tmp = strchr (line, ':'))            && // DB Version
       (sscanf (&tmp[1], "%u", &version))==1 && // DB Version
       version <= DATABASE_VERSION           && // DB Version
       (fgets (line, MAX_LINE, inf))         && // Fields Version
       (tmp = strchr (line, ':'))            && // Fields Version
       (sscanf (&tmp[1], "%u", &version))    && // Fields Version
       (fgets (line, MAX_LINE, inf))         && // Nrecs
       (tmp = strchr (line, ':'))            && // Nrecs
       (sscanf (&tmp[1], "%zu", &nrecs))) {     // Nrecs
      // SUCCESS
   } else {
      free (line);
      return NULL;
   }

   if (!(ret = calloc (nrecs + 1, sizeof *ret)))
      return NULL;

   while (!feof (inf) && !ferror (inf) && (fgets (line, MAX_LINE, inf))) {
      char *eol = strchr (line, '\n');
      if (eol)
         *eol = 0;

      if (!(ret[idx++] = strdup (line)))
         goto errorexit;
   }

   if (app_version)
      *app_version = version;

   error = false;

errorexit:
   free (line);

   if (error) {
      cdb_records_free (ret);
      ret = NULL;
   }

   return ret;
}


bool cdb_records_save (char **records,uint32_t app_version,  FILE *outf)
{
   size_t nrecs = 0;

   if (!outf)
      return false;

   for (size_t i=0; records && records[i]; i++)
      nrecs++;

   fprintf (outf, "version: %i\n", DATABASE_VERSION);
   fprintf (outf, "app-version: %u\n", app_version);
   fprintf (outf, "record count: %zu\n", nrecs);

   for (size_t i=0; records && records[i]; i++) {
      fprintf (outf, "%s" RECORD_DELIM, records[i]);
   }

   return true;
}

static void array_string_free (char **array)
{
   if (!array)
      return;

   for (size_t i=0; array[i]; i++) {
      free (array[i]);
   }
   free (array);
}

void cdb_records_free (char **records)
{
   array_string_free (records);
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

   if (!*record)
      tmp[0] = 0;

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

char *cdb_field_get (char *record, const char *name)
{
   bool error = true;
   char *ret = NULL;
   const char *start = NULL;
   const char *end = NULL;

   if (!record || !name)
      goto errorexit;

   if (!(start = find_field (record, name)))
      goto errorexit;

   if (!(start = strchr (start, ':')))
      goto errorexit;

   start++;
   if (!(end = strstr (start, FIELD_DELIM)))
      goto errorexit;

   size_t len = (size_t) (end - start);
   if (!(ret = malloc (len + 1)))
      goto errorexit;

   strncpy (ret, start, len);
   ret[len] = 0;

   error = false;

errorexit:
   if (error) {
      free (ret);
      ret = NULL;
   }

   return ret;
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
      return false;
   }
   if (!(cdb_field_del (record, name))) {
      fprintf (stderr, "Failed to remove [%s:%s]\n", name, value);
      // return false; // TODO: Decide which is better
      return true;
   }
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

char **cdb_field_list (char *record)
{
   bool error = true;
   char **ret = NULL;
   size_t nfields = 0;
   char *field = NULL;

   if (!record)
      goto errorexit;

   field = record;

   while ((field = strstr (field, FIELD_DELIM))!=NULL) {
      field += strlen (FIELD_DELIM);
      char *value = strchr (field, ':');
      if (!value)
         break;

      size_t len = (size_t)(value - field);
      char *name = malloc (len + 1);
      if (!name)
         goto errorexit;

      strncpy (name, field, len);
      name[len] = 0;

      char **tmp = realloc (ret, (nfields + 2) * sizeof *tmp);
      if (!tmp) {
         free (name);
         goto errorexit;
      }

      ret = tmp;
      ret[nfields++] = name;
      ret[nfields] = NULL;
   }

   error = false;

errorexit:
   if (error) {
      cdb_field_list_free (ret);
      ret = NULL;
   }

   return ret;
}

void cdb_field_list_free (char **field_names)
{
   array_string_free (field_names);
}



