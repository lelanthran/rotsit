
#ifndef H_PDATE
#define H_PDATE

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

enum pdate_errcode_t {
   pdate_valid = 0,
   pdate_ambig_year,
   pdate_ambig_month,
   pdate_ambig_day,
   pdate_ambig_hour,
   pdate_ambig_min,
   pdate_ambig_sec,
   pdate_unknown_field,
   pdate_error,
   pdate_invalid,
};

#ifdef __cplusplus
extern "C" {
#endif

   enum pdate_errcode_t pdate_parse (const char *string, time_t *ret, 
                                     bool fromdate);
   const char *pdate_errmsg (enum pdate_errcode_t pd);
   void pdate_test (void);

#ifdef __cplusplus
};
#endif


#endif
