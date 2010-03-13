#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_float.h"
#include "anna_type.h"

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

static anna_object_t *anna_float_exp(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    double v1 = anna_float_get(param[0]);
    double v2 = anna_float_get(param[1]);
    return anna_float_create(pow(v1, v2));
}

void anna_float_type_create(anna_stack_frame_t *stack)
{
    float_type = anna_type_native_create(L"Float", stack);
    anna_node_call_t *definition = anna_type_definition_get(float_type);

    /*
      FIXME, UGLY, BLERGH: Count and add payload twice, because it is twice the size of ptr on 32-bit. *ugh*
    */
    anna_member_add_node(
	definition, ANNA_MID_FLOAT_PAYLOAD,  L"!floatPayload",
	0, (anna_node_t *)anna_node_identifier_create(0, L"Null"));
    anna_member_add_node(
	definition, -1,  L"!floatPayload2", 0, 
	(anna_node_t *)anna_node_identifier_create(0, L"Null"));

    anna_float_type_i_create(definition, stack);
    anna_type_native_setup(float_type, stack);
        
//    anna_native_method_create(float_type, -1, L"__expFloat__", 0, (anna_native_t)&anna_float_exp, float_type, 2, argv, argn);
    
}
