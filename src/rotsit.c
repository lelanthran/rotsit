
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#include <sys/types.h>
#include <unistd.h>


#include "rotsit.h"
#include "cdb.h"

#define PROG_ERR(...)      do {\
   fprintf (stderr, "[%s:%i] ", __FILE__, __LINE__);\
   fprintf (stderr, __VA_ARGS__);\
} while (0)

#ifdef PLATFORM_WINDOWS
#define ENV_USERNAME    "USERNAME"
#define UNAMEVAR        "\%USERNAME\%"
#else
#define ENV_USERNAME    "USER"
#define UNAMEVAR        "$USER"
#endif


#define RECORD_DELIM       ("f\b\n")
#define FIELD_DELIM        ("f\b")

#if 0 // TODO: If I ever want to put the query language back in here
static void *exec_op (const void *p_op, void const *p_lhs, void const *p_rhs)
{
   const char *s_op = p_op;
   const char *s_lhs = p_lhs;
   const char *s_rhs = p_rhs;

   int result = -1;
   int64_t lhs = 0;
   int64_t rhs = 0;
   char tmp[40];

   bool parsed = false;

   enum pdate_errcode_t d_err_lhs, d_err_rhs;
   time_t tv_lhs, tv_rhs;

   // First try to parse this as a date. If it's a valid date we use it as
   // a large integer.
   d_err_lhs = pdate_parse (s_lhs, &tv_lhs, true);
   d_err_rhs = pdate_parse (s_rhs, &tv_rhs, true);

   // PROG_ERR (" [%s] (%s) [%s]\n", s_lhs, s_op, s_rhs);

   if (d_err_lhs==pdate_valid && d_err_rhs==pdate_valid) {
      // At this point the operands could still be numbers and not dates.
      bool is_date = false;
      for (size_t i=1; s_lhs[i]; i++) {
         if (!isxdigit (s_lhs[i])) {
            is_date = true;
            break;
         }
      }
      for (size_t i=1; !is_date && s_rhs[i]; i++) {
         if (!isxdigit (s_rhs[i])) {
            is_date = true;
            break;
         }
      }

      if (is_date) {
         lhs = (int32_t)tv_lhs;
         rhs = (int32_t)tv_rhs;
         PROG_ERR ("Read dates [0x%016" PRIx64 "], [0x%016" PRIx64 "] \n",
               lhs, rhs);
         parsed = true;
      }
   }

   // Next, try to read lhs/rhs as a number. If both are numbers
   // then we assume that the integer operators apply
   if (!parsed) {
      int i_lhs = sscanf (s_lhs, "0x%016" PRIx64, &lhs);
      int i_rhs = sscanf (s_rhs, "0x%016" PRIx64, &rhs);

      if (i_lhs==1 && i_rhs==1) {
         parsed = true;
      }
   }

   // If none of the above parsings worked, then we treat the operands as
   // strings. Note that not all operators are defined for strings, only
   // the equality and non-equality.
   if (!parsed) {
      tmp [0] = '0';
      tmp [1] = 0;
      if (*s_op == '!') {
         sprintf (tmp, "%i", strstr (s_rhs, s_lhs)==NULL);
      }
      if (*s_op == '=') {
         sprintf (tmp, "%i", strstr (s_rhs, s_lhs)!=NULL);
      }
      if (tmp[0] == '0') {
         if (*s_op == '!') {
            sprintf (tmp, "%i", strstr (s_lhs, s_rhs)==NULL);
         }
         if (*s_op == '=') {
            sprintf (tmp, "%i", strstr (s_lhs, s_rhs)!=NULL);
         }
      }
      return strdup (tmp);
   }

   switch (*s_op) {
      case '+':   result = lhs + rhs;  break;
      case '-':   result = lhs - rhs;  break;
      case '*':   result = lhs * rhs;  break;
      case '/':   result = lhs / rhs;  break;
      case '<':   result = lhs < rhs;  break;
      case '>':   result = lhs > rhs;  break;
      case '=':   result = lhs == rhs; break;
      case '!':   result = lhs != rhs; break;
      case '&':   result = lhs && rhs; break;
      case '|':   result = lhs || rhs; break;
   }

   sprintf (tmp, "%i", result);
   return strdup (tmp);
}

static eval_type_t check_type (void const *token)
{
   const char *s_token = token;

   if (s_token[1] && s_token[1]!='=')
      return eval_OPERAND;

   switch (*s_token) {
      case '*':
      case '/':   return eval_HIGH_OPS;
      case '=':
      case '!':
      case '+':
      case '-':
      case '<':
      case '>':
      case '&':
      case '|':   return eval_LOW_OPS;
      case '(':   return eval_OPEN;
      case ')':   return eval_CLOSE;
      default:    return eval_OPERAND;
   }
   return eval_UNKNOWN;
}
#endif

#if 0    // Will come in handy when I add in case-insensitive matching
static bool istrcmp (const char *lhs, const char *rhs)
{
   if (!lhs)
      return false;

   if (!rhs)
      return false;

   while (*lhs && *rhs && (tolower (*lhs++) == tolower (*rhs++)))
      ;

   return *lhs == *rhs;
}
#endif

static uint32_t hash_buffer (uint64_t current, void *buf, size_t len)
{
   uint8_t *b = buf;
   for (size_t i=0; i<len; i++) {
      current ^= (current * 31) + b[i];
   }
   current = current ^ (current >> 15);
   current = current ^ (current * 0x4e85ca7d);
   current = current ^ (current >> 15);
   current = current ^ (current * 0x4e85ca7d);
   return current;
}

static uint32_t getclock (uint32_t current)
{
   for (size_t i=0; i<0x000f; i++) {
      // Clock may not have enough resolution over repeated calls, so
      // add some entropy by counting how long it takes to change
      clock_t start = clock ();
      uint16_t counter = 0;
      while (start == clock()) {
         counter++;
      }
      current = hash_buffer (current, &counter, sizeof counter);
      current = hash_buffer (current, &start, sizeof start);
   }
   return current;
}

static uint64_t quickseed (void)
{
#ifndef PLATFORM_Windows
   extern int main (void);
#endif

   uint64_t ret = 0;

   /* Sources of entropy:
    * 1. Epoch in seconds since 1970:
    * 2. Processor time since program start:
    * 3. Pointer to current stack position:
    * 4. Pointer to heap:
    * 5. Pointer to function (self):
    * 6. Pointer to main():
    * 7. PID:
    * 8. mkstemp()
    */

   time_t epoch = time (NULL);
   uint32_t proc_time = getclock (ret);
   uint64_t *ptr_stack = &ret;
   uint64_t (*ptr_self) (void) = quickseed;
#ifndef PLATFORM_Windows
   int (*ptr_main) (void) = main;
#else
   int (*ptr_main) (void) = NULL;
#endif
   void *(*ptr_malloc) (size_t) = malloc;
   pid_t pid = getpid ();

   uint32_t int_stack = (uintptr_t)ptr_stack;
   uint32_t *tmp = malloc (1);
   uint32_t *ptr_heap_small = tmp
      ? tmp
      : (void *) (uintptr_t) ((int_stack >> 1) * (int_stack >> 1));
   free (tmp);
   tmp = malloc (1024 * 1024);
   uint32_t *ptr_heap_large = tmp
      ? tmp
      : (void *) (uintptr_t) ((proc_time >> 1) * (proc_time >> 1));
   free (tmp);

   char tmp_fname[] = "tmpfileXXXXXX";
   int fd = mkstemp (tmp_fname);
   if (fd >= 0) {
      close (fd);
      if ((remove (tmp_fname))!=0) {
         PROG_ERR ("Failed to remove temporary file [%s]: %m\n"
                   "Please delete this file manually\n", tmp_fname);
      }
   }

   ret = hash_buffer (ret, &epoch, sizeof epoch);
   ret = hash_buffer (ret, &proc_time, sizeof proc_time);
   ret = hash_buffer (ret, &ptr_stack, sizeof ptr_stack);
   ret = hash_buffer (ret, &ptr_self, sizeof ptr_self);
   ret = hash_buffer (ret, &ptr_main, sizeof ptr_main);
   ret = hash_buffer (ret, &ptr_malloc, sizeof ptr_malloc);
   ret = hash_buffer (ret, &pid, sizeof pid);
   ret = hash_buffer (ret, &ptr_heap_small, sizeof ptr_heap_small);
   ret = hash_buffer (ret, &ptr_heap_large, sizeof ptr_heap_large);
   ret = hash_buffer (ret, tmp_fname, strlen (tmp_fname));

   return ret;
}
static void get_random_bytes (void *dst, size_t num_bytes)
{
#ifdef PLATFORM_WINDOWS
#define SHIFTWIDTH         (8)
#else
#define SHIFTWIDTH         (8)
#endif

   static uint64_t seed = 0;
   if (!seed) {
      seed = quickseed ();
      srand ((int)seed);
   }

   uint8_t *buf = dst;

   for (size_t i=0; i<num_bytes; i++) {
      uint32_t r = rand ();
      buf[i] = (r >> SHIFTWIDTH) & 0xff;
   }
#undef SHIFTWIDTH
}

static char *make_guid (void)
{
   uint64_t guid;

   char *str_guid = malloc (2 + 16 + 1); // 0x + 16 digits + 0

   if (!str_guid) {
      PROG_ERR ("Out of memory\n");
      return NULL;
   }

   // xcrypto_random ((uint8_t *)&guid, sizeof guid);
   get_random_bytes (&guid, sizeof guid);

   sprintf (str_guid, "0x%016" PRIx64, guid);
   return str_guid;
}

static char *make_username (void)
{
   char *str_user = NULL;
   char *tmp_user = getenv (ENV_USERNAME);

   str_user = strdup (tmp_user ? tmp_user : "Unknown");

   if (!str_user) {
      PROG_ERR ("Out of memory\n");
      return NULL;
   }

   return str_user;
}

// TODO: Make this function thread-safe. Might need a mutex because
// localtime_r is not available on MingW32.
static char *make_time (time_t time_s)
{
   char *str_time = NULL;
   if (!time_s) {
      time_s = time (NULL);
   }

   str_time = strdup (asctime (localtime (&time_s)));
   if (!str_time) {
      PROG_ERR ("Out of memory\n");
      return NULL;
   }

   char *tmp = strchr (str_time, '\n');
   if (tmp)
      *tmp = 0;

   return str_time;
}

char *rotsit_issue_add (char ***records, const char *msg)
{
   bool error = true;
   char *ret = NULL;

   char *user = make_username (),
        *guid = make_guid (),
        *cr_time = make_time (time (NULL));

   if (!records || !msg) {
      PROG_ERR ("Invalid params\n");
      goto errorexit;
   }

   if (!user || !guid || !cr_time) {
      PROG_ERR ("Out of memory error\n");
      goto errorexit;
   }

   if (!(cdb_field_add (&ret, FIELD_GUID, guid))         ||
       !(cdb_field_add (&ret, FIELD_OPENED_BY, user))    ||
       !(cdb_field_add (&ret, FIELD_OPENED_ON, cr_time)) ||
       !(cdb_field_add (&ret, FIELD_ISSUE_DESCR, msg))   ||
       !(cdb_record_add (records, ret))) {
      PROG_ERR ("Failed to create new record and add it to record list\n");
      goto errorexit;
   }

   error = false;

errorexit:

   free (user);
   free (guid);
   free (cr_time);

   if (error) {
      free (ret);
      ret = NULL;
   }
   return ret;
}

