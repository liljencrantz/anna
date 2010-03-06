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

    float_type = anna_type_create(L"Float", 64);
    anna_type_t *argv[]=
	{
	    float_type, float_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L"this", L"param"
	}
    ;

    anna_stack_declare(stack, L"Float", type_type, float_type->wrapper);

    /*
      FIXME, UGLY, BLERGH: Count and add payload twice, because it is twice the size of ptr on 32-bit. *ugh*
    */
    anna_member_create(float_type, ANNA_MID_FLOAT_PAYLOAD,  L"!floatPayload", 0, null_type);
    anna_member_create(float_type, -1,  L"!floatPayload2", 0, null_type);
    anna_float_type_i_create(stack);
    anna_native_method_create(float_type, -1, L"__expFloat__", 0, (anna_native_t)&anna_float_exp, float_type, 2, argv, argn);
    
}
