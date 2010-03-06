#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "duck.h"
#include "duck_node.h"
#include "duck_list.h"
#include "duck_int.h"
#include "duck_stack.h"

#include "duck_macro.h"

static duck_object_t **duck_list_get_payload(duck_object_t *this);

duck_object_t *duck_list_create()
{
  return duck_object_create(list_type);
}

ssize_t calc_offset(ssize_t offset, size_t size)
{
  if(offset < 0) {
    return size-offset;
  }
  return offset;
}

void duck_list_set(struct duck_object *this, ssize_t offset, struct duck_object *value)
{
    size_t size = duck_list_get_size(this);
    ssize_t pos = calc_offset(offset, size);
    //wprintf(L"Set el %d in list of %d elements\n", pos, size);
    if(pos < 0)
    {
	return;
    }
    if(pos >= size)
    {
//	wprintf(L"Set new size\n");
	duck_list_set_size(this, pos+1);      
    }
    
    duck_object_t **ptr = duck_list_get_payload(this);
    ptr[pos] = value;  
}

duck_object_t *duck_list_get(duck_object_t *this, ssize_t offset)
{
  size_t size = duck_list_get_size(this);
  ssize_t pos = calc_offset(offset, size);
  if(pos < 0||pos >=size)
    {
      return null_object;
    }
  duck_object_t **ptr = duck_list_get_payload(this);
  return ptr[pos];
}

void duck_list_add(struct duck_object *this, struct duck_object *value)
{
  size_t capacity = duck_list_get_capacity(this);
  size_t size = duck_list_get_size(this);
  if(capacity == size)
    {
      duck_list_set_capacity(this, maxi(8, 2*capacity));
    }
  duck_object_t **ptr = duck_list_get_payload(this);
  duck_list_set_size(this, size+1);
  ptr[size]=value;
}

size_t duck_list_get_size(duck_object_t *this)
{
  assert(this);
  return *(size_t *)duck_member_addr_get_mid(this,DUCK_MID_LIST_SIZE);
}

void duck_list_set_size(duck_object_t *this, size_t sz)
{
  size_t old_size = duck_list_get_size(this);
  size_t capacity = duck_list_get_capacity(this);
  
  if(sz>old_size)
  {
      if(sz>capacity)
	{
	  duck_list_set_capacity(this, sz);
	}
      duck_object_t **ptr = duck_list_get_payload(this);
      int i;
      for(i=old_size; i<sz; i++)
	{
	  ptr[i] = null_object;
	}
  }
  *(size_t *)duck_member_addr_get_mid(this,DUCK_MID_LIST_SIZE) = sz;
}

size_t duck_list_get_capacity(duck_object_t *this)
{
    return *(size_t *)duck_member_addr_get_mid(this,DUCK_MID_LIST_CAPACITY);
}

void duck_list_set_capacity(duck_object_t *this, size_t sz)
{
    duck_object_t **ptr = duck_list_get_payload(this);
    ptr = realloc(ptr, sizeof(duck_object_t *)*sz);
    assert(ptr);
    *(size_t *)duck_member_addr_get_mid(this,DUCK_MID_LIST_CAPACITY) = sz;
    *(duck_object_t ***)duck_member_addr_get_mid(this,DUCK_MID_LIST_PAYLOAD) = ptr;
}

static duck_object_t **duck_list_get_payload(duck_object_t *this)
{
    return *(duck_object_t ***)duck_member_addr_get_mid(this,DUCK_MID_LIST_PAYLOAD);
}

static duck_object_t *duck_list_set_int(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
  duck_list_set(param[0], duck_int_get(param[1]), param[2]);
  return param[2];
}

static duck_object_t *duck_list_get_int(duck_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
    return duck_list_get(param[0], duck_int_get(param[1]));
}

static duck_object_t *duck_list_append(duck_object_t **param)
{
    duck_list_add(param[0], param[1]);
    return param[1];
}

static duck_object_t *duck_list_each_value(duck_object_t **param)
{
    duck_object_t *body_object;
    duck_object_t *result=null_object;
    body_object=param[1];
        
    size_t sz = duck_list_get_size(param[0]);
    duck_object_t **arr = duck_list_get_payload(param[0]);
    size_t i;

    duck_function_t **function_ptr = (duck_function_t **)duck_member_addr_get_mid(body_object, DUCK_MID_FUNCTION_WRAPPER_PAYLOAD);
    duck_stack_frame_t **stack_ptr = (duck_stack_frame_t **)duck_member_addr_get_mid(body_object, DUCK_MID_FUNCTION_WRAPPER_STACK);
    assert(function_ptr);
/*
    wprintf(L"each loop got function %ls\n", (*function_ptr)->name);
    wprintf(L"with param %ls\n", (*function_ptr)->input_name[0]);
*/  
    for(i=0;i<sz;i++)
    {
      /*
      wprintf(L"Run the following code:\n");
      duck_node_print((*function_ptr)->body);
      wprintf(L"\n");
      */
	result = duck_function_invoke_values(*function_ptr, 0, &arr[i], stack_ptr?*stack_ptr:0);
    }
    return result;
}


void duck_list_type_create(duck_stack_frame_t *stack)
{
    list_type = duck_type_create(L"List", 64);
    duck_stack_declare(stack, L"List", type_type, list_type->wrapper);
    duck_member_create(list_type, DUCK_MID_LIST_PAYLOAD,  L"!listPayload", 0, null_type);
    duck_member_create(list_type, DUCK_MID_LIST_SIZE,  L"!listSize", 0, null_type);
    duck_member_create(list_type, DUCK_MID_LIST_CAPACITY,  L"!listCapacity", 0, null_type);
    
    duck_type_t *i_argv[] = 
	{
	    list_type, int_type, object_type
	}
    ;
    wchar_t *i_argn[]=
	{
	    L"this", L"index", L"value"
	}
    ;
    
    duck_type_t *a_argv[] = 
	{
	    list_type, object_type
	}
    ;
    wchar_t *a_argn[]=
	{
	    L"this", L"value"
	}
    ;
    
    duck_type_t *e_argv[] = 
	{
	    list_type, object_type
	}
    ;
    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;
    

    duck_native_method_create(list_type, -1, L"__getInt__", 0, (duck_native_t)&duck_list_get_int, object_type, 2, i_argv, i_argn);

    duck_native_method_create(list_type, -1, L"__setInt__", 0, (duck_native_t)&duck_list_set_int, object_type, 3, i_argv, i_argn);
    duck_native_method_create(list_type, -1, L"__append__", 0, (duck_native_t)&duck_list_append, object_type, 2, a_argv, a_argn);

    duck_native_method_create(list_type, -1, L"each", DUCK_FUNCTION_MACRO, (duck_native_t)&duck_list_each, 0, 0, 0, 0);
    duck_native_method_create(list_type, -1, L"__eachValue__", 0, (duck_native_t)&duck_list_each_value, object_type, 2, e_argv, e_argn);
    /*
    duck_native_method_create(list_type, -1, L"__getslice__", 0, (duck_native_t)&duck_int_add, int_type, 2, argv, argn);
    duck_native_method_create(list_type, -1, L"__setslice__", 0, (duck_native_t)&duck_int_add, int_type, 2, argv, argn);
    duck_native_method_create(list_type, -1, L"__contains__", 0, (duck_native_t)&duck_int_add, int_type, 2, argv, argn);

      __add__, __each__, __select__
     */

  /*  
  duck_object_t *l = duck_list_create();
  duck_list_set(l, 3, L"TRALALA");
  duck_list_append(l, L"TJOHO");
  wprintf(L"%ls %ls\n", duck_list_get(l,3), duck_list_get(l,4));
  */
}
