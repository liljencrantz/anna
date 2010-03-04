#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "duck.h"
#include "duck_node.h"
#include "duck_int.h"

#include "duck_int_i.c"

duck_object_t *duck_int_create(int value)
{
  duck_object_t *obj= duck_object_create(int_type);
  duck_int_set(obj, value);
  return obj;
}

void duck_int_set(duck_object_t *this, int value)
{
    memcpy(duck_member_addr_get_mid(this,DUCK_MID_INT_PAYLOAD), &value, sizeof(int));
}

int duck_int_get(duck_object_t *this)
{
  
  int result;
  memcpy(&result, duck_member_addr_get_mid(this,DUCK_MID_INT_PAYLOAD), sizeof(int));
  return result;
}

static duck_object_t *duck_int_i_abs(duck_object_t **param)
{
    int v1 = duck_int_get(param[0]);
    return duck_int_create(abs(v1));
}

static duck_object_t *duck_int_i_neg(duck_object_t **param)
{
    int v1 = duck_int_get(param[0]);
    return duck_int_create(-v1);
}

static duck_object_t *duck_int_i_sign(duck_object_t **param)
{
    int v1 = duck_int_get(param[0]);
    return duck_int_create(v1==0?0:(v1>0?1:-1));
}

static duck_object_t *duck_int_i_bitnot(duck_object_t **param)
{
    int v1 = duck_int_get(param[0]);
    return duck_int_create(~v1);
}

static duck_object_t *duck_int_i_cshl(duck_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    int v1 = duck_int_get(param[0]);
    int v2 = duck_int_get(param[1]);
    return duck_int_create((v1 << v2) | (v1 >> (32-v2)));
}
static duck_object_t *duck_int_i_cshr(duck_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    int v1 = duck_int_get(param[0]);
    int v2 = duck_int_get(param[1]);
    return duck_int_create((v1 >> v2) | (v1 << (32-v2)));
}


void duck_int_type_create(duck_stack_frame_t *stack)
{
    int_type = duck_type_create(L"Int", 64);
    duck_type_t *argv[]=
	{
	    int_type, int_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L"this", L"param"
	}
    ;

    duck_stack_declare(stack, L"Int", type_type, int_type->wrapper);
    duck_member_create(int_type, DUCK_MID_INT_PAYLOAD,  L"!intPayload", 0, null_type);    

    duck_native_method_create(int_type, -1, L"__abs__", 0, (duck_native_t)&duck_int_i_abs, int_type, 1, argv, argn);
    duck_native_method_create(int_type, -1, L"__sign__", 0, (duck_native_t)&duck_int_i_sign, int_type, 1, argv, argn);
    duck_native_method_create(int_type, -1, L"__neg__", 0, (duck_native_t)&duck_int_i_neg, int_type, 1, argv, argn);
    duck_native_method_create(int_type, -1, L"__bitnot__", 0, (duck_native_t)&duck_int_i_bitnot, int_type, 1, argv, argn);

    duck_native_method_create(int_type, -1, L"__cshlInt__", 0, (duck_native_t)&duck_int_i_cshl, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__cshrInt__", 0, (duck_native_t)&duck_int_i_cshr, int_type, 2, argv, argn);

    duck_int_type_i_create(stack);
    duck_int_one = duck_int_create(1);
}
