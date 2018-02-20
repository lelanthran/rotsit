#include <stdlib.h>
#include <string.h>

#include "eval/eval.h"

#include "xvector/xvector.h"

struct eval_t {
   xvector_t *st1;
   xvector_t *st2;
   eval_copy_t       *fcopy;
   eval_del_t        *fdel;
   eval_run_op_t     *frun;
   eval_typefunc_t   *ftype;
};

eval_t *eval_new (eval_copy_t *copy_func, eval_del_t *del_func,
                  eval_run_op_t *run_op, eval_typefunc_t *type)
{
   eval_t *ret = malloc (sizeof *ret);
   if (!ret)
      return NULL;

   memset (ret, 0, sizeof *ret);
   ret->fcopy = copy_func;
   ret->fdel = del_func;
   ret->frun = run_op;
   ret->ftype = type;

   return ret;
}

void  eval_del (eval_t *ev)
{
   if (!ev)
      return;

   xvector_iterate (ev->st1, (void (*) (void *))ev->fdel);
   xvector_iterate (ev->st2, (void (*) (void *))ev->fdel);

   xvector_free (ev->st1);
   xvector_free (ev->st2);
   free (ev);
}

void eval_clear (eval_t *ev)
{
   xvector_iterate (ev->st1, (void (*) (void *))ev->fdel);
   xvector_iterate (ev->st2, (void (*) (void *))ev->fdel);

   xvector_free (ev->st1);
   xvector_free (ev->st2);

   ev->st1 = NULL;
   ev->st2 = NULL;
}

static bool push (xvector_t **st, eval_copy_t *cf, const void *elm)
{
   if (!elm)
      return true;

   void *tmp = cf (elm);
   if (!tmp)
      return false;

   xvector_t *tmpv = xvector_ins_tail ((*st), tmp);
   if (!tmpv)
      return false;

   (*st) = tmpv;
   return true;
}

static void *pop (xvector_t **st)
{
   if (!st || !(*st) || XVECT_LENGTH (*st)==0)
      return NULL;

   return xvector_del_tail ((*st));
}

static size_t length (xvector_t *st)
{
   return XVECT_LENGTH (st);
}

static bool apply (eval_t *ev)
{
   bool error = true;
   void *rhs = pop (&ev->st1);
   void *lhs = pop (&ev->st1);
   void *op = pop (&ev->st2);

   if (!lhs || !rhs || !op) {
       push (&ev->st1, ev->fcopy, lhs);
       push (&ev->st1, ev->fcopy, rhs);
       goto errorexit;
   }

   void *lcpy = ev->frun (op, lhs, rhs);
   error = push (&ev->st1, ev->fcopy, lcpy) ? false : true;
   ev->fdel (lcpy);

errorexit:

   ev->fdel (lhs);
   ev->fdel (rhs);
   ev->fdel (op);

   return !error;

}

#if 0
static void prstack (xvector_t *st, const char *name)
{
   printf ("[%s]: ");
   for (size_t i=0; i<XVECT_LENGTH (st); i++) {
      printf ("%s,", (char *)(XVECT_INDEX (st, i)));
   }
   printf ("\n");
}
#endif

void *eval_execute (eval_t *ev, const void **tokens)
{
   bool apply_immed = false,
        already_applied = false;
   for (size_t i=0; tokens[i]; i++) {
      switch (ev->ftype (tokens[i])) {
         case eval_UNKNOWN:   return NULL;

         case eval_HIGH_OPS:  push (&ev->st2, ev->fcopy, tokens[i]);
                              apply_immed = true;
                              break;

         case eval_LOW_OPS:   push (&ev->st2, ev->fcopy, tokens[i]);
                              break;

         case eval_OPERAND:   push (&ev->st1, ev->fcopy, tokens[i]);
                              if (apply_immed) {
                                 if (!apply (ev)) {
                                    return NULL;
                                 }
                                 apply_immed = false;
                                 already_applied = true;
                              }
                              break;

         case eval_OPEN:      break;   // Do Nothing!

         case eval_CLOSE:     if (!already_applied && !apply (ev)) {
                                 return NULL;
                              }
                              already_applied = false;
                              break;
      }
   }

   while (length (ev->st1) >= 2 && length (ev->st2) >= 1) {
      if (!apply (ev)) {
         return NULL;
      }
   }

   if (length (ev->st1) != 1 || length (ev->st2) !=0) {
      return NULL;
   }

   return pop (&ev->st1);
}


