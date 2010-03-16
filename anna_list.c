#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_list.h"
#include "anna_int.h"
#include "anna_stack.h"

#include "anna_macro.h"

static anna_object_t **anna_list_get_payload(anna_object_t *this);

anna_object_t *anna_list_create()
{
  return anna_object_create(list_type);
}

ssize_t calc_offset(ssize_t offset, size_t size)
{
  if(offset < 0) {
    return size-offset;
  }
  return offset;
}

void anna_list_set(struct anna_object *this, ssize_t offset, struct anna_object *value)
{
    size_t size = anna_list_get_size(this);
    ssize_t pos = calc_offset(offset, size);
    //wprintf(L"Set el %d in list of %d elements\n", pos, size);
    if(pos < 0)
    {
	return;
    }
    if(pos >= size)
    {
//	wprintf(L"Set new size\n");
	anna_list_set_size(this, pos+1);      
    }
    
    anna_object_t **ptr = anna_list_get_payload(this);
    ptr[pos] = value;  
}

anna_object_t *anna_list_get(anna_object_t *this, ssize_t offset)
{
  size_t size = anna_list_get_size(this);
  ssize_t pos = calc_offset(offset, size);
  if(pos < 0||pos >=size)
    {
      return null_object;
    }
  anna_object_t **ptr = anna_list_get_payload(this);
  return ptr[pos];
}

void anna_list_add(struct anna_object *this, struct anna_object *value)
{
  size_t capacity = anna_list_get_capacity(this);
  size_t size = anna_list_get_size(this);
  if(capacity == size)
    {
      anna_list_set_capacity(this, maxi(8, 2*capacity));
    }
  anna_object_t **ptr = anna_list_get_payload(this);
  anna_list_set_size(this, size+1);
  ptr[size]=value;
}

size_t anna_list_get_size(anna_object_t *this)
{
  assert(this);
  return *(size_t *)anna_member_addr_get_mid(this,ANNA_MID_LIST_SIZE);
}

void anna_list_set_size(anna_object_t *this, size_t sz)
{
  size_t old_size = anna_list_get_size(this);
  size_t capacity = anna_list_get_capacity(this);
  
  if(sz>old_size)
  {
      if(sz>capacity)
	{
	  anna_list_set_capacity(this, sz);
	}
      anna_object_t **ptr = anna_list_get_payload(this);
      int i;
      for(i=old_size; i<sz; i++)
	{
	  ptr[i] = null_object;
	}
  }
  *(size_t *)anna_member_addr_get_mid(this,ANNA_MID_LIST_SIZE) = sz;
}

size_t anna_list_get_capacity(anna_object_t *this)
{
    return *(size_t *)anna_member_addr_get_mid(this,ANNA_MID_LIST_CAPACITY);
}

void anna_list_set_capacity(anna_object_t *this, size_t sz)
{
    anna_object_t **ptr = anna_list_get_payload(this);
    ptr = realloc(ptr, sizeof(anna_object_t *)*sz);
    assert(ptr);
    *(size_t *)anna_member_addr_get_mid(this,ANNA_MID_LIST_CAPACITY) = sz;
    *(anna_object_t ***)anna_member_addr_get_mid(this,ANNA_MID_LIST_PAYLOAD) = ptr;
}

static anna_object_t **anna_list_get_payload(anna_object_t *this)
{
    return *(anna_object_t ***)anna_member_addr_get_mid(this,ANNA_MID_LIST_PAYLOAD);
}

static anna_object_t *anna_list_set_int(anna_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
  anna_list_set(param[0], anna_int_get(param[1]), param[2]);
  return param[2];
}

static anna_object_t *anna_list_get_int(anna_object_t **param)
{
  if(param[1]==null_object)
    return null_object;
    return anna_list_get(param[0], anna_int_get(param[1]));
}

static anna_object_t *anna_list_append(anna_object_t **param)
{
    anna_list_add(param[0], param[1]);
    return param[1];
}

static anna_object_t *anna_list_each_value(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=null_object;
    body_object=param[1];
        
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;

    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_stack_frame_t *stack = stack_ptr?*stack_ptr:0;
    assert(function_ptr);
/*
    wprintf(L"each loop got function %ls\n", (*function_ptr)->name);
    wprintf(L"with param %ls\n", (*function_ptr)->input_name[0]);
*/  
    for(i=0;i<sz;i++)
    {
      /*
      wprintf(L"Run the following code:\n");
      anna_node_print((*function_ptr)->body);
      wprintf(L"\n");
      */
	result = anna_function_invoke_values(*function_ptr, 0, &arr[i], stack);
    }
    return result;
}


static anna_object_t *anna_list_each_pair(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=null_object;
    body_object=param[1];
        
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;

    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_stack_frame_t *stack = stack_ptr?*stack_ptr:0;
    assert(function_ptr);
/*
    wprintf(L"each loop got function %ls\n", (*function_ptr)->name);
    wprintf(L"with param %ls\n", (*function_ptr)->input_name[0]);
*/  
    anna_object_t *o_param[2];
    for(i=0;i<sz;i++)
    {
      /*
      wprintf(L"Run the following code:\n");
      anna_node_print((*function_ptr)->body);
      wprintf(L"\n");
      */
	o_param[0] = anna_int_create(i);
	o_param[1] = arr[i];
	result = anna_function_invoke_values(*function_ptr, 0, o_param, stack);
    }
    return result;
}


static anna_object_t *anna_list_map_value(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=anna_list_create();
    
    body_object=param[1];
        
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;
    anna_list_set_size(result, sz);

    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_stack_frame_t *stack = stack_ptr?*stack_ptr:0;
    assert(function_ptr);
/*
    wprintf(L"each loop got function %ls\n", (*function_ptr)->name);
    wprintf(L"with param %ls\n", (*function_ptr)->input_name[0]);
*/  
    for(i=0;i<sz;i++)
    {
      /*
      wprintf(L"Run the following code:\n");
      anna_node_print((*function_ptr)->body);
      wprintf(L"\n");
      */
	anna_list_set(result, i, anna_function_invoke_values(*function_ptr, 0, &arr[i], stack));
    }
    return result;
}


static anna_object_t *anna_list_map_pair(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=anna_list_create();
    body_object=param[1];
        
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;
    anna_list_set_size(result, sz);

    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_stack_frame_t *stack = stack_ptr?*stack_ptr:0;
    assert(function_ptr);
/*
    wprintf(L"each loop got function %ls\n", (*function_ptr)->name);
    wprintf(L"with param %ls\n", (*function_ptr)->input_name[0]);
*/  
    anna_object_t *o_param[2];
    for(i=0;i<sz;i++)
    {
      /*
      wprintf(L"Run the following code:\n");
      anna_node_print((*function_ptr)->body);
      wprintf(L"\n");
      */
	o_param[0] = anna_int_create(i);
	o_param[1] = arr[i];
	anna_list_set(result, i, anna_function_invoke_values(*function_ptr, 0, o_param, stack));
    }
    return result;
}


static anna_object_t *anna_list_filter_value(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=anna_list_create();
    
    body_object=param[1];
        
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;
    int pos=0;
    anna_list_set_capacity(result, sz);
    
    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_stack_frame_t *stack = stack_ptr?*stack_ptr:0;
    assert(function_ptr);

/*
    wprintf(L"each loop got function %ls\n", (*function_ptr)->name);
    wprintf(L"with param %ls\n", (*function_ptr)->input_name[0]);
*/  
    for(i=0;i<sz;i++)
    {
      /*
      wprintf(L"Run the following code:\n");
      anna_node_print((*function_ptr)->body);
      wprintf(L"\n");
      */
	if(anna_function_invoke_values(*function_ptr, 0, &arr[i], stack) != null_object)
	    anna_list_set(result, pos++, arr[i]);
    }
    anna_list_set_capacity(result, pos);
    return result;
}


static anna_object_t *anna_list_filter_pair(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=anna_list_create();
    body_object=param[1];
        
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;
    int pos=0;
    
    anna_list_set_capacity(result, sz);
    
    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_stack_frame_t *stack = stack_ptr?*stack_ptr:0;
    assert(function_ptr);
/*
    wprintf(L"each loop got function %ls\n", (*function_ptr)->name);
    wprintf(L"with param %ls\n", (*function_ptr)->input_name[0]);
*/  
    anna_object_t *o_param[2];
    for(i=0;i<sz;i++)
    {
      /*
      wprintf(L"Run the following code:\n");
      anna_node_print((*function_ptr)->body);
      wprintf(L"\n");
      */
	o_param[0] = anna_int_create(i);
	o_param[1] = arr[i];
	if(anna_function_invoke_values(*function_ptr, 0, o_param, stack) != null_object)
	    anna_list_set(result, pos++, arr[i]);
    }
    anna_list_set_size(result, pos);
    return result;
}

static anna_object_t *anna_list_first_value(anna_object_t **param)
{
    anna_object_t *body_object=param[1];
    
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;

    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_stack_frame_t *stack = stack_ptr?*stack_ptr:0;
    assert(function_ptr);
    for(i=0;i<sz;i++)
    {
       if(anna_function_invoke_values(*function_ptr, 0, &arr[i], stack) != null_object)
       {
	  return arr[i];
       }
       
    }
    return null_object;
}


static anna_object_t *anna_list_first_pair(anna_object_t **param)
{
    anna_object_t *body_object=param[1];
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;
    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_stack_frame_t *stack = stack_ptr?*stack_ptr:0;
    anna_object_t *o_param[2];    
    assert(function_ptr);
    
    for(i=0;i<sz;i++)
    {
	o_param[0] = anna_int_create(i);
	o_param[1] = arr[i];
	if(anna_function_invoke_values(*function_ptr, 0, o_param, stack) != null_object)
	   return arr[i];
    }
    return null_object;
}

static anna_object_t *anna_list_init(anna_object_t **param)
{
    size_t sz = anna_list_get_size(param[1]);
    anna_object_t **src = anna_list_get_payload(param[1]);
    anna_list_set_size(param[0], sz);
    anna_object_t **dest = anna_list_get_payload(param[0]);
    memcpy(dest, src, sizeof(anna_object_t *)*sz);
    
    return param[0];
}



void anna_list_type_create(anna_stack_frame_t *stack)
{
    list_type = anna_type_create(L"List", 64, 0);
    anna_stack_declare(stack, L"List", type_type, list_type->wrapper);

    anna_location_t loc=
	{
	    0,0,0,0,0
	}
    ;

    anna_function_t *func;

    func = anna_native_create(L"!anonymous",
			      ANNA_FUNCTION_MACRO,
			      (anna_native_t)anna_list_map_pair,
			      0,
			      0,
			      0, 
			      0);
    func->stack_template=stack;
				
    anna_node_call_t *definition = 
	anna_node_call_create(
	    &loc,
	    (anna_node_t *)anna_node_identifier_create(&loc, L"__block__"),
	    0,
	    0);
    
    anna_node_call_t *full_definition = 
	anna_node_call_create(
	    &loc,
	    (anna_node_t *)anna_node_identifier_create(&loc, L"__type__"),
	    0,
	    0);

    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)anna_node_identifier_create(
	    &loc,
	    L"List"));
    
    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)anna_node_identifier_create(
	    &loc,
	    L"class"));
    
    /*
      Attibute list
     */
    anna_node_call_t *attribute_list = 
	anna_node_call_create(
	    &loc,
	    (anna_node_t *)anna_node_identifier_create(&loc, L"__block__"),
	    0,
	    0);	
    
    anna_node_call_t *template = 
	anna_node_call_create(
	    &loc,
	    (anna_node_t *)anna_node_identifier_create(&loc, L"template"),
	    0,
	    0);	
    
    anna_node_call_t *pair = 
	anna_node_call_create(
	    &loc,
	    (anna_node_t *)anna_node_identifier_create(&loc, L"Pair"),
	    0,
	    0);	
    
    anna_node_call_add_child(
	pair,
	(anna_node_t *)anna_node_identifier_create(
	    &loc,
	    L"T"));
    
    anna_node_call_add_child(
	pair,
	(anna_node_t *)anna_node_identifier_create(
	    &loc,
	    L"Object"));
    
    anna_node_call_add_child(
	template,
	(anna_node_t *)pair);
    
    anna_node_call_add_child(
	attribute_list,
	(anna_node_t *)template);
    

    anna_node_call_add_child(
	full_definition,	
	(anna_node_t *)attribute_list);
    
    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)definition);
    
    list_type->definition = full_definition;

    anna_member_add_node(definition, ANNA_MID_LIST_PAYLOAD,  L"!listPayload", 0, (anna_node_t *)anna_node_identifier_create(&loc, L"Null") );
    anna_member_add_node(definition, ANNA_MID_LIST_SIZE,  L"!listSize", 0, (anna_node_t *)anna_node_identifier_create(&loc, L"Null") );
    anna_member_add_node(definition, ANNA_MID_LIST_CAPACITY,  L"!listCapacity", 0, (anna_node_t *)anna_node_identifier_create(&loc, L"Null") );
    
    anna_node_t *i_argv[] = 
	{
	  (anna_node_t *)anna_node_identifier_create(&loc, L"List"),
	  (anna_node_t *)anna_node_identifier_create(&loc, L"Int"),
	  (anna_node_t *)anna_node_identifier_create(&loc, L"T")
	}
    ;
    wchar_t *i_argn[]=
	{
	    L"this", L"index", L"value"
	}
    ;
    
    anna_node_t *a_argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(&loc, L"List"),
	    (anna_node_t *)anna_node_identifier_create(&loc, L"T")
	}
    ;
    wchar_t *a_argn[]=
	{
	    L"this", L"value"
	}
    ;

    anna_node_t *e_method_value_argv[] = 
       {
	  (anna_node_t *)anna_node_identifier_create(&loc, L"T")
       }
    ;

    wchar_t *e_method_value_argn[] = 
       {
	  L"value"
       }
    ;

    anna_node_t *e_method_pair_argv[] = 
       {
	  (anna_node_t *)anna_node_identifier_create(&loc, L"Int"),
	  (anna_node_t *)anna_node_identifier_create(&loc, L"T")
       }
    ;

    wchar_t *e_method_pair_argn[] = 
	{
	    L"index",
	    L"value"
	}
    ;

    anna_node_t *e_argv_value[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(&loc, L"List"),
	    anna_node_function_declaration_create(&loc, (anna_node_t *)anna_node_identifier_create(&loc, L"Object"), 1, e_method_value_argv, e_method_value_argn)
	}
    ;
    anna_node_t *e_argv_pair[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(&loc, L"List"),
	    anna_node_function_declaration_create(&loc, (anna_node_t *)anna_node_identifier_create(&loc, L"Object"), 2, e_method_pair_argv, e_method_pair_argn)
	}
    ;

    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;
    
    anna_node_t *list_template_param[] = 
       {
	  (anna_node_t *)anna_node_identifier_create(&loc, L"T")
       }
    ;
    
    anna_node_t *my_list_type = anna_node_templated_type_create(
       &loc, 
       (anna_node_t *)anna_node_identifier_create(&loc, L"List"),
       1,
       list_template_param);


    
    anna_native_method_add_node(
	definition,
	-1,
	L"__init__",
	ANNA_FUNCTION_VARIADIC, 
	(anna_native_t)&anna_list_init, 
	(anna_node_t *)anna_node_identifier_create(&loc, L"Null") , 
	2, a_argv, a_argn);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__get__Int__",
	0, 
	(anna_native_t)&anna_list_get_int, 
	(anna_node_t *)anna_node_identifier_create(&loc, L"T") , 
	2, 
	i_argv, 
	i_argn);
    
    anna_native_method_add_node(
	definition, 
	-1,
	L"__set__Int__", 
	0, 
	(anna_native_t)&anna_list_set_int, 
	(anna_node_t *)anna_node_identifier_create(&loc, L"T"), 
	3,
	i_argv, 
	i_argn);

    anna_native_method_add_node(
	definition, -1, L"__appendValue__", 0, 
	(anna_native_t)&anna_list_append, 
	(anna_node_t *)anna_node_identifier_create(&loc, L"T") , 
	2, a_argv, a_argn);
    
    anna_native_method_add_node(
	definition, -1, L"__eachValue__", 0, 
	(anna_native_t)&anna_list_each_value, 
	(anna_node_t *)anna_node_identifier_create(&loc, L"T"),
	2, e_argv_value, e_argn);
    
    anna_native_method_add_node(
	definition, -1, L"__eachPair__", 0, 
	(anna_native_t)&anna_list_each_pair, 
	(anna_node_t *)anna_node_identifier_create(&loc, L"T"), 
	2, e_argv_pair, e_argn);
    
    /*
      FIXME: This is the wrong return type for map - we need to check
      the return type of the function argument and do a cast, or
      something...
     */
    anna_native_method_add_node(
       definition, -1, L"__mapValue__",
       0, (anna_native_t)&anna_list_map_value,
       anna_node_clone_deep(my_list_type) ,
       2, e_argv_value, e_argn);

    anna_native_method_add_node(
	definition, -1, L"__mapPair__", 
	0, (anna_native_t)&anna_list_map_pair, 
	anna_node_clone_deep(my_list_type), 
	2, e_argv_pair, e_argn);
    
    anna_native_method_add_node(
	definition, -1, L"__filterValue__",
	0, (anna_native_t)&anna_list_filter_value,
	anna_node_clone_deep(my_list_type) ,
	2, e_argv_value, e_argn);

    anna_native_method_add_node(
	definition, -1, L"__filterPair__", 
	0, (anna_native_t)&anna_list_filter_pair, 
	anna_node_clone_deep(my_list_type), 
	2, e_argv_pair, e_argn);
    
    anna_native_method_add_node(
	definition, -1, L"__firstValue__",
	0, (anna_native_t)&anna_list_first_value,
	(anna_node_t *)anna_node_identifier_create(&loc, L"T"),
	2, e_argv_value, e_argn);

    anna_native_method_add_node(
	definition, -1, L"__firstPair__", 
	0, (anna_native_t)&anna_list_first_pair, 
	(anna_node_t *)anna_node_identifier_create(&loc, L"T"),
	2, e_argv_pair, e_argn);
    
    //anna_node_print(e_argv[1]);
    //anna_stack_print(func->stack_template);
    
    anna_macro_type_setup(list_type, func, 0);
        
    /*
    anna_native_method_add_node(definition, -1, L"__getslice__", 0, (anna_native_t)&anna_int_add, int_type, 2, argv, argn);
    anna_native_method_add_node(definition, -1, L"__setslice__", 0, (anna_native_t)&anna_int_add, int_type, 2, argv, argn);
    anna_native_method_add_node(definition, -1, L"__contains__", 0, (anna_native_t)&anna_int_add, int_type, 2, argv, argn);

      __add__, __sub__, __mul__ and friends.
      __select__, __first__, __last__
     */

  /*  
  anna_object_t *l = anna_list_create();
  anna_list_set(l, 3, L"TRALALA");
  anna_list_append(l, L"TJOHO");
  wprintf(L"%ls %ls\n", anna_list_get(l,3), anna_list_get(l,4));
  */
}
