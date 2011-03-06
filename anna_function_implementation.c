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

static void anna_object_print_val(anna_object_t *value)
{    
	if(value->type == int_type) 
	{
	    int val = anna_int_get(value);
	    wprintf(L"%d", val);
	}
	else if(value->type == float_type) 
	{
	    double val = anna_float_get(value);
	    wprintf(L"%f", val);
	}
	else if(value->type == string_type) 
	{
	    anna_string_print(value);
	}
	else if(value->type == char_type) 
	{
	    wchar_t payload = anna_char_get(value);
	    wprintf(L"%lc", payload);
	}
	else if(value == null_object) 
	{
	    wprintf(L"null");
	}
	else 
	{
	    /*
	      FIXME: Print using a toString method
	    */
	    wprintf(L"Object of type %ls", value->type->name);
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
