#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "duck.h"
#include "duck_node.h"
#include "duck_int.h"


static duck_object_t *duck_int_gt(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
  
    int v1 = duck_int_get(param[0]);
    int v2 = duck_int_get(param[1]);
    return v1>v2?param[0]:null_object;
}


static duck_object_t *duck_int_lt(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
    int v1 = duck_int_get(param[0]);
    int v2 = duck_int_get(param[1]);
    return v1<v2?param[0]:null_object;
}

static duck_object_t *duck_int_eq(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
    int v1 = duck_int_get(param[0]);
    int v2 = duck_int_get(param[1]);
    return v1==v2?param[0]:null_object;
}

static duck_object_t *duck_int_gte(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
    int v1 = duck_int_get(param[0]);
    int v2 = duck_int_get(param[1]);
    return v1>=v2?param[0]:null_object;
}

static duck_object_t *duck_int_lte(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
    int v1 = duck_int_get(param[0]);
    int v2 = duck_int_get(param[1]);
    return v1<=v2?param[0]:null_object;
}


static duck_object_t *duck_int_neq(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
    int v1 = duck_int_get(param[0]);
    int v2 = duck_int_get(param[1]);
    return v1!=v2?param[0]:null_object;
}

static duck_object_t *duck_int_add(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
  duck_object_t *result = duck_int_create();
  int v1 = duck_int_get(param[0]);
  int v2 = duck_int_get(param[1]);
  duck_int_set(result, v1+v2);
  return result;
}

static duck_object_t *duck_int_sub(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
  duck_object_t *result = duck_int_create();
  int v1 = duck_int_get(param[0]);
  int v2 = duck_int_get(param[1]);
  duck_int_set(result, v1-v2);
  return result;
}

static duck_object_t *duck_int_mul(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
  duck_object_t *result = duck_int_create();
  int v1 = duck_int_get(param[0]);
  int v2 = duck_int_get(param[1]);
  duck_int_set(result, v1*v2);
  return result;
}

static duck_object_t *duck_int_div(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
  duck_object_t *result = duck_int_create();
  int v1 = duck_int_get(param[0]);
  int v2 = duck_int_get(param[1]);
  duck_int_set(result, v1/v2);
  return result;
}


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

void duck_int_type_create(duck_stack_frame_t *stack)
{
    int_type = duck_type_create(L"Int", 64);
    duck_stack_declare(stack, L"Int", type_type, int_type->wrapper);
    duck_member_create(int_type, DUCK_MID_INT_PAYLOAD,  L"!intPayload", 0, null_type);
    
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
    
    duck_native_method_create(int_type, -1, L"__addInt__", 0, (duck_native_t)&duck_int_add, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__subInt__", 0, (duck_native_t)&duck_int_sub, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__mulInt__", 0, (duck_native_t)&duck_int_mul, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__divInt__", 0, (duck_native_t)&duck_int_div, int_type, 2, argv, argn);
    
    duck_native_method_create(int_type, -1, L"__gtInt__", 0, (duck_native_t)&duck_int_gt, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__ltInt__", 0, (duck_native_t)&duck_int_lt, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__eqInt__", 0, (duck_native_t)&duck_int_eq, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__gteInt__", 0, (duck_native_t)&duck_int_gte, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__lteInt__", 0, (duck_native_t)&duck_int_lte, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__neqInt__", 0, (duck_native_t)&duck_int_neq, int_type, 2, argv, argn);

    assert(hash_get_count(&int_type->name_lookup) == int_type->member_count+int_type->static_member_count);
}
