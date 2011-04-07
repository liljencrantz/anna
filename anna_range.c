#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_range.h"
#include "anna_stack.h"
#include "anna_int.h"
#include "anna_member.h"
#include "anna_function_type.h"
#include "anna_function.h"
#include "anna_vm.h"

ssize_t anna_range_get_from(anna_object_t *obj)
{
    return *(ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_FROM);    
}

ssize_t anna_range_get_to(anna_object_t *obj)
{
    return *(ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_TO);
}

ssize_t anna_range_get_step(anna_object_t *obj)
{
    return *(ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_STEP);    
}

int anna_range_get_open(anna_object_t *obj)
{
    return *(int *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_OPEN);    
}

void anna_range_set_from(anna_object_t *obj, ssize_t v)
{
    *((ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_FROM)) = v;
}

void anna_range_set_to(anna_object_t *obj, ssize_t v)
{
    *((ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_TO)) = v;
}

void anna_range_set_step(anna_object_t *obj, ssize_t v)
{
    if(v != 0)
	*((ssize_t *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_STEP)) = v;
}

void anna_range_set_open(anna_object_t *obj, int v)
{
    *((int *)anna_member_addr_get_mid(obj,ANNA_MID_RANGE_OPEN)) = v;
}

static anna_vmstack_t *anna_range_get_from_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *r = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    anna_vmstack_push(stack, anna_int_create(anna_range_get_from(r)));
    return stack;
}

static anna_vmstack_t *anna_range_get_to_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *r = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    anna_vmstack_push(stack, anna_int_create(anna_range_get_to(r)));
    return stack;
}

static anna_vmstack_t *anna_range_get_step_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *r = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    anna_vmstack_push(stack, anna_int_create(anna_range_get_step(r)));
    return stack;
}

static anna_vmstack_t *anna_range_get_open_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *r = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    anna_vmstack_push(stack, anna_int_create(anna_range_get_open(r)));
    return stack;
}

static anna_vmstack_t *anna_range_set_from_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_range_set_from(param[0], anna_int_get(param[1]));
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, param[1]);
    return stack;    
}

static anna_vmstack_t *anna_range_set_to_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_range_set_to(param[0], anna_int_get(param[1]));
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, param[1]);
    return stack;    
}

static anna_vmstack_t *anna_range_set_step_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_range_set_step(param[0], anna_int_get(param[1]));
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, param[1]);
    return stack;    
}

static anna_vmstack_t *anna_range_set_open_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_range_set_open(param[0], param[1] != null_object);
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, param[1]);
    return stack;    
}

static anna_vmstack_t *anna_range_init(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **obj = stack->top - 5;

    ssize_t from = (obj[1] == null_object)?0:anna_int_get(obj[1]);
    anna_range_set_from(obj[0], from);
    ssize_t to = (obj[2] == null_object)?0:anna_int_get(obj[2]);
    anna_range_set_to(obj[0], to);
    int open = (obj[2]==null_object);
    open |= (obj[4] != null_object);
    anna_range_set_open(obj[0], open);

    if(obj[3] == null_object)
    {
	anna_range_set_step(obj[0], ((to>from)|| open)?1:-1);
    }
    else
    {
	ssize_t step = anna_int_get(obj[3]);
	anna_range_set_step(obj[0], step != 0 ? step:1);
    }
    anna_vmstack_drop(stack, 6);
    anna_vmstack_push(stack, obj[0]);
    return stack;    
}

static anna_vmstack_t *anna_range_get_int(anna_vmstack_t *stack, anna_object_t *me)
{  
    anna_object_t **param = stack->top - 2;
    anna_object_t *obj = null_object;
    ssize_t from = anna_range_get_from(param[0]);
    ssize_t to = anna_range_get_to(param[0]);
    ssize_t step = anna_range_get_step(param[0]);
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

static inline anna_object_t *anna_range_in_i(anna_object_t **param)
{
    ssize_t from = anna_range_get_from(param[0]);
    ssize_t to = anna_range_get_to(param[0]);
    ssize_t step = anna_range_get_step(param[0]);
    int open = anna_range_get_open(param[0]);
    if(param[1]->type == null_type)
	return null_object;
    int val = anna_int_get(param[1]);
    if(step > 0)
    {
	if(val < from)
	{
	    return null_object;
	}
	if((val >= to) && !open)
	{
	    return null_object;
	}
	int rem = (val-from)%step;
	int res = (val-from)/step;
	return (rem == 0)?anna_int_create(res):null_object;
    }
    else
    {
	if(val > from)
	{
	    return null_object;
	}
	if((val <= to) && !open)
	{
	    return null_object;
	}
	int rem = (val-from)%step;
	int res = (val-from)/step;
	return (rem == 0)?anna_int_create(res):null_object;
    }
}
ANNA_VM_NATIVE(anna_range_in, 2)

static int anna_range_is_valid(anna_object_t *obj)
{
    ssize_t from = anna_range_get_from(obj);
    ssize_t to = anna_range_get_to(obj);
    ssize_t step = anna_range_get_step(obj);
    return (to>from)==(step>0);
}

ssize_t anna_range_get_count(anna_object_t *obj)
{
    ssize_t from = anna_range_get_from(obj);
    ssize_t to = anna_range_get_to(obj);
    ssize_t step = anna_range_get_step(obj);
    return (1+(to-from-sign(step))/step);
}

static anna_vmstack_t *anna_range_get_count_i(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push(stack, anna_range_get_open(param[0])? null_object:(anna_range_is_valid(*param)?anna_int_create(anna_range_get_count(*param)):null_object));
    return stack;    
}

/**
   This is the bulk of the each method
 */
static anna_vmstack_t *anna_range_each_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    // Discard the output of the previous method call
    anna_vmstack_pop(stack);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_object_t **param = stack->top - 3;
    // Unwrap and name the params to make things more explicit
    anna_object_t *range = param[0];
    anna_object_t *body = param[1];
    int idx = anna_int_get(param[2]);

    ssize_t from = anna_range_get_from(param[0]);
    ssize_t to = anna_range_get_to(param[0]);
    ssize_t step = anna_range_get_step(param[0]);
    ssize_t count = 1+(to-from-sign(step))/step;
    int open = anna_range_get_open(param[0]);
    
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
	anna_vmstack_push(stack, range);
    }
    return stack;
}

static anna_vmstack_t *anna_range_each(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *body = anna_vmstack_pop(stack);
    anna_object_t *range = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    
    ssize_t from = anna_range_get_from(range);
    ssize_t to = anna_range_get_to(range);
    ssize_t step = anna_range_get_step(range);
    int open = anna_range_get_open(range);
    
    if(((to>from) != (step>0)) && !open)
    {
	anna_vmstack_push(stack, range);
    }
    else
    {
	anna_object_t *callback_param[] = 
	    {
		range,
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
	    anna_range_each_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }    
    return stack;
}

void anna_range_type_create(struct anna_stack_template *stack)
{
    mid_t mmid;
    anna_function_t *fun;

    anna_member_create(
	range_type, ANNA_MID_RANGE_FROM,  L"!rangeFrom", 
	0, null_type);
    anna_member_create(
	range_type, ANNA_MID_RANGE_TO,  L"!rangeTo", 
	0, null_type);
    anna_member_create(
	range_type, ANNA_MID_RANGE_STEP,  L"!rangeStep", 
	0, null_type);
    anna_member_create(
	range_type, ANNA_MID_RANGE_OPEN,  L"!rangeOpen", 
	0, null_type);    
    
    anna_type_t *c_argv[] = 
	{
	    range_type,
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
	range_type,
	-1,
	L"__init__", 0,
	&anna_range_init, 
	range_type,
	5, c_argv, c_argn);


    anna_type_t *i_argv[] = 
	{
	    range_type,
	    int_type
	}
    ;

    wchar_t *i_argn[]=
	{
	    L"this", L"index"
	}
    ;

    mmid = anna_native_method_create(
	range_type,
	-1,
	L"__get__Int__",
	0, 
	&anna_range_get_int, 
	int_type,
	2, 
	i_argv, 
	i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(range_type, mmid));
    anna_function_alias_add(fun, L"__get__");
    
    anna_native_property_create(
	range_type,
	-1,
	L"count",
	int_type,
	&anna_range_get_count_i, 
	0);

    anna_native_property_create(
	range_type,
	-1,
	L"from",
	int_type,
	&anna_range_get_from_i, 
	&anna_range_set_from_i);

    anna_native_property_create(
	range_type,
	-1,
	L"to",
	int_type,
	&anna_range_get_to_i, 
	&anna_range_set_to_i);

    anna_native_property_create(
	range_type,
	-1,
	L"step",
	int_type,
	&anna_range_get_step_i, 
	&anna_range_set_step_i);

    anna_native_property_create(
	range_type,
	-1,
	L"isOpen",
	int_type,
	&anna_range_get_open_i, 
	&anna_range_set_open_i);

    anna_type_t *fun_type = anna_function_type_each_create(
	L"!RangeIterFunction", int_type, int_type);

    anna_type_t *e_argv[] = 
	{
	    range_type,
	    fun_type
	}
    ;
    
    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;    
    
    anna_type_t *a_argv[] = 
	{
	    range_type,
	    int_type
	}
    ;
    
    wchar_t *a_argn[]=
	{
	    L"this", L"value"
	}
    ;

    anna_native_method_create(
	range_type, -1, L"__each__", 0, 
	&anna_range_each, 
	range_type,
	2, e_argv, e_argn);

    anna_native_method_create(
	range_type, -1, L"__in__", 0, 
	&anna_range_in, 
	int_type,
	2, a_argv, a_argn);
        

    /*
    anna_native_method_create(
	type, -1, L"__filter__", 
	0, &anna_list_filter, 
	type,
	2, e_argv, e_argn);

    anna_native_method_create(
	type, -1, L"__first__", 
	0, &anna_list_first, 
	spec,
	2, e_argv, e_argn);  

    anna_native_method_create(
	type, -1, L"__map__", 
	0, &anna_list_map, 
	list_type,
	2, e_argv, e_argn);
    
    anna_native_method_create(
	type, -1, L"__in__", 0, 
	&anna_list_in, 
	spec,
	2, a_argv, a_argn);
    */

}
