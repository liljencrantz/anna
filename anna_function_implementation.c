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

static anna_object_t *anna_i_print(anna_object_t **param)
{
    int i;
    //    for(i=0; i<node->child_count; i++) {
    anna_object_t *value = param[0];
	if(value->type == int_type) {
	    int val = anna_int_get(value);
	    wprintf(L"%d", val);
	}
	else if(value->type == float_type) {
	    double val = anna_float_get(value);
	    wprintf(L"%f", val);
	}
	else if(value->type == string_type) {
	    wchar_t *payload = anna_string_get_payload(value);
	    size_t payload_size = anna_string_get_payload_size(value);
	    wprintf(L"%.*ls", payload_size, payload);
	}
	else if(value->type == char_type) {
	    wchar_t payload = anna_char_get(value);
	    wprintf(L"%lc", payload);
	}
	else if(value == null_object) {
	    wprintf(L"null");
	}
	else 
	{
	    wprintf(L"%ls", value->type->name);
	}
	/*
	  FIXME: Print other things than just ints!
	*/
	//    }
    return null_object;
}

static anna_object_t *anna_i_not(anna_object_t **param)
{
    return(param[0] == null_object)?anna_int_one:null_object;
}

static anna_object_t *anna_i_if(anna_object_t **param)
{
    anna_object_t *body_object;
    if(param[0]!=null_object)
    {
	body_object=param[1];
    }
    else
    {
	body_object=param[2];
    }
    
    return anna_function_wrapped_invoke(body_object, 0, 0, 0);
}

void anna_function_implementation_init(struct anna_stack_frame *stack)
{
    static wchar_t *p_argn[]={L"object"};
    anna_native_declare(stack, L"print", ANNA_FUNCTION_FUNCTION, (anna_native_t)&anna_i_print, null_type, 1, &object_type, p_argn);
    
    anna_native_declare(stack, L"__not__", ANNA_FUNCTION_FUNCTION, (anna_native_t)&anna_i_not, int_type, 1, &object_type, p_argn);
    
    anna_type_t *if_argv[]={object_type, object_type, object_type};
    static wchar_t *if_argn[]={L"condition", L"trueBlock", L"falseBlock"};    
    anna_native_declare(stack, L"__if__", ANNA_FUNCTION_FUNCTION, (anna_native_t)&anna_i_if, object_type, 3, if_argv, if_argn);
    
}