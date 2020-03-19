
#ifndef H_EVAL
#define H_EVAL


typedef enum {
   eval_UNKNOWN = 0,
   eval_HIGH_OPS,
   eval_LOW_OPS,
   eval_OPERAND,
   eval_OPEN,
   eval_CLOSE
} eval_type_t;

// Copy and delete tokens
typedef void * (eval_copy_t) (const void *);
typedef void (eval_del_t) (void *);

// Execute a "lhs OP rhs" evaluation
typedef void * (eval_run_op_t) (void const *, void const *, void const *);

// Return the type of the token
typedef eval_type_t (eval_typefunc_t) (void const *);

typedef struct eval_t eval_t;

#ifdef __cplusplus
extern "C" {
#endif

   eval_t *eval_new (eval_copy_t *copy_func, eval_del_t *del_func,
                     eval_run_op_t *run_op, eval_typefunc_t *type);
   void  eval_del (eval_t *ev);
   void eval_clear (eval_t *ev);

   void *eval_execute (eval_t *ev, const void **tokens);

#ifdef __cplusplus
};
#endif

#endif

