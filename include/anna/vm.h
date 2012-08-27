#ifndef ANNA_VM_H
#define ANNA_VM_H

#include <complex.h>
#include <stdint.h>
#include "anna/alloc.h"
#include "anna/lib/lang/int.h"

double anna_float_get(anna_object_t *this);
wchar_t anna_char_get(anna_object_t *this);
complex double anna_complex_get(anna_object_t *this);
anna_object_t *anna_float_create(double val);
anna_object_t *anna_char_create(wchar_t val);

#define ANNA_ENTRY_NULL_CHECK(par) if(anna_entry_null(par)) return null_entry;

/**
   A macro that creates a wrapper function for native calls. This
   makes it a tiny bit less error prone to write your own native
   functions, since you aren't mucking around with stack frames
   directly. However, using this wrapper prevents you from setting up
   callbacks that allow you to call non-native code from inside of
   your native function.
 */
#define ANNA_VM_NATIVE(name, param_count) \
    static inline anna_entry_t name ## _i(anna_entry_t *param);	\
    static __hot void name(anna_context_t *context) \
    {									\
	anna_entry_t res = name ## _i(context->top-param_count);		\
	anna_context_drop(context, param_count+1);			\
	anna_context_push_entry(context, res);				\
    }									\
    static inline anna_entry_t name ## _i(anna_entry_t *param)

#define ANNA_VM_MACRO(name)						\
    static inline anna_node_t *name ## _i(anna_node_call_t *node);	\
    static __cold void name(anna_context_t *context) \
    {									\
	anna_node_t *res = name ## _i((anna_node_call_t *)anna_node_unwrap(anna_as_obj(*(context->top-1)))); \
	anna_context_drop(context, 2);					\
	anna_context_push_object(context, anna_node_wrap(res));		\
    }									\
    static inline anna_node_t *name ## _i(anna_node_call_t *node)

void anna_vm_compile(
    anna_function_t *fun);

__cold void anna_bc_print(char *code);

__cold void anna_vm_init(void);
__hot anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_entry_t *argv);

//void anna_vm_call_loop(anna_vm_callback_t callback, void *aux, anna_object_t *entry, int argc);

__hot void anna_vm_callback_native(
    anna_context_t *stack, 
    anna_native_t callback, int paramc, anna_entry_t *param,
    anna_object_t *entry, int argc, anna_entry_t *argv);

__hot void anna_vm_callback_reset(
    anna_context_t *stack, 
    anna_object_t *entry, int argc, anna_entry_t *argv);

__hot void anna_vm_callback(
    anna_context_t *parent, 
    anna_object_t *entry, int argc, anna_entry_t *argv);

/**
   This is a convenience wrapper around anna_vm_callback_native useful
   when raising an error. It will basically set up a call to the
   error.raise function with the specified message as the argument. If
   error.raise returns, the return_value value is returned.
 */
__hot void anna_vm_raise(
    anna_context_t *context,
    anna_entry_t message,
    anna_entry_t return_value);

__hot anna_context_t *anna_vm_stack_get(void);
__hot void anna_vm_mark_code(anna_function_t *f);
__cold void anna_vm_destroy(void);

/**
   This method is the best ever! It does nothing and returns a null
   object. All method calls on the null object run this. 
*/
__hot void anna_vm_null_function(anna_context_t *stack);
/**

 */
__hot void anna_vm_fallback_function(anna_context_t *stack);

__hot void anna_vm_method_wrapper(anna_context_t *stack);

static inline void anna_context_push_object(anna_context_t *stack, anna_object_t *val)
{
    *(stack->top++)= anna_from_obj(val);
}

static inline void anna_context_push_entry(anna_context_t *stack, anna_entry_t val)
{
    *(stack->top++)= val;
}

static inline void anna_context_push_int(anna_context_t *stack, int val)
{
    *(stack->top++)= anna_from_int(val);
}

static inline void anna_context_push_char(anna_context_t *stack, wchar_t val)
{
    *(stack->top++)= anna_from_char(val);
}

static inline void anna_context_push_float(anna_context_t *stack, double val)
{
    *(stack->top++)= anna_from_float(val);
}

static inline anna_object_t *anna_context_pop_object(anna_context_t *stack)
{
    stack->top--;
    return anna_as_obj(*(stack->top));
}

static inline long anna_context_pop_int(anna_context_t *stack)
{
    return anna_as_int(*(--stack->top));
}

static inline void *anna_context_pop_blob(anna_context_t *stack)
{
    return anna_as_blob(*(--stack->top));
}

static inline anna_object_t *anna_context_pop_object_fast(anna_context_t *stack)
{
    return anna_as_obj_fast(*(--stack->top));
}

static inline anna_entry_t anna_context_pop_entry(anna_context_t *stack)
{
    return *(--stack->top);
}

__pure static inline anna_object_t *anna_context_peek_object(anna_context_t *stack, size_t off)
{
    return anna_as_obj(*(stack->top-1-off));
}

__pure static inline anna_object_t *anna_context_peek_object_fast(anna_context_t *stack, size_t off)
{
    return anna_as_obj_fast(*(stack->top-1-off));
}

__pure static inline anna_entry_t anna_context_peek_entry(anna_context_t *stack, size_t off)
{
    return *(stack->top-1-off);
}

static inline void anna_context_drop(anna_context_t *stack, int count)
{
    stack->top-=count;
}

#endif
