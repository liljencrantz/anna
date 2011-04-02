#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_continuation.h"
#include "anna_stack.h"
#include "anna_int.h"
#include "anna_member.h"
#include "anna_function_type.h"
#include "anna_function.h"
#include "anna_vm.h"

ssize_t anna_continuation_get_from(anna_object_t *obj)
{
    return *(ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_CONTINUATION_FROM);    
}

ssize_t anna_continuation_get_to(anna_object_t *obj)
{
    return *(ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_CONTINUATION_TO);
}

ssize_t anna_continuation_get_step(anna_object_t *obj)
{
    return *(ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_CONTINUATION_STEP);    
}

int anna_continuation_get_open(anna_object_t *obj)
{
    return *(int *)anna_member_addr_get_mid(obj,ANNA_MID_CONTINUATION_OPEN);    
}

void anna_continuation_set_from(anna_object_t *obj, ssize_t v)
{
    *((ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_CONTINUATION_FROM)) = v;
}

void anna_continuation_set_to(anna_object_t *obj, ssize_t v)
{
    *((ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_CONTINUATION_TO)) = v;
}

void anna_continuation_set_step(anna_object_t *obj, ssize_t v)
{
    if(v != 0)
	*((ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_CONTINUATION_STEP)) = v;
}

void anna_continuation_set_open(anna_object_t *obj, int v)
{
    *((int *)anna_member_addr_get_mid(obj,ANNA_MID_CONTINUATION_OPEN)) = v;
}

static anna_vmstack_t *anna_continuation_get_from_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *r = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    anna_vmstack_push(stack, anna_int_create(anna_continuation_get_from(r)));
    return stack;
}

static anna_vmstack_t *anna_continuation_get_to_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *r = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    anna_vmstack_push(stack, anna_int_create(anna_continuation_get_to(r)));
    return stack;
}

static anna_vmstack_t *anna_continuation_get_step_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *r = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    anna_vmstack_push(stack, anna_int_create(anna_continuation_get_step(r)));
    return stack;
}

static anna_vmstack_t *anna_continuation_get_open_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *r = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    anna_vmstack_push(stack, anna_int_create(anna_continuation_get_open(r)));
    return stack;
}

static anna_vmstack_t *anna_continuation_set_from_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_continuation_set_from(param[0], anna_int_get(param[1]));
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, param[1]);
    return stack;    
}

static anna_vmstack_t *anna_continuation_set_to_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_continuation_set_to(param[0], anna_int_get(param[1]));
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, param[1]);
    return stack;    
}

static anna_vmstack_t *anna_continuation_set_step_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_continuation_set_step(param[0], anna_int_get(param[1]));
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, param[1]);
    return stack;    
}

static anna_vmstack_t *anna_continuation_set_open_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_continuation_set_open(param[0], param[1] != null_object);
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, param[1]);
    return stack;    
}

static anna_vmstack_t *anna_continuation_init(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **obj = stack->top - 5;

    ssize_t from = (obj[1] == null_object)?0:anna_int_get(obj[1]);
    anna_continuation_set_from(obj[0], from);
    ssize_t to = (obj[2] == null_object)?0:anna_int_get(obj[2]);
    anna_continuation_set_to(obj[0], to);
    int open = (obj[2]==null_object);
    open |= (obj[4] != null_object);
    anna_continuation_set_open(obj[0], open);

    if(obj[3] == null_object)
    {
	anna_continuation_set_step(obj[0], ((to>from)|| open)?1:-1);
    }
    else
    {
	ssize_t step = anna_int_get(obj[3]);
	anna_continuation_set_step(obj[0], step != 0 ? step:1);
    }
    anna_vmstack_drop(stack, 6);
    anna_vmstack_push(stack, obj[0]);
    return stack;    
}

static anna_vmstack_t *anna_continuation_get_int(anna_vmstack_t *stack, anna_object_t *me)
{  
    anna_object_t **param = stack->top - 2;
    anna_object_t *obj = null_object;
    ssize_t from = anna_continuation_get_from(param[0]);
    ssize_t to = anna_continuation_get_to(param[0]);
    ssize_t step = anna_continuation_get_step(param[0]);
    ssize_t idx = anna_int_get(param[1]);
    if(likely(idx >= 0)){
	ssize_t res = from + step*idx;
	if(likely(!(step>0 && res >= to)))
	{
	    if(likely(!(step<0 && res <= to)))
	    {
		obj = anna_int_create(res);
	    }
	}
    }
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, obj);
    return stack;    
}

static int anna_continuation_is_valid(anna_object_t *obj)
{
    ssize_t from = anna_continuation_get_from(obj);
    ssize_t to = anna_continuation_get_to(obj);
    ssize_t step = anna_continuation_get_step(obj);
    return (to>from)==(step>0);
}

ssize_t anna_continuation_get_count(anna_object_t *obj)
{
    ssize_t from = anna_continuation_get_from(obj);
    ssize_t to = anna_continuation_get_to(obj);
    ssize_t step = anna_continuation_get_step(obj);
    return (1+(to-from-sign(step))/step);
}

static anna_vmstack_t *anna_continuation_get_count_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push(stack, anna_continuation_get_open(param[0])? null_object:(anna_continuation_is_valid(*param)?anna_int_create(anna_continuation_get_count(*param)):null_object));
    return stack;    
}

/**
   This is the bulk of the each method
 */
static anna_vmstack_t *anna_continuation_each_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    // Discard the output of the previous method call
    anna_vmstack_pop(stack);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_object_t **param = stack->top - 3;
    // Unwrap and name the params to make things more explicit
    anna_object_t *continuation = param[0];
    anna_object_t *body = param[1];
    int idx = anna_int_get(param[2]);

    ssize_t from = anna_continuation_get_from(param[0]);
    ssize_t to = anna_continuation_get_to(param[0]);
    ssize_t step = anna_continuation_get_step(param[0]);
    ssize_t count = 1+(to-from-sign(step))/step;
    int open = anna_continuation_get_open(param[0]);
    
    // Are we done or do we need another lap?
    if(idx < count || open)
    {
	// Set up params for the next lap of the each body function
	anna_object_t *o_param[] =
	    {
		param[2],
		anna_int_create(from + step*idx)
	    }
	;
	// Then update our internal lap counter
	param[2] = anna_int_create(idx+1);
	
	// Finally, roll the code point back a bit and push new arguments
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	// Oops, we're done. Drop our internal param list and push the correct output
	anna_vmstack_drop(stack, 4);
	anna_vmstack_push(stack, continuation);
    }
    return stack;
}

static anna_vmstack_t *anna_continuation_each(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *body = anna_vmstack_pop(stack);
    anna_object_t *continuation = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    
    ssize_t from = anna_continuation_get_from(continuation);
    ssize_t to = anna_continuation_get_to(continuation);
    ssize_t step = anna_continuation_get_step(continuation);
    int open = anna_continuation_get_open(continuation);
    
    if(((to>from) != (step>0)) && !open)
    {
	anna_vmstack_push(stack, continuation);
    }
    else
    {
	anna_object_t *callback_param[] = 
	    {
		continuation,
		body,
		anna_int_one
	    }
	;
	
	anna_object_t *o_param[] =
	    {
		anna_int_zero,
		anna_int_create(from)
	    }
	;
	
	stack = anna_vm_callback_native(
	    stack,
	    anna_continuation_each_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }    
    return stack;
}

void anna_continuation_type_create(struct anna_stack_template *stack)
{
    mid_t mmid;
    anna_function_t *fun;

    anna_member_create(
	continuation_type, ANNA_MID_CONTINUATION_STACK,  L"!continuationStack", 
	ANNA_MEMBER_ALLOC, null_type);
    anna_member_create(
	continuation_type, ANNA_MID_CONTINUATION_OFFSET,  L"!continuationOffset", 
	0, null_type);
    
    anna_type_t *c_argv[] = 
	{
	    continuation_type,
	    int_type,
	    int_type,
	    int_type,
	    object_type
	}
    ;
    
    wchar_t *c_argn[]=
	{
	    L"this", L"from", L"to", L"step", L"isOpen"
	}
    ;

    anna_native_method_create(
	continuation_type,
	-1,
	L"__init__", 0,
	&anna_continuation_init, 
	continuation_type,
	5, c_argv, c_argn);


    anna_type_t *i_argv[] = 
	{
	    continuation_type,
	    int_type
	}
    ;

    wchar_t *i_argn[]=
	{
	    L"this", L"index"
	}
    ;

    mmid = anna_native_method_create(
	continuation_type,
	-1,
	L"__get__Int__",
	0, 
	&anna_continuation_get_int, 
	int_type,
	2, 
	i_argv, 
	i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(continuation_type, mmid));
    anna_function_alias_add(fun, L"__get__");
    
    anna_native_property_create(
	continuation_type,
	-1,
	L"count",
	int_type,
	&anna_continuation_get_count_i, 
	0);

    anna_native_property_create(
	continuation_type,
	-1,
	L"from",
	int_type,
	&anna_continuation_get_from_i, 
	&anna_continuation_set_from_i);

    anna_native_property_create(
	continuation_type,
	-1,
	L"to",
	int_type,
	&anna_continuation_get_to_i, 
	&anna_continuation_set_to_i);

    anna_native_property_create(
	continuation_type,
	-1,
	L"step",
	int_type,
	&anna_continuation_get_step_i, 
	&anna_continuation_set_step_i);

    anna_native_property_create(
	continuation_type,
	-1,
	L"isOpen",
	int_type,
	&anna_continuation_get_open_i, 
	&anna_continuation_set_open_i);

    anna_type_t *fun_type = anna_function_type_each_create(
	L"!ContinuationIterFunction", int_type, int_type);

    anna_type_t *e_argv[] = 
	{
	    continuation_type,
	    fun_type
	}
    ;
    
    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;    
    
    anna_native_method_create(
	continuation_type, -1, L"__each__", 0, 
	&anna_continuation_each, 
	continuation_type,
	2, e_argv, e_argn);

}
