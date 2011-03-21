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

static void anna_object_print_val(anna_object_t *value)
{    
    	if(value == null_object) 
	{
	    wprintf(L"null");
	}
	else 
	{
	    anna_object_t *o = value;
	    anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	    anna_object_t *str = anna_vm_run(o->type->static_member[tos_mem->offset], 1, &o);
	    if(str->type == string_type)
	    {
		anna_string_print(str);
	    }
	    else
	    {
		wprintf(L"<invalid toString method>");
	    }
	}
}


static anna_object_t *anna_i_print(anna_object_t **param)
{
    int i;
    
    for(i=0; i<anna_list_get_size(param[0]); i++){	
	anna_object_t *value = anna_list_get(param[0], i);
	anna_object_print_val(value);
    }
    return param[0];
}

static anna_object_t *anna_i_not(anna_object_t **param)
{
    return(param[0] == null_object)?anna_int_one:null_object;
}

void anna_function_implementation_init(struct anna_stack_template *stack)
{
    static wchar_t *p_argn[]={L"object"};
    anna_function_t *f = anna_native_create(
	L"print", 
	ANNA_FUNCTION_VARIADIC, 
	(anna_native_t)&anna_i_print, 
	null_type, 1, &object_type, 
	p_argn, stack);
    anna_stack_declare(
	stack,
	L"print",
	f->wrapper->type,
	f->wrapper,
	0);
    
    anna_function_t *not = anna_native_create(L"__not__", 0, (anna_native_t)&anna_i_not, int_type, 1, &object_type, p_argn, stack);
    anna_stack_declare(
	stack,
	L"__not__",
	not->wrapper->type,
	not->wrapper,
	0);
}
