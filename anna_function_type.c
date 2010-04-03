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
#include "anna_function_type.h"
#include "anna_function.h"
#include "anna_string.h"
#include "anna_list.h"
#include "anna_type.h"

static int base_created = 0;


static anna_object_t *anna_function_type_i_get_name(anna_object_t **param)
{
    anna_function_t *f = anna_function_unwrap(param[0]);
    return anna_string_create(wcslen(f->name), f->name);
}

static anna_object_t *anna_function_type_i_get_output(anna_object_t **param)
{
    anna_function_t *f = anna_function_unwrap(param[0]);
    return anna_type_wrap(f->return_type);
}

static anna_object_t *anna_function_type_i_get_input_type(anna_object_t **param)
{

    anna_object_t *lst = anna_list_create();
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

    anna_object_t *lst = anna_list_create();
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
    if(base_created)
	return;
    
    base_created = 1;
    
    anna_node_t *argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"!FunctionTypeBase")
	}
    ;
    
    wchar_t *argn[]=
	{
	    L"this"
	}
    ;
    
    anna_type_t *res =
	anna_type_native_create(
	    L"!FunctionTypeBase",
	    stack_global);	
    anna_node_call_t *definition = 
	anna_type_definition_get(res);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"!getName",
	0, 
	(anna_native_t)&anna_function_type_i_get_name, 
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    L"String"),
	1,
	argv,
	argn );    
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"name",
	    (anna_node_t *)anna_node_identifier_create(0, L"String") , 
	    L"!getName", 0));

    anna_native_method_add_node(
	definition,
	-1,
	L"!getOutputType",
	0, 
	(anna_native_t)&anna_function_type_i_get_output, 
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    L"Type"),
	1,
	argv,
	argn );
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"outputType",
	    (anna_node_t *)anna_node_identifier_create(0, L"Type"),
	    L"!getOutputType", 0));


    anna_native_method_add_node(
	definition,
	-1,
	L"!getInputType",
	0,
	(anna_native_t)&anna_function_type_i_get_input_type, 
	anna_node_simple_templated_type_create(
	    0, 
	    L"List",
	    L"Type"),
	1,
	argv,
	argn );    
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"inputType",
	    anna_node_simple_templated_type_create(
		0, 
		L"List",
		L"Type"),
	    L"!getInputType",
	    0));
    

    anna_native_method_add_node(
	definition,
	-1,
	L"!getInputName",
	0,
	(anna_native_t)&anna_function_type_i_get_input_name, 
	anna_node_simple_templated_type_create(
	    0, 
	    L"List",
	    L"String"),
	1,
	argv,
	argn );    
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"inputName",
	    anna_node_simple_templated_type_create(
		0, 
		L"List",
		L"String"),
	    L"!getInputName",
	    0));
    


    
}


anna_type_t *anna_function_type_create(anna_function_type_key_t *key)
{

    anna_function_type_base_create();
    static int num = 0;
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%ls%d", L"!FunctionType", num++);
/*    
    wprintf(L"Creating function type %ls\n", sb_content(&sb));
    anna_function_type_key_print(key);
*/  
    anna_type_t *res = anna_type_native_create(sb_content(&sb), stack_global);	
    anna_type_native_parent(res, L"!FunctionTypeBase");
    sb_destroy(&sb);

    anna_member_create(
	res,
	ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD, 
	L"!functionTypePayload",
	1,
	null_type);
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
    
    anna_node_call_t *definition = 
	anna_type_definition_get(res);
    
    anna_member_add_node(
	definition, 
	ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD, 
	L"!functionTypePayload",
	1,
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    L"Null"));
    
    anna_member_add_node(
	definition, 
	ANNA_MID_FUNCTION_WRAPPER_PAYLOAD, 
	L"!functionPayload",
	0,
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    L"Null"));
    
    anna_member_add_node(
	definition, 
	ANNA_MID_FUNCTION_WRAPPER_STACK, 
	L"!functionStack",
	0,
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    L"Null"));
    
    (*anna_static_member_addr_get_mid(res, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD)) = (anna_object_t *)key;
    return res;
    
}
