
#ifndef H_ROTSIT
#define H_ROTSIT

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// These are the fields in each record
#define FIELD_GUID         ("GUID")
#define FIELD_ORDER        ("ORDER") // TODO: WTH was this used for?
#define FIELD_OPENED_BY    ("OPENED-BY")
#define FIELD_OPENED_ON    ("CREATED-ON")
#define FIELD_ISSUE_DESCR  ("ISSUE-DESCR")
#define FIELD_STATUS       ("STATUS")
#define FIELD_ASSIGNED_TO  ("ASSIGNED-TO")
#define FIELD_ASSIGNED_ON  ("ASSIGNED-ON")
#define FIELD_CLOSED_BY    ("CLOSED-BY")
#define FIELD_CLOSED_ON    ("CLOSED-ON")
#define FIELD_CLOSED_MSG   ("CLOSE-MSG")
#define FIELD_DUP_BY       ("DUPLICATED-BY")
#define FIELD_DUPS         ("DUPLICATES")
#define FIELD_DUP_MSG      ("DUPLICATE-MSG")

// From field RF_DUP_MSG onwards on all the remaining fields in a record
// are comments added to the issue. Each comment has the following fields:
#define CF_GUID         (0)
#define CF_USER         (1)
#define CF_TIME         (2)
#define CF_COMMENT      (3)

#ifdef __cplusplus
extern "C" {
#endif

   // Returns the record just added.
   char *rotsit_issue_add (char ***records, const char *msg);

   uint32_t (*rotsit_user_rand) (void);

#ifdef __cplusplus
};
#endif

#endif

