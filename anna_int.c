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
    int_type = anna_type_create(L"Int", 64);
    anna_stack_declare(stack, L"Int", type_type, int_type->wrapper);
    anna_member_create(int_type, ANNA_MID_INT_PAYLOAD,  L"!intPayload", 0, null_type);    

    anna_int_type_i_create(stack);
    anna_int_one = anna_int_create(1);
}
