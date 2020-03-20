
#ifndef H_CDB
#define H_CDB

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef bool (cdb_filter_func_t) (const void *lhs, const void *rhs);

#ifdef __cplusplus
extern "C" {
#endif

   char **cdb_records_load (const char *fname, int *errcode);
   void cdb_records_save (char **records);
   void cdb_records_free (char **records);

   char **cdb_records_filter (char **records, cdb_filter_func_t *fptr);

   bool cdb_field_add (char **record, const char *name, const char *value);
   bool cdb_field_mod (char **record, const char *name, const char *value);
   bool cdb_field_del (char **record, const char *name);

   char *cdb_field_find (char *record, const char *name);
   void cdb_field_free (char *field);

   char **cdb_field_list (char *record);
   void cdb_field_list_free (char **field_names);



#ifdef __cplusplus
};
#endif


#endif
