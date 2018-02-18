
#ifndef H_ROTSIT
#define H_ROTSIT

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// These are the fields in each record
#define RF_GUID         (0)
#define RF_ORDER        (1)
#define RF_OPENED_BY    (2)
#define RF_OPENED_ON    (3)
#define RF_OPENED_MSG   (4)
#define RF_ASSIGNED_BY  (5)
#define RF_ASSIGNED_TO  (6)
#define RF_ASSIGNED_ON  (7)
#define RF_CLOSED_BY    (8)
#define RF_CLOSED_ON    (9)
#define RF_CLOSED_MSG   (10)
#define RF_DUP_BY       (11)
#define RF_DUP_GUID     (12)
#define RF_DUP_MSG      (13)
#define RF_LAST_FIELD   RF_DUP_MSG

// From field RF_DUP_MSG onwards on all the remaining fields in a record
// are comments added to the issue. Each comment has the following fields:
#define CF_GUID         (0)
#define CF_USER         (1)
#define CF_TIME         (2)
#define CF_COMMENT      (3)

typedef struct rotsit_t rotsit_t;
typedef struct rotrec_t rotrec_t;

#ifdef __cplusplus
extern "C" {
#endif

   rotsit_t *rotsit_parse (char *input_buf);
   void rotsit_del (rotsit_t *rs);
   void rotsit_dump (rotsit_t *rs, const char *id, FILE *outf);
   bool rotsit_write (rotsit_t *rs, FILE *outf);

   uint32_t rotsit_count_records (rotsit_t *rs);
   rotrec_t *rotsit_get_record (rotsit_t *rs, uint32_t recnum);

   bool rotsit_add_record (rotsit_t *rs, rotrec_t *rr);

   rotrec_t *rotrec_new (const char *msg);

   bool rotrec_set_field (rotrec_t *rr, uint8_t fieldnum, const char *src);
   bool rotrec_add_comment (rotrec_t *rr, const char *comment);


#ifdef __cplusplus
};
#endif

#endif

