#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_node_wrapper.h"
#include "anna_type.h"
#include "anna_string.h"
#include "anna_list.h"
#include "anna_int.h"
#include "anna_member.h"
#include "anna_function.h"
#include "anna_function_type.h"
#include "anna_vm.h"

anna_type_t *node_wrapper_type, *node_identifier_wrapper_type, *node_int_literal_wrapper_type, *node_string_literal_wrapper_type;


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


static inline anna_entry_t *anna_node_wrapper_i_replace_i(anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *tree = anna_node_unwrap(this);
    anna_node_identifier_t *old = (anna_node_identifier_t *)anna_node_unwrap(anna_as_obj(param[1]));
    anna_node_t *new = anna_node_unwrap(anna_as_obj(param[2]));
    anna_node_t *res = anna_node_replace(anna_node_clone_deep(tree), old, new);
    return anna_from_obj(anna_node_wrap(res));
}
ANNA_VM_NATIVE(anna_node_wrapper_i_replace, 3)

static inline anna_entry_t *anna_node_wrapper_i_error_i(anna_entry_t **param)
{
    anna_node_t *this = anna_node_unwrap(anna_as_obj_fast(param[0]));
    wchar_t *msg;
    if(ANNA_VM_NULL(param[1]))
    {
	msg = L"Unknown error";
    }
    else
    {
	msg = anna_string_payload(anna_as_obj(param[1]));
    }
    anna_error(this, L"%ls", msg);
    return param[0];
}
ANNA_VM_NATIVE(anna_node_wrapper_i_error, 2)

static inline anna_entry_t *anna_node_wrapper_i_to_string_i(anna_entry_t **param)
{
    anna_object_t *thiso = anna_as_obj_fast(param[0]);
    anna_node_t *this = anna_node_unwrap(thiso);
    wchar_t *str = anna_node_string(this);
    
    anna_object_t *res = anna_string_create(wcslen(str), str);
    free(str);
    return anna_from_obj(res);
}
ANNA_VM_NATIVE(anna_node_wrapper_i_to_string, 1)

static void anna_node_create_wrapper_type(anna_stack_template_t *stack)
{


    anna_type_t *replace_argv[] = 
	{
	    node_wrapper_type,
	    node_identifier_wrapper_type,
	    node_wrapper_type,
	}
    ;
    wchar_t *replace_argn[] =
	{
	    L"this", L"old", L"new"
	}
    ;
    
    anna_type_t *error_argv[] = 
	{
	    node_wrapper_type,
	    string_type
	}
    ;
    wchar_t *error_argn[] =
	{
	    L"this", L"message"
	}
    ;
    
    anna_member_create(
	node_wrapper_type, ANNA_MID_NODE_PAYLOAD,  L"!nodePayload", 
	0, null_type);
    
    anna_native_method_create(
	node_wrapper_type, -1, L"replace", 0, 
	&anna_node_wrapper_i_replace, 
	node_wrapper_type,
	3, replace_argv, replace_argn);
 
    anna_native_method_create(
	node_wrapper_type, -1, L"error", 0, 
	&anna_node_wrapper_i_error, 
	node_wrapper_type,
	2, error_argv, error_argn);
    
    anna_native_method_create(
	node_wrapper_type, -1, L"toString", 0, 
	&anna_node_wrapper_i_to_string, 
	string_type,
	1, error_argv, error_argn);    
}

#include "anna_node_identifier_wrapper.c"
#include "anna_node_int_literal_wrapper.c"
#include "anna_node_string_literal_wrapper.c"
#include "anna_node_call_wrapper.c"

void anna_node_create_wrapper_types(anna_stack_template_t *stack)
{
    node_wrapper_type = anna_type_native_create(L"Node", stack);
    node_identifier_wrapper_type = anna_type_native_create(L"Identifier", stack);
    node_int_literal_wrapper_type = anna_type_native_create(L"IntLiteral", stack);
    node_string_literal_wrapper_type = anna_type_native_create(L"StringLiteral", stack);
    
    node_call_wrapper_type = anna_type_native_create(L"Call", stack);

    anna_node_create_wrapper_type(stack);
    anna_node_create_identifier_wrapper_type(stack);
    anna_node_create_int_literal_wrapper_type(stack);
    anna_node_create_string_literal_wrapper_type(stack);
    anna_node_create_call_wrapper_type(stack);

    int i;
    anna_type_t *types[] = 
	{
	    node_wrapper_type,  node_identifier_wrapper_type,  
	    node_int_literal_wrapper_type,  node_string_literal_wrapper_type, 
	    node_call_wrapper_type, 
	};

    for(i=0; i<(sizeof(types)/sizeof(*types)); i++)
    {
	anna_type_copy_object(types[i]);
	anna_stack_declare(
	    stack, types[i]->name, 
	    type_type, anna_type_wrap(types[i]), ANNA_STACK_READONLY); 
    }



}
