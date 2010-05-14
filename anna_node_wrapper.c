#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_type.h"
#include "anna_string.h"
#include "anna_list.h"
#include "anna_int.h"

static anna_type_t *node_wrapper_type, *node_identifier_wrapper_type, *node_int_literal_wrapper_type, *node_string_literal_wrapper_type;


anna_type_t *node_call_wrapper_type;

anna_object_t *anna_node_wrap(anna_node_t *node)
{
    if(node->wrapper)
	return node->wrapper;
    
    anna_type_t *type=0;
    
    switch(node->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	    type = node_identifier_wrapper_type;
	    break;

	case ANNA_NODE_INT_LITERAL:
	    type = node_int_literal_wrapper_type;
	    break;

	case ANNA_NODE_CALL:
	    type = node_call_wrapper_type;
	    break;

	default:
	    type = node_wrapper_type;
	    break;
    }
    anna_object_t *obj= anna_object_create(type);
    *(anna_node_t **)anna_member_addr_get_mid(obj,ANNA_MID_NODE_PAYLOAD)=node;
    node->wrapper = obj;
    return obj;
}

anna_node_t *anna_node_unwrap(anna_object_t *this)
{
    return *(anna_node_t **)anna_member_addr_get_mid(this,ANNA_MID_NODE_PAYLOAD);
}


static anna_object_t *anna_node_wrapper_i_replace(anna_object_t **param)
{
    anna_node_t *tree = anna_node_unwrap(param[0]);
    anna_node_identifier_t *old = (anna_node_identifier_t *)anna_node_unwrap(param[1]);
    anna_node_t *new = anna_node_unwrap(param[2]);
    anna_node_t *res = anna_node_replace(anna_node_clone_deep(tree), old, new);
    return anna_node_wrap(res);
}

static anna_object_t *anna_node_wrapper_i_error(anna_object_t **param)
{
    anna_node_t *this = anna_node_unwrap(param[0]);
    wchar_t *msg;
    if(param[1] == null_object)
    {
	msg = L"Unknown error";
	
    }
    else
    {
	msg = anna_string_payload(param[1]);
    }
    anna_error(this, L"%ls", msg);
    return null_object;
}

static anna_object_t *anna_node_wrapper_i_print(anna_object_t **param)
{
    anna_node_t *this = anna_node_unwrap(param[0]);
    anna_node_print(this);
    return null_object;
}


void anna_node_wrapper_type_create(anna_stack_frame_t *stack)
{

    anna_node_t *replace_argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Node"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Identifier"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Node"),
	}
    ;
    wchar_t *replace_argn[] =
	{
	    L"this", L"old", L"new"
	}
    ;
    
    anna_node_t *error_argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Node"),
	    (anna_node_t *)anna_node_identifier_create(0, L"String"),
	}
    ;
    wchar_t *error_argn[] =
	{
	    L"this", L"message"
	}
    ;
    
    node_wrapper_type = anna_type_native_create(L"Node", stack);

    anna_node_call_t *definition = anna_type_definition_get(node_wrapper_type);
    
    anna_member_add_node(
	definition, ANNA_MID_NODE_PAYLOAD,  L"!nodePayload", 
	0, (anna_node_t *)anna_node_identifier_create(0, L"Null") );
    
    anna_native_method_add_node(
	definition, -1, L"replace", 0, 
	(anna_native_t)&anna_node_wrapper_i_replace, 
	(anna_node_t *)anna_node_identifier_create(0, L"Node"), 
	3, replace_argv, replace_argn);
 
    anna_native_method_add_node(
	definition, -1, L"error", 0, 
	(anna_native_t)&anna_node_wrapper_i_error, 
	(anna_node_t *)anna_node_identifier_create(0, L"Null"), 
	2, error_argv, error_argn);
    
    anna_native_method_add_node(
	definition, -1, L"print", 0, 
	(anna_native_t)&anna_node_wrapper_i_print, 
	(anna_node_t *)anna_node_identifier_create(0, L"Null"), 
	1, error_argv, error_argn);
    
}

#include "anna_node_identifier_wrapper.c"
#include "anna_node_int_literal_wrapper.c"
#include "anna_node_string_literal_wrapper.c"
#include "anna_node_call_wrapper.c"

void anna_node_wrapper_types_create(anna_stack_frame_t *stack)
{
    anna_node_wrapper_type_create(stack);
    anna_node_identifier_wrapper_type_create(stack);
    anna_node_int_literal_wrapper_type_create(stack);
    anna_node_string_literal_wrapper_type_create(stack);
    anna_node_call_wrapper_type_create(stack);
    
}
