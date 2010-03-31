#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_int.h"
#include "anna_type.h"

#include "anna_int_i.c"

anna_object_t *anna_int_create(int value)
{
  anna_object_t *obj= anna_object_create(int_type);
  anna_int_set(obj, value);
  return obj;
}

void anna_int_set(anna_object_t *this, int value)
{
    memcpy(anna_member_addr_get_mid(this,ANNA_MID_INT_PAYLOAD), &value, sizeof(int));
}

int anna_int_get(anna_object_t *this)
{
    int result;
    memcpy(&result, anna_member_addr_get_mid(this,ANNA_MID_INT_PAYLOAD), sizeof(int));
    return result;
}

static anna_object_t *anna_int_init(anna_object_t **param)
{
    anna_int_set(param[0], 0);
    return param[0];
}

void anna_int_type_create(anna_stack_frame_t *stack)
{
    anna_node_t *i_argv[] = 
	{
	  (anna_node_t *)anna_node_identifier_create(0, L"Int")
	}
    ;
    wchar_t *i_argn[]=
	{
	    L"this"
	}
    ;


    int_type = anna_type_native_create(L"Int", stack);
    anna_node_call_t *definition = 
	anna_type_definition_get(int_type);
    
    anna_member_add_node(
	definition, ANNA_MID_INT_PAYLOAD,  L"!intPayload", 
	0, (anna_node_t *)anna_node_identifier_create(0, L"Null") );
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__init__",
	ANNA_FUNCTION_VARIADIC, 
	(anna_native_t)&anna_int_init, 
	(anna_node_t *)anna_node_identifier_create(0, L"Null") , 
	1, i_argv, i_argn);    

    anna_int_type_i_create(definition, stack);
}
