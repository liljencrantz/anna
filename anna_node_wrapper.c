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

static anna_object_t *anna_node_call_wrapper_i_get_int(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    int idx = anna_int_get(param[1]);
    if(idx < 0 || idx >= node->child_count)
	return null_object;
    return anna_node_wrap(node->child[idx]);
}

static anna_object_t *anna_node_call_wrapper_i_set_int(anna_object_t **param)
{
    
    if(param[1]==null_object)
	return null_object;
    
    if(param[2]==null_object)
	param[2] = anna_node_wrap(anna_node_null_create(0));
    
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    int idx = anna_int_get(param[1]);
    if(idx < 0 || idx >= node->child_count)
	return param[1];
    
    node->child[idx] = anna_node_unwrap(param[2]);
    return param[2];
}

static anna_object_t *anna_node_int_literal_wrapper_i_get_function(anna_object_t **param)
{
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    return anna_node_wrap(node->function);
}

static anna_object_t *anna_node_int_literal_wrapper_i_set_function(anna_object_t **param)
{
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    if(param[1]==null_object)
	return null_object;
    node->function = anna_node_unwrap(param[1]);
    return param[1];
    
}

static anna_object_t *anna_node_call_wrapper_i_join_list(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    size_t count = anna_list_get_size(param[1]);
    
    anna_node_call_t *src = (anna_node_call_t *)anna_node_unwrap(param[0]);
    anna_node_call_t *dst = anna_node_call_create(
	&src->location,
	src->function,
	src->child_count,
	src->child);
    int i;
    for(i=0;i<count; i++)
    {
	anna_object_t *n = anna_list_get(param[1], i);
	anna_node_call_add_child(dst,anna_node_unwrap(n));
    }
    return anna_node_wrap(dst);
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
    
    anna_node_t *i_argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Call"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Int"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Node"),
	}
    ;
    
    wchar_t *i_argn[] =
	{
	    L"this", L"index", L"value"
	}
    ;
    
    anna_node_t *j_argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Call"),
	    anna_node_simple_template_create(
		0,
		L"List",
		L"Node"),
	}
    ;
    
    wchar_t *j_argn[] =
	{
	    L"this", L"list"
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

    anna_native_method_add_node(
	definition,
	-1,
	L"__get__Int__",
	0, 
	(anna_native_t)&anna_node_call_wrapper_i_get_int, 
	(anna_node_t *)anna_node_identifier_create(0, L"Node") , 
	2, 
	i_argv, 
	i_argn);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__set__Int__",
	0, 
	(anna_native_t)&anna_node_call_wrapper_i_set_int, 
	(anna_node_t *)anna_node_identifier_create(0, L"Node") , 
	3, 
	i_argv, 
	i_argn);

    
    anna_native_method_add_node(
	definition,
	-1,
	L"__join__List__",
	0, 
	(anna_native_t)&anna_node_call_wrapper_i_join_list, 
	(anna_node_t *)anna_node_identifier_create(0, L"Call") , 
	2, 
	j_argv, 
	j_argn);
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"count",
	    (anna_node_t *)anna_node_identifier_create(0, L"Int") , 
	    L"getCount",
	    0));

	
    anna_native_method_add_node(
	definition, -1, L"setFunction", 0, 
	(anna_native_t)&anna_node_int_literal_wrapper_i_set_function, 
	(anna_node_t *)anna_node_identifier_create(0, L"Node"), 
	2, argv, argn);
    
    anna_native_method_add_node(
	definition, -1, L"getFunction", 0, 
	(anna_native_t)&anna_node_int_literal_wrapper_i_get_function, 
	(anna_node_t *)anna_node_identifier_create(0, L"Node"), 
	1, argv, argn);
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"func",
	    (anna_node_t *)anna_node_identifier_create(0, L"Node") , 
	    L"getFunction",
	    L"setFunction"));
    

}

void anna_node_wrapper_types_create(anna_stack_frame_t *stack)
{
    anna_node_wrapper_type_create(stack);
    anna_node_identifier_wrapper_type_create(stack);
    anna_node_int_literal_wrapper_type_create(stack);
    anna_node_call_wrapper_type_create(stack);
    
    anna_type_native_setup(node_wrapper_type, stack);
    anna_type_native_setup(node_identifier_wrapper_type, stack);
    anna_type_native_setup(node_int_literal_wrapper_type, stack);
    anna_type_native_setup(node_call_wrapper_type, stack);
}
