#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "duck.h"
#include "duck_float.h"
#include "duck_string.h"
#include "duck_int.h"
#include "duck_char.h"

static duck_object_t *duck_i_print(duck_object_t **param)
{
    int i;
    //    for(i=0; i<node->child_count; i++) {
    duck_object_t *value = param[0];
	if(value->type == int_type) {
	    int val = duck_int_get(value);
	    wprintf(L"%d", val);
	}
	else if(value->type == float_type) {
	    double val = duck_float_get(value);
	    wprintf(L"%f", val);
	}
	else if(value->type == string_type) {
	    wchar_t *payload = duck_string_get_payload(value);
	    size_t payload_size = duck_string_get_payload_size(value);
	    wprintf(L"%.*ls", payload_size, payload);
	}
	else if(value->type == char_type) {
	    wchar_t payload = duck_char_get(value);
	    wprintf(L"%lc", payload);
	}
	else if(value == null_object) {
	    wprintf(L"null");
	}
	else 
	{
	    wprintf(L"WAAAH\n");
	}
	/*
	  FIXME: Print other things than just ints!
	*/
	//    }
    return null_object;
}

static duck_object_t *duck_i_not(duck_object_t **param)
{
    return(param[0] == null_object)?duck_int_one:null_object;
}

static duck_object_t *duck_i_if(duck_object_t **param)
{
    duck_object_t *body_object;
    if(param[0]!=null_object)
    {
	body_object=param[1];
    }
    else
    {
	body_object=param[2];
    }
    
    return duck_function_wrapped_invoke(body_object, 0, 0);
}

void duck_function_implementation_init(struct duck_stack_frame *stack)
{
    static wchar_t *p_argn[]={L"object"};
    duck_native_declare(stack, L"print", DUCK_FUNCTION_FUNCTION, (duck_native_t)&duck_i_print, null_type, 1, &object_type, p_argn);
    
    duck_native_declare(stack, L"__not__", DUCK_FUNCTION_FUNCTION, (duck_native_t)&duck_i_not, int_type, 1, &object_type, p_argn);
    

    duck_type_t *if_argv[]={object_type, object_type, object_type};
    static wchar_t *if_argn[]={L"condition", L"trueBlock", L"falseBlock"};    
    duck_native_declare(stack, L"__if__", DUCK_FUNCTION_FUNCTION, (duck_native_t)&duck_i_if, object_type, 3, if_argv, if_argn);
    
}
