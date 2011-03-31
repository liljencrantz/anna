#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_type.h"
#include "anna_type_type.h"
#include "anna_string.h"
#include "anna_list.h"
#include "anna_member.h"
#include "anna_node_create.h"
#include "anna_vm.h"
/*
static void anna_member_add_create(
    anna_type_t *type,
    mid_t mid,
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
	(anna_node_t *)anna_node_create_identifier(0, member_type->name));
}

static void anna_native_method_add_create(
    anna_type_t *type,
    mid_t mid,
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
	n_argv[i] = (anna_node_t *)anna_node_create_identifier(0, argv[i]->name);
    }
    
    anna_native_method_add_node(
	definition,
	mid,
	name,
	flags,
	func,
	(anna_node_t *)anna_node_create_identifier(0, result->name),
	argc,
	n_argv,
	argn);
}
*/

static inline anna_object_t *anna_type_i_get_name_i(anna_object_t **param)
{
    anna_type_t *type = anna_type_unwrap(param[0]);
    return anna_string_create(wcslen(type->name), type->name);
}
ANNA_VM_NATIVE(anna_type_i_get_name, 1)

static inline anna_object_t *anna_type_i_get_member_i(anna_object_t **param)
{
    anna_object_t *lst = anna_list_create(member_type);
    int i;
    anna_type_t *type = anna_type_unwrap(param[0]);

    wchar_t **member_name = malloc(sizeof(wchar_t *)*anna_type_member_count(type));
    anna_type_get_member_names(type, member_name);
    for(i=0;i<anna_type_member_count(type); i++)
    {
	anna_list_add(
	    lst,
	    anna_member_wrap(
		type,
		anna_type_member_info_get(
		    type,
		    member_name[i])));	
    }
    
    return lst;
}
ANNA_VM_NATIVE(anna_type_i_get_member, 1)

void anna_type_type_create(anna_stack_template_t *stack)
{

    anna_member_create(
	type_type,
	ANNA_MID_TYPE_WRAPPER_PAYLOAD,
	L"!typeWrapperPayload",
	0,
	null_type
	);
}


void anna_type_type_create2(anna_stack_template_t *stack)
{
    anna_native_property_create(
	type_type,
	-1,
	L"name",
	string_type,
	&anna_type_i_get_name, 
	0);
    
    anna_native_property_create(
	type_type,
	-1,
	L"member",
	anna_list_type_get(member_type),
	&anna_type_i_get_member, 
	0);

}
