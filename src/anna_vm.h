#ifndef ANNA_VM_H
#define ANNA_VM_H

#include <complex.h>
#include <stdint.h>
#include "anna_alloc.h"
#include "clib/lang/int.h"

double anna_float_get(anna_object_t *this);
wchar_t anna_char_get(anna_object_t *this);
complex double anna_complex_get(anna_object_t *this);
anna_object_t *anna_float_create(double val);
anna_object_t *anna_char_create(wchar_t val);

#define ANNA_INT_FAST_MAX 0x1fffffff
#define ANNA_ENTRY_NULL_CHECK(par) if(anna_entry_null(par)) return null_entry;

/**
   A macro that creates a wrapper function for native calls. This
   makes it a tiny bit less error prone to write your own native
   functions, since you aren't mucking around with stack frames
   directly. However, using this wrapper prevents you from setting up
   callbacks that allow you to call non-native code from inside of
   your native function.
 */
#define ANNA_VM_NATIVE(name,param_count) \
    static inline anna_entry_t *name ## _i(anna_entry_t **param);	\
    static __hot void name(anna_context_t *stack) \
    {									\
	anna_entry_t *res = name ## _i(stack->top-param_count);		\
	anna_context_drop(stack, param_count+1);			\
	anna_context_push_entry(stack, res);				\
    }									\
    static inline anna_entry_t *name ## _i(anna_entry_t **param)

#define ANNA_VM_MACRO(name)						\
    static inline anna_node_t *name ## _i(anna_node_call_t *node);	\
    static __cold void name(anna_context_t *stack) \
    {									\
	anna_node_t *res = name ## _i((anna_node_call_t *)anna_node_unwrap(anna_as_obj(*(stack->top-1)))); \
	anna_context_drop(stack, 2);					\
	anna_context_push_object(stack, anna_node_wrap(res));		\
    }									\
    static inline anna_node_t *name ## _i(anna_node_call_t *node)

#define ANNA_ENTRY_JMP_TABLE static void *jmp_table[] =			\
    {									\
	&&LAB_ENTRY_OBJ, &&LAB_ENTRY_CHAR, &&LAB_ENTRY_BLOB, &&LAB_ENTRY_INT, \
	&&LAB_ENTRY_FLOAT, &&LAB_ENTRY_CHAR, &&LAB_ENTRY_BLOB, &&LAB_ENTRY_INT \
    }

#define ANNA_STACK_ENTRY_FILTER 7l
#define ANNA_STACK_ENTRY_SUBFILTER 3l

#define ANNA_STACK_ENTRY_OBJ 0l
#define ANNA_STACK_ENTRY_CHAR 1l
#define ANNA_STACK_ENTRY_BLOB 2l
#define ANNA_STACK_ENTRY_INT 3l
#define ANNA_STACK_ENTRY_FLOAT 4l

extern anna_function_t *anna_vm_run_fun;

/*  
                  /   \

                /       \

              /           \             

            0               1
 
          /   \           /   \
     
        0       1       0       1
       / \     Blob    Char    Int
      0   1
     Obj Alc
*/

static inline double anna_as_float(anna_entry_t *entry);
static inline anna_entry_t *anna_from_obj(anna_object_t *val);

static inline int anna_entry_null(anna_entry_t *par)
{
    return ((anna_object_t *)par) == null_object;
}

static inline int anna_is_obj(anna_entry_t *val)
{
    long type = ((long)val) & ANNA_STACK_ENTRY_FILTER;
    return type == ANNA_STACK_ENTRY_OBJ;
}

static inline int anna_is_float(anna_entry_t *val)
{
    long type = ((long)val) & ANNA_STACK_ENTRY_FILTER;
    return type == ANNA_STACK_ENTRY_FLOAT;
}

static inline int anna_is_blob(anna_entry_t *val)
{
    long type = ((long)val) & ANNA_STACK_ENTRY_SUBFILTER;
    return type == ANNA_STACK_ENTRY_BLOB;
}

static inline int anna_is_char(anna_entry_t *val)
{
    long type = ((long)val) & ANNA_STACK_ENTRY_SUBFILTER;
    return type == ANNA_STACK_ENTRY_CHAR;
}

static inline int anna_is_int_small(anna_entry_t *val)
{
    long type = ((long)val) & ANNA_STACK_ENTRY_SUBFILTER;
    return type == ANNA_STACK_ENTRY_INT;
}

static inline anna_entry_t *anna_from_int(long res)
{
    if(abs(res) < ANNA_INT_FAST_MAX)
    {
	res <<= 2;
	res |= ANNA_STACK_ENTRY_INT;
	return (anna_entry_t *)res;
    }
    else
    {
	return anna_from_obj(anna_int_create(res));
    }    
}

static inline anna_entry_t *anna_from_uint64(uint64_t val)
{
    mpz_t mp;
    mpz_init(mp);
    anna_mpz_set_ui64(mp, val);
    
    anna_entry_t *res = anna_from_obj(anna_int_create_mp(mp));
    mpz_clear(mp);
    return res;
    
}

static inline anna_entry_t *anna_from_float(double val)
{
    double *res = anna_alloc_blob(sizeof(double));
    *res = val;
    res  = (double *)((long)res | ANNA_STACK_ENTRY_FLOAT);
    return (anna_entry_t *)res;
}

static inline anna_entry_t *anna_from_blob(void *val)
{
    return (anna_entry_t *)((long)val | ANNA_STACK_ENTRY_BLOB);
}
/*
static inline anna_entry_t *anna_from_alloc(void *val)
{
    return (anna_entry_t *)((long)val | ANNA_STACK_ENTRY_ALLOC);
}
*/
static inline anna_entry_t *anna_from_char(wchar_t val)
{
    long res = (long)val;
    res <<= 2;
    res |= ANNA_STACK_ENTRY_CHAR;
    return (anna_entry_t *)res;
}

static inline anna_entry_t *anna_from_obj(anna_object_t *val)
{
/*
    if(((long)val) & ANNA_STACK_ENTRY_FILTER == 0)
    {
	wprintf(L"OOOPS %ls %d\n", val->type->name, val);
	CRASH;
    }
*/  
    return (anna_entry_t *)val;
}

static inline anna_object_t *anna_as_obj(anna_entry_t *entry)
{
    ANNA_ENTRY_JMP_TABLE;
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    goto *jmp_table[type];
  LAB_ENTRY_OBJ:
    return (anna_object_t *)entry;
  LAB_ENTRY_CHAR:
    {
	long res = (long)entry;
	res >>= 2;
	return anna_char_create((int)res);
    }
  LAB_ENTRY_FLOAT:
    {
	double *res = (double *)((long)entry & ~ANNA_STACK_ENTRY_FILTER);
	return anna_float_create(*res);
    }
  LAB_ENTRY_INT:
    {
	long res = (long)entry;
	res >>= 2;
	return anna_int_create(res);
    }
  LAB_ENTRY_BLOB:
    CRASH;
}

static inline anna_object_t *anna_as_obj_fast(anna_entry_t *entry)
{
    return (anna_object_t *)entry;
}


static inline long anna_as_int(anna_entry_t *entry)
{
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    if(likely(type))
    {
	type = ((long)entry) & ANNA_STACK_ENTRY_SUBFILTER;
#ifdef ANNA_CHECK_VM
	if(type != ANNA_STACK_ENTRY_INT)
	{
  	    wprintf(L"Invalid vmstack entry\n");
	    CRASH;
	}
#endif	
	long res = (long)entry;
	res >>= 2;
	return res;
    }
    return anna_int_get((anna_object_t *)entry);
}

static inline long anna_as_int_unsafe(anna_entry_t *entry)
{
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    type = ((long)entry) & ANNA_STACK_ENTRY_SUBFILTER;
    long res = (long)entry;
    res >>= 2;
    return res;
}

static inline uint64_t anna_as_uint64(anna_entry_t *entry)
{
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    if(likely(type))
    {
	type = ((long)entry) & ANNA_STACK_ENTRY_SUBFILTER;
#ifdef ANNA_CHECK_VM
	if(type != ANNA_STACK_ENTRY_INT)
	{
	    wprintf(L"Invalid vmstack entry\n");
	    CRASH;
	}
#endif
	long res = (long)entry;
	res >>= 2;
	return res;
    }
    mpz_t *mp = anna_int_unwrap((anna_object_t *)entry);
    return anna_mpz_get_ui64(*mp);
}

static inline wchar_t anna_as_char(anna_entry_t *entry)
{
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    if(likely(type))
    {
	type = ((long)entry) & ANNA_STACK_ENTRY_SUBFILTER;
#ifdef ANNA_CHECK_VM
	if(type != ANNA_STACK_ENTRY_CHAR)
	{
	    wprintf(L"Invalid vmstack entry\n");
	    CRASH;
	}
#endif
	long res = (long)entry;
	res >>= 2;
	return (wchar_t)res;
    }
    return anna_char_get((anna_object_t *)entry);
}

static inline double anna_as_float(anna_entry_t *entry)
{
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    if(likely(type))
    {
#ifdef ANNA_CHECK_VM
	if(type != ANNA_STACK_ENTRY_FLOAT)
	{
	    wprintf(L"Invalid vmstack entry %d\n", entry);
	    CRASH;
	}
#endif
	double *res = (double *)((long)entry & ~ANNA_STACK_ENTRY_FILTER);
	return *res;
    }
    return anna_float_get((anna_object_t *)entry);
}

static inline double anna_as_float_unsafe(anna_entry_t *entry)
{
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    return *(double *)((long)entry & ~ANNA_STACK_ENTRY_FILTER);
}

static inline void *anna_as_blob(anna_entry_t *entry)
{
    return (void *)(((long)entry) & ~ANNA_STACK_ENTRY_SUBFILTER);
}

static inline void *anna_as_float_payload(anna_entry_t *entry)
{
    return (void *)(((long)entry) & ~ANNA_STACK_ENTRY_FILTER);
}

static inline complex double anna_as_complex(anna_entry_t *entry)
{
    return anna_complex_get((anna_object_t *)entry);
}

static inline anna_entry_t *anna_as_native(anna_entry_t *e)
{
    anna_object_t *obj = anna_as_obj(e);
    if(obj->type == int_type)
    {
	return anna_int_entry(obj);
    }
    else if(obj->type == float_type)
    {
	return anna_from_float(anna_as_float(e));
    }
    else if(obj->type == char_type)
    {
	return anna_from_char(anna_as_char(e));
    }
    return e;
}

void anna_vm_compile(
    anna_function_t *fun);

__cold void anna_bc_print(char *code);

__cold void anna_vm_init(void);
__hot anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_object_t **argv);

//void anna_vm_call_loop(anna_vm_callback_t callback, void *aux, anna_object_t *entry, int argc);

__hot void anna_vm_callback_native(
    anna_context_t *stack, 
    anna_native_t callback, int paramc, anna_entry_t **param,
    anna_object_t *entry, int argc, anna_entry_t **argv);

__hot void anna_vm_callback_reset(
    anna_context_t *stack, 
    anna_object_t *entry, int argc, anna_entry_t **argv);

__hot void anna_vm_callback(
    anna_context_t *parent, 
    anna_object_t *entry, int argc, anna_entry_t **argv);

__hot anna_context_t *anna_vm_stack_get(void);
__hot void anna_vm_mark_code(anna_function_t *f);
__cold void anna_vm_destroy(void);

/**
   This method is the best ever! All method calls on the null object run this
*/
__hot void anna_vm_null_function(anna_context_t *stack);
__hot void anna_vm_method_wrapper(anna_context_t *stack);

static inline void anna_context_push_object(anna_context_t *stack, anna_object_t *val)
{
    *(stack->top++)= (anna_entry_t *)val;
}

static inline void anna_context_push_entry(anna_context_t *stack, anna_entry_t *val)
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
    *(stack->top++)= (anna_entry_t *)anna_from_float(val);
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
    return (anna_object_t *)*(--stack->top);
}

static inline anna_entry_t *anna_context_pop_entry(anna_context_t *stack)
{
    return *(--stack->top);
}

__pure static inline anna_object_t *anna_context_peek_object(anna_context_t *stack, size_t off)
{
    return anna_as_obj(*(stack->top-1-off));
}

__pure static inline anna_object_t *anna_context_peek_object_fast(anna_context_t *stack, size_t off)
{
    return (anna_object_t *)*(stack->top-1-off);
}

__pure static inline anna_entry_t *anna_context_peek_entry(anna_context_t *stack, size_t off)
{
    return *(stack->top-1-off);
}

static inline void anna_context_drop(anna_context_t *stack, int count)
{
    stack->top-=count;
}

#endif
