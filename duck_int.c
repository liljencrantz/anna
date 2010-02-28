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

void duck_int_type_create(duck_stack_frame_t *stack)
{
    int_type = duck_type_create(L"Int", 64);
    duck_stack_declare(stack, L"Int", type_type, int_type->wrapper);
    duck_member_create(int_type, DUCK_MID_INT_PAYLOAD,  L"!intPayload", 0, null_type);    
    duck_int_type_i_create(stack);
    duck_int_one = duck_int_create(1);
}
