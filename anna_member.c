#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_member.h"
#include "anna_int.h"
#include "anna_char.h"
#include "anna_type.h"

static anna_type_t *member_method_type, *member_property_type, *member_variable_type;

anna_object_t *anna_member_wrap(anna_member_t *result)
{
    if(likely(result->wrapper))
	return result->wrapper;
    anna_type_t * type;
    if(result->is_method)
    {
	type = member_method_type;
    }
    else if(result->is_property)
    {
	type = member_property_type;
    }
    else
    {
	type=member_variable_type;
    }
    
    result->wrapper = anna_object_create(type);
    memcpy(anna_member_addr_get_mid(result->wrapper, ANNA_MID_MEMBER_PAYLOAD), &result, sizeof(anna_type_t *));  
    return result->wrapper;
}

anna_member_t *anna_member_unwrap(anna_object_t *wrapper)
{
    return *(anna_member_t **)anna_member_addr_get_mid(wrapper, ANNA_MID_MEMBER_PAYLOAD);
}

static anna_object_t *anna_member_i_get_name(anna_object_t **param)
{
    anna_member_t *m = anna_member_unwrap(param[0]);
    return anna_string_create(wcslen(m->name), m->name);
}


static anna_object_t *anna_member_i_get_static(anna_object_t **param)
{
    anna_member_t *m = anna_member_unwrap(param[0]);
    return m->is_static?anna_int_one:null_object;
}

static void anna_member_type_create(anna_stack_frame_t *stack)
{
    anna_node_t *argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Member"),
	}
    ;

    wchar_t *argn[] =
	{
	    L"this"
	}
    ;
    
    member_type = 
	anna_type_native_create(
	    L"Member",
	    stack);
    anna_node_call_t *definition = 
	anna_type_definition_get(member_type);
    
    anna_member_add_node(
	definition, 
	ANNA_MID_MEMBER_PAYLOAD, 
	L"!memberPayload", 
	0,
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    L"Null"));
    
    anna_native_method_add_node(
	definition,
	-1,
	L"!getName",
	0, 
	(anna_native_t)&anna_member_i_get_name, 
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
	L"!getStatic",
	0, 
	(anna_native_t)&anna_member_i_get_static, 
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    L"Int"),
	1,
	argv,
	argn );    
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"isStatic",
	    (anna_node_t *)anna_node_identifier_create(0, L"Int") , 
	    L"!getStatic", 0));
    
}

#include "anna_member_method.c"
#include "anna_member_property.c"
#include "anna_member_variable.c"

void anna_member_types_create(anna_stack_frame_t *stack)
{
    anna_member_type_create(stack);
    anna_member_method_type_create(stack);
    anna_member_property_type_create(stack);
    anna_member_variable_type_create(stack);
    
}
