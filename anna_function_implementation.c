#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "anna.h"
#include "anna_float.h"
#include "anna_string.h"
#include "anna_int.h"
#include "anna_char.h"
#include "anna_list.h"
#include "anna_function.h"
#include "anna_function_type.h"
#include "anna_vm.h"
#include "anna_member.h"

static anna_vmstack_t *anna_print_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    anna_object_t *value = anna_vmstack_pop(stack);
    anna_object_t **param = stack->top - 2;
    anna_object_t *list = param[0];
    int idx = anna_int_get(param[1]);

    if(value == null_object) 
    {
	wprintf(L"null");
    }
    else 
    {
	if(value->type == string_type)
	{
	    anna_string_print(value);
	}
	else
	{
	    wprintf(L"<invalid toString method>");
	}
    }    
    
    if(anna_list_get_size(param[0]) > idx)
    {
	param[1] = anna_int_create(idx+1);	
	anna_object_t *o = anna_list_get(list, idx);
	anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	anna_object_t *meth = o->type->static_member[tos_mem->offset];
	anna_vm_callback_reset(stack, meth, 1, &o);
    }
    else
    {
	anna_vmstack_drop(stack, 3);
	anna_vmstack_push(stack, list);
    }
    
    return stack;
}


static anna_vmstack_t *anna_i_print(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *list = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    if(anna_list_get_size(list))
    {
	anna_object_t *callback_param[] = 
	    {
		list,
		anna_int_one
	    }
	;
	
	anna_object_t *o = anna_list_get(list, 0);
	anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	anna_object_t *meth = o->type->static_member[tos_mem->offset];
	//wprintf(L"Get toString method of type %ls\n", o->type->name);
	
	stack = anna_vm_callback_native(
	    stack,
	    anna_print_callback, 2, callback_param,
	    meth, 1, &o
	    );
    }
    else
    {
	anna_vmstack_push(stack, list);
    }
    return stack;
}

static anna_vmstack_t *anna_i_not(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *val = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    anna_vmstack_push(stack, (val == null_object)?anna_int_one:null_object);
    return stack;
}

void anna_function_implementation_init(struct anna_stack_template *stack)
{
    static wchar_t *p_argn[]={L"object"};
    anna_function_t *f = anna_native_create(
	L"print", 
	ANNA_FUNCTION_VARIADIC, 
	(anna_native_t)&anna_i_print, 
	list_type, 1, &object_type, 
	p_argn, stack);

    anna_stack_declare(
	stack,
	L"print",
	f->wrapper->type,
	f->wrapper,
	0);
    
    anna_function_t *not = anna_native_create(
	L"__not__", 0, 
	(anna_native_t)&anna_i_not, 
	int_type, 
	1, &object_type, p_argn, stack);
    anna_stack_declare(
	stack,
	L"__not__",
	not->wrapper->type,
	not->wrapper,
	0);
}
