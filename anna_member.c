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

anna_object_t *anna_member_wrapper_create(size_t sz, wchar_t *data)
{
    anna_object_t *obj = 
	anna_object_create(
	    member_type);
    return obj;
}

anna_object_t *anna_member_wrap(anna_member_t *result)
{
    if(likely(result->wrapper))
	return result->wrapper;

    result->wrapper = anna_object_create(member_type);
    memcpy(anna_member_addr_get_mid(result->wrapper, ANNA_MID_MEMBER_PAYLOAD), &result, sizeof(anna_type_t *));  
    return result->wrapper;
}

anna_member_t *anna_member_unwrap(anna_object_t *wrapper)
{
    return *(anna_member_t **)anna_member_addr_get_mid(wrapper, ANNA_MID_MEMBER_PAYLOAD);
}

void anna_member_type_create(anna_stack_frame_t *stack)
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
}
