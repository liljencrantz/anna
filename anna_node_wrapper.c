#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_type.h"

static anna_type_t *node_wrapper_type, *node_identifier_wrapper_type, *node_int_literal_wrapper_type;


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

static anna_object_t *anna_node_identifier_wrapper_i_get_name(anna_object_t **param)
{
    anna_node_identifier_t *node = (anna_node_identifier_t *)anna_node_unwrap(param[0]);
    return anna_string_create(wcslen(node->name), node->name);
}

static anna_object_t *anna_node_identifier_wrapper_i_init(anna_object_t **param)
{
    assert(param[0] != null_object);
    assert(param[1] != null_object);
    assert(param[2] != null_object);
    anna_node_t *source = anna_node_unwrap(param[1]);
    *(anna_node_t **)anna_member_addr_get_mid(param[0],ANNA_MID_NODE_PAYLOAD)=anna_node_identifier_create(
	&source->location,
	wcsdup(anna_string_payload(param[2])));
    return param[0];
}

void anna_node_identifier_wrapper_type_create(anna_stack_frame_t *stack)
{
    anna_node_t *argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Identifier"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Node"),
	    (anna_node_t *)anna_node_identifier_create(0, L"String"),
	}
    ;
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"name"
	}
    ;

    node_identifier_wrapper_type = anna_type_native_create(L"Identifier", stack);
    anna_type_native_parent(node_identifier_wrapper_type, L"Node");
    
    anna_node_call_t *definition = anna_type_definition_get(node_identifier_wrapper_type);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__init__",
	0,
	(anna_native_t)&anna_node_identifier_wrapper_i_init, 
	(anna_node_t *)anna_node_identifier_create(0, L"Null") , 
	3, argv, argn);


    anna_native_method_add_node(
	definition, -1, L"getName", 0, 
	(anna_native_t)&anna_node_identifier_wrapper_i_get_name, 
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

static anna_object_t *anna_node_int_literal_wrapper_i_get_payload(anna_object_t **param)
{
    anna_node_int_literal_t *node = (anna_node_int_literal_t *)anna_node_unwrap(param[0]);
    return anna_int_create(node->payload);
}

static anna_object_t *anna_node_int_literal_wrapper_i_init(anna_object_t **param)
{
    assert(param[0] != null_object);
    assert(param[1] != null_object);
    assert(param[2] != null_object);
    anna_node_t *source = anna_node_unwrap(param[1]);
    *(anna_node_t **)anna_member_addr_get_mid(
	param[0],
	ANNA_MID_NODE_PAYLOAD)=
	anna_node_int_literal_create(
	    &source->location,
	    anna_int_get(param[2]));
    return param[0];
}

void anna_node_int_literal_wrapper_type_create(anna_stack_frame_t *stack)
{
    anna_node_t *argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Identifier"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Node"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Int"),
	}
    ;
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"payload"
	}
    ;

    node_int_literal_wrapper_type = anna_type_native_create(L"IntLiteral", stack);
    anna_type_native_parent(node_int_literal_wrapper_type, L"Node");
    
    anna_node_call_t *definition = anna_type_definition_get(node_int_literal_wrapper_type);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__init__",
	0,
	(anna_native_t)&anna_node_int_literal_wrapper_i_init, 
	(anna_node_t *)anna_node_identifier_create(0, L"Null") , 
	3, argv, argn);
    
    anna_native_method_add_node(
	definition, -1, L"getPayload", 0, 
	(anna_native_t)&anna_node_int_literal_wrapper_i_get_payload, 
	(anna_node_t *)anna_node_identifier_create(0, L"Int"), 
	1, argv, argn);
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"payload",
	    (anna_node_t *)anna_node_identifier_create(0, L"Int") , 
	    L"getPayload", 0));
    
    anna_type_native_setup(node_int_literal_wrapper_type, stack);
}

static anna_object_t *anna_node_call_wrapper_i_init(anna_object_t **param)
{
    assert(param[0] != null_object);
    assert(param[1] != null_object);
    assert(param[2] != null_object);

    wprintf(L"Creating new call nodes is not yet implemented\n");
    CRASH;
    
    anna_node_t *source = anna_node_unwrap(param[1]);

    *(anna_node_t **)anna_member_addr_get_mid(
	param[0],
	ANNA_MID_NODE_PAYLOAD)=
	anna_node_call_create(
	    &source->location,
	    0, 0,0);
    
    return param[0];
}

static anna_object_t *anna_node_call_wrapper_i_get_count(anna_object_t **param)
{
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    return anna_int_create(node->child_count);
}

void anna_node_call_wrapper_type_create(anna_stack_frame_t *stack)
{
    anna_node_t *argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Identifier"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Node"),
	}
    ;
    
    wchar_t *argn[] =
	{
	    L"this", L"source"
	}
    ;
    
    node_call_wrapper_type = anna_type_native_create(L"Call", stack);
    anna_type_native_parent(node_call_wrapper_type, L"Node");
    
    anna_node_call_t *definition = anna_type_definition_get(node_call_wrapper_type);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__init__",
	ANNA_FUNCTION_VARIADIC,
	(anna_native_t)&anna_node_call_wrapper_i_init, 
	(anna_node_t *)anna_node_identifier_create(0, L"Null") , 
	2, argv, argn);
    
    anna_native_method_add_node(
	definition, -1, L"getCount", 0, 
	(anna_native_t)&anna_node_call_wrapper_i_get_count, 
	(anna_node_t *)anna_node_identifier_create(0, L"Int"), 
	1, argv, argn);
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"count",
	    (anna_node_t *)anna_node_identifier_create(0, L"Int") , 
	    L"getCount",
	    0));
	
    anna_type_native_setup(node_call_wrapper_type, stack);
}

void anna_node_wrapper_types_create(anna_stack_frame_t *stack)
{
    anna_node_wrapper_type_create(stack);
    anna_node_identifier_wrapper_type_create(stack);
    anna_node_int_literal_wrapper_type_create(stack);
    anna_node_call_wrapper_type_create(stack);
    
}
