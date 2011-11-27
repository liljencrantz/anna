#ifndef ANNA_FUNCTION_H
#define ANNA_FUNCTION_H

#include "anna/base.h"
#include "anna/node.h"
#include "anna/stack.h"

/*
  Various flags used by functions. 
 */

/**
  Set to true for variadic functions.
 */
#define ANNA_FUNCTION_VARIADIC 512
/**
   This function can be used as a macro.
 */
#define ANNA_FUNCTION_MACRO 1024
/**
   The outwardly visible interface of this function, i.e. it's input
   and return types have been calculated.
 */
#define ANNA_FUNCTION_PREPARED_INTERFACE 2048
/**
   The body of this function has been prepared, i.e. the return type
   of every node has been calculated.
 */
#define ANNA_FUNCTION_PREPARED_BODY 4096
/**
   This function is a block-type function. This implies that a return
   expression within this function will return not just this function
   but it's innermost non-block function.
 */
#define ANNA_FUNCTION_BLOCK 8192
/**
   This function is a continuation.
 */
#define ANNA_FUNCTION_CONTINUATION (8192*2)
/**
   This function is a bound method.
 */
#define ANNA_FUNCTION_BOUND_METHOD (8192*4)
/**
   This function is the body of a loop.
 */
#define ANNA_FUNCTION_LOOP (8192*8)

/**
   This function is pure (has no side effects).
*/
#define ANNA_FUNCTION_PURE (8192*16)

/**
   This function has been validated.
*/
#define ANNA_FUNCTION_VALIDATED (8192*32)

extern array_list_t anna_function_list;

/**
   Same as anna_function_unwrap, but is slightly faster but causes
   undefined behavior if the supplied object is not a valid function
   wrapper.
 */
static inline __pure anna_function_t *anna_function_unwrap_fast(anna_object_t *wrapper)
{
    anna_member_t *m = wrapper->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_PAYLOAD];
    return (anna_function_t *)wrapper->member[m->offset];
}


__pure anna_function_t *anna_function_unwrap(anna_object_t *wrapper);

__pure static inline int anna_function_is_variadic(anna_function_t *f)
{
    return !!(f->flags & ANNA_FUNCTION_VARIADIC);
}

__pure anna_object_t *anna_function_wrap(anna_function_t *fun);

__cold int anna_function_prepared(anna_function_t *t);

__cold anna_function_t *anna_native_create(
    wchar_t *name,
    int flags,
    anna_native_t native, 
    anna_type_t *return_type,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn,
    anna_node_t **argd,
    struct anna_stack_template *parent_stack);

__cold anna_function_t *anna_function_create_from_definition(
    struct anna_node_call *definition);

__cold anna_function_t *anna_macro_create(
    wchar_t *name,
    struct anna_node_call *body,
    wchar_t *arg_name);

__cold anna_function_t *anna_function_create_from_block(
    struct anna_node_call *definition);

__cold void anna_function_print(anna_function_t *function);

__cold void anna_function_set_stack(
    anna_function_t *f,
    anna_stack_template_t *parent_stack);

void anna_function_setup_interface(anna_function_t *f);
void anna_function_setup_body(anna_function_t *f);

void anna_function_argument_hint(
    anna_function_t *f,
    int argument,
    anna_type_t *type);

int anna_function_has_alias(anna_function_t *fun, wchar_t *name);
int anna_function_has_alias_reverse(anna_function_t *fun, wchar_t *name);
int anna_function_has_alias_reverse_static(anna_function_t *fun, wchar_t *name, anna_type_t *type);

void anna_function_alias_add(anna_function_t *fun, wchar_t *name);
void anna_function_alias_reverse_add(anna_function_t *fun, wchar_t *name);

void anna_function_document(anna_function_t *fun, wchar_t *documentation);

anna_function_t *anna_continuation_create(
    anna_entry_t **stack_ptr,
    size_t stack_sz,
    anna_activation_frame_t *frame,
    int copy);

anna_function_t *anna_method_bind(
    anna_context_t *stack,
    anna_function_t *method);

int anna_function_line(
    anna_function_t *fun,
    int offset);

#endif
