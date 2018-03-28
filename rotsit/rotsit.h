
#ifndef H_ROTSIT
#define H_ROTSIT

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef PLATFORM_WINDOWS
#define ENV_USERNAME       ("USERNAME")
#else
#define ENV_USERNAME       ("USER")
#endif

// These are the fields in each record
#define RF_GUID         (0)
#define RF_ORDER        (1)
#define RF_OPENED_BY    (2)
#define RF_OPENED_ON    (3)
#define RF_OPENED_MSG   (4)
#define RF_STATUS       (5)
#define RF_ASSIGNED_BY  (6)
#define RF_ASSIGNED_TO  (7)
#define RF_ASSIGNED_ON  (8)
#define RF_CLOSED_BY    (9)
#define RF_CLOSED_ON    (10)
#define RF_CLOSED_MSG   (11)
#define RF_DUP_BY       (12)
#define RF_DUP_GUID     (13)
#define RF_DUP_MSG      (14)
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
   rotrec_t **rotsit_filter (rotsit_t *rs, const char *expr);

   rotrec_t *rotsit_find_by_id (rotsit_t *rs, const char *id);

   bool rotsit_add_record (rotsit_t *rs, rotrec_t *rr);

   rotrec_t *rotrec_new (const char *msg);
   void rotrec_del (rotrec_t *rec);

   bool rotrec_add_comment (rotrec_t *rr, const char *comment);
   bool rotrec_close (rotrec_t *rr, const char *message);
   bool rotrec_dup (rotrec_t *rr, const char *id);
   bool rotrec_reopen (rotrec_t *rr, const char *message);

   const char *rotrec_get_field (rotrec_t *rr, size_t field);

   bool rotrec_dump (rotrec_t *rr, FILE *outf);

#ifdef __cplusplus
};
#endif

#endif

