#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_float.h"
#include "anna_type.h"
#include "anna_int.h"

#include "anna_float_i.c"

anna_object_t *anna_float_create(double value)
{
    anna_object_t *obj= anna_object_create(float_type);
    anna_float_set(obj, value);
    return obj;
}

void anna_float_set(anna_object_t *this, double value)
{
    memcpy(anna_member_addr_get_mid(this,ANNA_MID_FLOAT_PAYLOAD), &value, sizeof(double));
}

double anna_float_get(anna_object_t *this)
{
    double result;
    memcpy(&result, anna_member_addr_get_mid(this,ANNA_MID_FLOAT_PAYLOAD), sizeof(double));
    return result;
}

void anna_float_type_create(anna_stack_frame_t *stack)
{
    float_type = anna_type_native_create(L"Float", stack);
    anna_node_call_t *definition = anna_type_definition_get(float_type);

    anna_member_add_node(
	definition, ANNA_MID_FLOAT_PAYLOAD,  L"!floatPayload",
	0, (anna_node_t *)anna_node_create_identifier(0, L"Null"));
    /*
      If we can't fit a double into a void * on this platform, add a
      second dummy payload. This assumes that sizeof(double) is never
      more than twice the size of sizeof(void *).

      This code also assumes that consecutively registered members
      will be stored next to each other, which is currently true, but
      we will need to keep this in mind when trying to optimize member
      layout in the future.
    */
    if(sizeof(double) > sizeof(void *)) 
    {
	anna_member_add_node(
	    definition, -1,  L"!floatPayload2", 0, 
	    (anna_node_t *)anna_node_create_identifier(0, L"Null"));
    }
    
    anna_float_type_i_create(definition, stack);
}
