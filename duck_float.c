#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "duck.h"
#include "duck_node.h"
#include "duck_float.h"

#include "duck_float_i.c"


duck_object_t *duck_float_create(double value)
{
    duck_object_t *obj= duck_object_create(float_type);
    duck_float_set(obj, value);
    return obj;
}

void duck_float_set(duck_object_t *this, double value)
{
    memcpy(duck_member_addr_get_mid(this,DUCK_MID_FLOAT_PAYLOAD), &value, sizeof(double));
}

double duck_float_get(duck_object_t *this)
{
    double result;
    memcpy(&result, duck_member_addr_get_mid(this,DUCK_MID_FLOAT_PAYLOAD), sizeof(double));
    return result;
}

static duck_object_t *duck_float_exp(duck_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    double v1 = duck_float_get(param[0]);
    double v2 = duck_float_get(param[1]);
    return duck_float_create(pow(v1, v2));
}

void duck_float_type_create(duck_stack_frame_t *stack)
{

    float_type = duck_type_create(L"Float", 64);
    duck_type_t *argv[]=
	{
	    float_type, float_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L"this", L"param"
	}
    ;

    duck_stack_declare(stack, L"Float", type_type, float_type->wrapper);

    /*
      FIXME, UGLY, BLERGH: Count and add payload twice, because it is twice the size of ptr on 32-bit. *ugh*
    */
    duck_member_create(float_type, DUCK_MID_FLOAT_PAYLOAD,  L"!floatPayload", 0, null_type);
    duck_member_create(float_type, -1,  L"!floatPayload2", 0, null_type);
    duck_float_type_i_create(stack);
    duck_native_method_create(float_type, -1, L"__expFloat__", 0, (duck_native_t)&duck_float_exp, float_type, 2, argv, argn);
    
}
