
#ifndef H_CDB
#define H_CDB

#include <stdint.h>

typedef struct cdb_t cdb_t;

#define CDB_CREATE      (1 << 0)
#define CDB_READ        (1 << 1)
#define CDB_WRITE       (1 << 2)

#ifdef __cplusplus
extern "C" {
#endif

   cdb_t *cdb_open (const char *fname, uint32_t flags);
   void cdb_close (cdb_t *cdb);


#ifdef __cplusplus
};
#endif


#endif

