#ifndef ANNA_VM_H
#define ANNA_VM_H

#include <complex.h>
#include "anna_alloc.h"

double anna_float_get(anna_object_t *this);
wchar_t anna_char_get(anna_object_t *this);
long anna_int_get(anna_object_t *this);
complex double anna_complex_get(anna_object_t *this);
anna_object_t *anna_int_create(long val);
anna_object_t *anna_float_create(double val);
anna_object_t *anna_char_create(wchar_t val);
anna_entry_t *anna_int_entry(anna_object_t *this);

#define ANNA_INT_FAST_MAX 0x1fffffff
#define ANNA_VM_NULL(par) (unlikely(((anna_object_t *)par) == null_object))
#define ANNA_VM_NULLCHECK(par) if(ANNA_VM_NULL(par)) return anna_from_obj(null_object);

/**
   A macro that creates a wrapper function for native calls. This
   makes it a tiny bit less error prone to write your own native
   functions, since you aren't mucking around with stack frames
   directly. However, using this wrapper prevents you from setting up
   callbacks that allow you to call non-native code from inside of
   your native function.
 */
#define ANNA_VM_NATIVE(name,param_count) static anna_vmstack_t *name(	\
	anna_vmstack_t *stack, anna_object_t *me)			\
    {									\
	anna_entry_t *res = name ## _i(stack->top-param_count);	\
	anna_vmstack_drop(stack, param_count+1);			\
	anna_vmstack_push_entry(stack, res);				\
	return stack;							\
    }

#define ANNA_VM_MACRO(name) static anna_vmstack_t *name(		\
	anna_vmstack_t *stack, anna_object_t *me)			\
    {									\
	anna_node_t *res = name ## _i((anna_node_call_t *)anna_node_unwrap(anna_as_obj(*(stack->top-1)))); \
	anna_vmstack_drop(stack, 2);					\
	anna_vmstack_push_object(stack, anna_node_wrap(res)); \
	return stack;							\
    }

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
    res <<= 2;
    res |= ANNA_STACK_ENTRY_INT;
    return (anna_entry_t *)res;
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
	if(type == ANNA_STACK_ENTRY_INT)
	{
	    long res = (long)entry;
	    res >>= 2;
	    return res;
	}
	wprintf(L"Invalid vmstack entry\n");
	CRASH;
    }
    return anna_int_get((anna_object_t *)entry);
}

static inline wchar_t anna_as_char(anna_entry_t *entry)
{
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    if(likely(type))
    {
	type = ((long)entry) & ANNA_STACK_ENTRY_SUBFILTER;
	if(type == ANNA_STACK_ENTRY_CHAR)
	{
	    long res = (long)entry;
	    res >>= 2;
	    return (wchar_t)res;
	}
	wprintf(L"Invalid vmstack entry\n");
	CRASH;
    }
    return anna_char_get((anna_object_t *)entry);
}

static inline double anna_as_float(anna_entry_t *entry)
{
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    if(likely(type))
    {
	if(type == ANNA_STACK_ENTRY_FLOAT)
	{
	    double *res = (double *)((long)entry & ~ANNA_STACK_ENTRY_FILTER);
	    return *res;
	}
	wprintf(L"Invalid vmstack entry %d\n", entry);
	CRASH;
    }
    return anna_float_get((anna_object_t *)entry);
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

static inline anna_entry_t *anna_as_native(anna_object_t *obj)
{
    anna_entry_t *e = anna_from_obj(obj);
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

void anna_bc_print(char *code);

void anna_vm_init(void);
anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_object_t **argv);

//void anna_vm_call_loop(anna_vm_callback_t callback, void *aux, anna_object_t *entry, int argc);

anna_vmstack_t *anna_vm_callback_native(
    anna_vmstack_t *stack, 
    anna_native_t callback, int paramc, anna_entry_t **param,
    anna_object_t *entry, int argc, anna_entry_t **argv);

void anna_vm_callback_reset(
    anna_vmstack_t *stack, 
    anna_object_t *entry, int argc, anna_entry_t **argv);

anna_vmstack_t *anna_vm_stack_get(void);
void anna_vm_mark_code(anna_function_t *f);
void anna_vm_destroy(void);

/**
   This method is the best ever! All method calls on the null object run this
*/
anna_vmstack_t *anna_vm_null_function(anna_vmstack_t *stack, anna_object_t *me);
anna_vmstack_t *anna_vm_method_wrapper(anna_vmstack_t *stack, anna_object_t *me);

static inline void anna_vmstack_push_object(anna_vmstack_t *stack, anna_object_t *val)
{
    *(stack->top++)= (anna_entry_t *)val;
}

static inline void anna_vmstack_push_entry(anna_vmstack_t *stack, anna_entry_t *val)
{
    *(stack->top++)= val;
}

static inline void anna_vmstack_push_int(anna_vmstack_t *stack, int val)
{
    *(stack->top++)= anna_from_int(val);
}

static inline void anna_vmstack_push_char(anna_vmstack_t *stack, wchar_t val)
{
    *(stack->top++)= anna_from_char(val);
}

static inline void anna_vmstack_push_float(anna_vmstack_t *stack, double val)
{
    *(stack->top++)= (anna_entry_t *)anna_from_float(val);
}

static inline anna_object_t *anna_vmstack_pop_object(anna_vmstack_t *stack)
{
    stack->top--;
    return anna_as_obj(*(stack->top));
}

static inline long anna_vmstack_pop_int(anna_vmstack_t *stack)
{
    return anna_as_int(*(--stack->top));
}

static inline void *anna_vmstack_pop_blob(anna_vmstack_t *stack)
{
    return anna_as_blob(*(--stack->top));
}

static inline anna_object_t *anna_vmstack_pop_object_fast(anna_vmstack_t *stack)
{
    return (anna_object_t *)*(--stack->top);
}

static inline anna_entry_t *anna_vmstack_pop_entry(anna_vmstack_t *stack)
{
    return *(--stack->top);
}

__pure static inline anna_object_t *anna_vmstack_peek_object(anna_vmstack_t *stack, size_t off)
{
    return anna_as_obj(*(stack->top-1-off));
}

__pure static inline anna_object_t *anna_vmstack_peek_object_fast(anna_vmstack_t *stack, size_t off)
{
    return (anna_object_t *)*(stack->top-1-off);
}

__pure static inline anna_entry_t *anna_vmstack_peek_entry(anna_vmstack_t *stack, size_t off)
{
    return *(stack->top-1-off);
}

static inline void anna_vmstack_drop(anna_vmstack_t *stack, int count)
{
    stack->top-=count;
}

#endif
