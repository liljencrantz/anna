#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_int.h"

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


void anna_int_type_create(anna_stack_frame_t *stack)
{
    int_type = anna_type_create(L"Int", 64, 0);
    anna_stack_declare(stack, L"Int", type_type, int_type->wrapper);

    anna_node_call_t *definition = 
	anna_node_call_create(
	    0,
	    (anna_node_t *)anna_node_identifier_create(0, L"__block__"),
	    0,
	    0);
    
    anna_node_call_t *full_definition = 
	anna_node_call_create(
	    0,
	    (anna_node_t *)anna_node_identifier_create(0, L"__type__"),
	    0,
	    0);
    int_type->definition = full_definition;

    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    L"Int"));
    
    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    L"class"));
    
    anna_node_call_t *attribute_list = 
	anna_node_call_create(
	    0,
	    (anna_node_t *)anna_node_identifier_create(0, L"__block__"),
	    0,
	    0);	

    anna_node_call_add_child(
	full_definition,	
	(anna_node_t *)attribute_list);
    
    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)definition);
    
    anna_member_add_node(
	definition, ANNA_MID_INT_PAYLOAD,  L"!intPayload", 
	0, (anna_node_t *)anna_node_identifier_create(0, L"Null") );

    anna_int_type_i_create(definition, stack);

    anna_function_t *func;

    func = anna_native_create(L"!anonymous",
			      ANNA_FUNCTION_MACRO,
			      (anna_native_t)(anna_native_function_t)0,
			      0,
			      0,
			      0, 
			      0);
    func->stack_template=stack;
    anna_macro_type_setup(int_type, func, 0);

    anna_int_one = anna_int_create(1);
}
