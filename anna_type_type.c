#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_type.h"

static void anna_member_add_create(
    anna_type_t *type,
    ssize_t mid,
    wchar_t *name,
    int is_static,
    anna_type_t *member_type)
{
    anna_node_call_t *definition = anna_type_definition_get(type);
    anna_member_create(
	type,
	mid,
	name,
	is_static,
	member_type);    
    anna_member_add_node(
	definition,
	mid,
	name,
	is_static,
	(anna_node_t *)anna_node_identifier_create(0, member_type->name));
}

size_t anna_native_method_add_create(
    anna_type_t *type,
    ssize_t mid,
    wchar_t *name,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn)
{
    anna_node_call_t *definition = anna_type_definition_get(type);
    anna_native_method_create(
	type,
	mid,
	name,
	flags,
	func,
	result,
	argc,
	argv,
	argn);
    anna_node_t *n_argv[argc];
    int i;
    for(i=0;i<argc;i++)
    {
	n_argv[i] = (anna_node_t *)anna_node_identifier_create(0, argv[i]->name);
    }
    
    anna_native_method_add_node(
	definition,
	mid,
	name,
	flags,
	func,
	(anna_node_t *)anna_node_identifier_create(0, result->name),
	argc,
	n_argv,
	argn);
}

void anna_type_type_create_early(anna_stack_frame_t *stack)
{

    type_type = anna_type_create(L"Type");
    anna_type_definition_make(type_type);

    anna_member_add_create(
	type_type,
	ANNA_MID_TYPE_WRAPPER_PAYLOAD,
	L"!typeWrapperPayload",
	0,
	null_type);
}

static anna_object_t *anna_type_i_get_name(anna_object_t **param)
{
    anna_type_t *type = anna_type_unwrap(param[0]);
    return anna_string_create(wcslen(type->name), type->name);
}

void anna_type_type_create_late(anna_stack_frame_t *stack)
{
    anna_type_t *argv[] = 
	{
	    type_type
	}
    ;
    
    wchar_t *argn[]=
	{
	    L"this"
	}
    ;
    
    anna_native_method_add_create(
	type_type,
	-1,
	L"getName",
	0, (anna_native_t)&anna_type_i_get_name, 
	string_type,
	1, argv, argn );    
    
    
}
