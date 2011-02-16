#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_function_type.h"
#include "anna_function.h"
#include "anna_string.h"
#include "anna_list.h"
#include "anna_type.h"
#include "anna_member.h"

static anna_type_t *function_type_base = 0;

static anna_object_t *anna_function_type_i_get_name(anna_object_t **param)
{
    anna_function_t *f = anna_function_unwrap(param[0]);
    return anna_string_create(wcslen(f->name), f->name);
}

static anna_object_t *anna_function_type_i_get_output(anna_object_t **param)
{
    CRASH;
    anna_function_t *f = anna_function_unwrap(param[0]);
    return anna_type_wrap(f->return_type);
}

static anna_object_t *anna_function_type_i_get_input_type(anna_object_t **param)
{

    anna_object_t *lst = anna_list_create(type_type);
    int i;
    anna_function_t *f = anna_function_unwrap(param[0]);

    for(i=0;i<f->input_count; i++)
    {
	anna_list_add(
	    lst,
	    anna_type_wrap(
		f->input_type[i]));
    }
    
    return lst;
}

static anna_object_t *anna_function_type_i_get_input_name(anna_object_t **param)
{

    anna_object_t *lst = anna_list_create(string_type);
    int i;
    anna_function_t *f = anna_function_unwrap(param[0]);

    for(i=0;i<f->input_count; i++)
    {
	anna_list_add(
	    lst,
	    anna_string_create(
		wcslen(f->input_name[i]),
		f->input_name[i]));
    }
    
    return lst;
}


void anna_function_type_key_print(anna_function_type_key_t *k)
{

    wprintf(L"%ls <- (", k->result?k->result->name:L"<null>");
    int i;
    for(i=0;i<k->argc; i++)
    {
	if(i!=0)
	    wprintf(L", ");
	wprintf(
	    L"%ls %ls",
	    (k->argv && k->argv[i])?k->argv[i]->name:L"<null>",
	    (k->argn && k->argn[i])?k->argn[i]:L"");
    }
    wprintf(L")\n");
}

void anna_function_type_base_create()
{

    if(function_type_base)
	return;

    function_type_base = anna_type_native_create(L"!FunctionBase", stack_global);
    anna_type_t *res = function_type_base;

    anna_native_property_create(
	res,
	-1,
	L"name",
	string_type,
	&anna_function_type_i_get_name,
	0);
    
    anna_native_property_create(
	res,
	-1,
	L"outputType",
	type_type,
	&anna_function_type_i_get_output,
	0);

    anna_native_property_create(
	res,
	-1,
	L"inputType",
	anna_list_type_get(type_type),
	&anna_function_type_i_get_input_type,
	0);

    anna_native_property_create(
	res,
	-1,
	L"inputName",
	anna_list_type_get(string_type),
	&anna_function_type_i_get_input_name,
	0);
}

void anna_function_type_create(
    anna_function_type_key_t *key, 
    anna_type_t *res)
{
    //anna_function_type_key_print(key);

    anna_function_type_base_create();
    anna_type_copy(res, function_type_base);

    /*
      Non-static member variables
    */
    anna_member_create(
	res,
	ANNA_MID_FUNCTION_WRAPPER_PAYLOAD, L"!functionPayload", 
	0,
	null_type);
    anna_member_create(
	res,
	ANNA_MID_FUNCTION_WRAPPER_STACK, L"!functionStack", 
	0,
	null_type);
    
    /*
      Static member variables
    */
    anna_member_create(
	res,
	ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD, 
	L"!functionTypePayload",
	1,
	null_type);
    
    anna_member_create(
	res, 
	ANNA_MID_FUNCTION_WRAPPER_PAYLOAD, 
	L"!functionPayload",
	0,
	null_type);
    
    anna_member_create(
	res, 
	ANNA_MID_FUNCTION_WRAPPER_STACK, 
	L"!functionStack",
	0,
	null_type);    
    (*anna_static_member_addr_get_mid(res, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD)) = (anna_object_t *)key;

    return res;
}

anna_function_type_key_t *anna_function_type_extract(anna_type_t *type)
{
    anna_function_type_key_t **res = (anna_function_type_key_t **)anna_static_member_addr_get_mid(type, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    return res?*res:0;
}
