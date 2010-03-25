#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_type.h"

anna_type_t *node_wrapper_type, *node_identifier_wrapper_type;


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


void anna_node_wrapper_type_create(anna_stack_frame_t *stack)
{
    /*
    anna_node_t *o_argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Char"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Int"),
	}
    ;
    wchar_t *o_argn[] =
	{
	    L"this", L"ordinal"
	}
    ;
    */
    node_wrapper_type = anna_type_native_create(L"Node", stack);

    anna_node_call_t *definition = anna_type_definition_get(node_wrapper_type);
    
    anna_member_add_node(
	definition, ANNA_MID_NODE_PAYLOAD,  L"!nodePayload", 
	0, (anna_node_t *)anna_node_identifier_create(0, L"Null") );
    
    anna_type_native_setup(node_wrapper_type, stack);
}

static anna_object_t *anna_node_identifier_wrapper_get_name(anna_object_t **param)
{
    anna_node_identifier_t *node = (anna_node_identifier_t *)anna_node_unwrap(param[0]);
    return anna_string_create(wcslen(node->name), node->name);
}

void anna_node_identifier_wrapper_type_create(anna_stack_frame_t *stack)
{
    anna_node_t *argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Identifier"),
	}
    ;

    wchar_t *argn[] =
	{
	    L"this"
	}
    ;

    node_identifier_wrapper_type = anna_type_native_create(L"Identifier", stack);
    anna_type_native_parent(node_identifier_wrapper_type, L"Node");

    anna_node_call_t *definition = anna_type_definition_get(node_identifier_wrapper_type);
    
    anna_native_method_add_node(
	definition, -1, L"getName", 0, 
	(anna_native_t)&anna_node_identifier_wrapper_get_name, 
	(anna_node_t *)anna_node_identifier_create(0, L"String"), 
	1, argv, argn);
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"name",
	    (anna_node_t *)anna_node_identifier_create(0, L"String") , 
	    L"getName", 0));
    
    anna_type_native_setup(node_identifier_wrapper_type, stack);
}

void anna_node_wrapper_types_create(anna_stack_frame_t *stack)
{
    anna_node_wrapper_type_create(stack);
    anna_node_identifier_wrapper_type_create(stack);
    
}
