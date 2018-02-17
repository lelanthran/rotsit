
#ifndef H_ROTSIT
#define H_ROTSIT

#include <stdio.h>

typedef struct rotsit_t rotsit_t;
typedef struct rotrec_t rotrec_t;

#ifdef __cplusplus
extern "C" {
#endif

   rotsit_t *rotsit_parse (char *input_buf);
   void rotsit_del (rotsit_t *rs);
   void rotsit_dump (rotsit_t *rs, const char *id, FILE *outf);

#ifdef __cplusplus
};
#endif

#endif

