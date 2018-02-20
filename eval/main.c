
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


#include "eval/eval.h"

#include "xstring/xstring.h"
#include "xvector/xvector.h"

void *exec_op (const void *p_op, void const *p_lhs, void const *p_rhs)
{
   const char *s_op = p_op;
   const char *s_lhs = p_lhs;
   const char *s_rhs = p_rhs;

   int result = -1;
   int lhs = 0;
   int rhs = 0;
   sscanf (s_lhs, "%i", &lhs);
   sscanf (s_rhs, "%i", &rhs);

   switch (*s_op) {
      case '+':   result = lhs + rhs; break;
      case '-':   result = lhs - rhs; break;
      case '*':   result = lhs * rhs; break;
      case '/':   result = lhs / rhs; break;
      case '<':   result = lhs < rhs; break;
      case '>':   result = lhs > rhs; break;
      case '=':   result = lhs == rhs; break;
      case '!':   result = lhs != rhs; break;
   }

   char tmp[40];
   sprintf (tmp, "%i", result);
   return xstr_dup (tmp);
}

eval_type_t check_type (void const *token)
{
   const char *s_token = token;
   switch (*s_token) {
      case '*':
      case '/':   return eval_HIGH_OPS;
      case '=':
      case '!':
      case '+':
      case '-':
      case '<':
      case '>':   return eval_LOW_OPS;
      case '(':   return eval_OPEN;
      case ')':   return eval_CLOSE;
      default:    return eval_OPERAND;
   }
   return eval_UNKNOWN;
}

static char **make_tokens (const char *input)
{
   bool error = true;
   char **ret = NULL;
   xvector_t *xv = NULL;
   char *local = xstr_dup (input);
   if (!local) {
      goto errorexit;
   }

   char *start = local;
   while (*start) {
      char *new_token = NULL;
      char *end;
      char tmp_c;

      switch (*start) {
         case '(':   new_token = xstr_dup ("("); break;
         case ')':   new_token = xstr_dup (")"); break;
         case '+':   new_token = xstr_dup ("+"); break;
         case '-':   new_token = xstr_dup ("-"); break;
         case '/':   new_token = xstr_dup ("/"); break;
         case '*':   new_token = xstr_dup ("*"); break;
         case '<':   new_token = xstr_dup ("<"); break;
         case '>':   new_token = xstr_dup (">"); break;

         case '=':   start++;
                     if (*start=='=')
                        new_token = xstr_dup ("==");
                     else
                        printf ("Warning: error near: %s\n", start);
                     break;
         case '!':   start++;
                     if (*start=='=')
                        new_token = xstr_dup ("!=");
                     else
                        printf ("Warning: error near: %s\n", start);
                     break;

         case ' ':
         case '\n':
         case '\r':
         case '\t':  break;

         default: end = start;
                  while (isalnum (*end))
                     end++;
                  tmp_c = *end;
                  *end = 0;
                  new_token = xstr_dup (start);
                  *end = tmp_c;
                  end--;
                  start = end;
                  break;
      }

      if (new_token) {
         xvector_t *tmp = xvector_ins_tail (xv, new_token);
         if (!tmp) {
            free (new_token);
            goto errorexit;
         }
         xv = tmp;
      }

      if (*start)
         start++;
   }

   ret = xvector_native (xv);
   error = false;

errorexit:

   free (local);
   xvector_free (xv);

   if (error) {
      xstr_delarray (ret);
      ret = NULL;
   }
   return ret;
}

int main (void)
{
   int ret = EXIT_FAILURE;
   static const struct {
      int result;
      const char *expr;
   } tests[] = {
      { 1,  "5 != 3" },
      { 0,  "5 == 3" },
      { 8,  "5 + 3" },
      { 11, "5 + 3 * 2" },
      { 16, "(5 + 3) * 2" },
      { 14, "(5 + (3 - 1)) * 2" },
      { 0,  "((5 + (3 - 1)) * 2) == 14" },
      { 1,  "((5 + (3 - 1)) * 2) > 10" },
   };

   eval_t *ev = eval_new ((void *(*) (const void *))xstr_dup,
                          (void (*) (void *))free,
                           exec_op, check_type);
   if (!ev) {
      printf ("Failed to create new eval context\n");
      goto errorexit;
   }

   for (size_t i=0; i<sizeof tests/sizeof tests[0]; i++) {
      char **tokens = make_tokens (tests[i].expr);
      if (!tokens) {
         printf ("Error producing tokens for [%s]\n", tests[i].expr);
         goto errorexit;
      }
      printf ("-----------------------------\n");
      char *result = eval_execute (ev, (const void **)tokens);
      int ires = -1;
      if (result) {
         sscanf (result, "%i", &ires);
      }
      free (result);
      result = ires == tests[i].result ? "passed" : "failed";

      printf ("[%s] = [%i] (%s)\n", tests[i].expr, ires, result);
      for (size_t j=0; tokens[j]; j++) {
         free (tokens[j]);
      }
      free (tokens);
      eval_clear (ev);
   }

   ret = EXIT_SUCCESS;
errorexit:
   eval_del (ev);
   return ret;
}
