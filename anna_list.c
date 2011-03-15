#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_list.h"
#include "anna_int.h"
#include "anna_stack.h"
#include "anna_type.h"
#include "anna_member.h"
#include "anna_function_type.h"
#include "anna_function.h"
#include "anna_range.h"
#include "anna_vm.h"

#include "anna_macro.h"

static hash_table_t anna_list_specialization;


anna_object_t *anna_list_create(anna_type_t *spec)
{
    anna_object_t *obj= anna_object_create(anna_list_type_get(spec));
    (*anna_member_addr_get_mid(obj,ANNA_MID_LIST_PAYLOAD))=0;
    (*(size_t *)anna_member_addr_get_mid(obj,ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_member_addr_get_mid(obj,ANNA_MID_LIST_SIZE)) = 0;
    return obj;
}

anna_object_t *anna_list_create2(anna_type_t *list_type)
{
    anna_object_t *obj= anna_object_create(list_type);
    (*anna_member_addr_get_mid(obj,ANNA_MID_LIST_PAYLOAD))=0;
    (*(size_t *)anna_member_addr_get_mid(obj,ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_member_addr_get_mid(obj,ANNA_MID_LIST_SIZE)) = 0;
    return obj;
}

static anna_type_t *anna_list_get_specialization(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_member_addr_get_mid(
		 obj,
		 ANNA_MID_LIST_SPECIALIZATION));    
}

static ssize_t calc_offset(ssize_t offset, size_t size)
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
    anna_list_set(this, size, value);
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
    if(!ptr)
    {
	CRASH;
    }    
    (*(size_t *)anna_member_addr_get_mid(this,ANNA_MID_LIST_CAPACITY)) = sz;
    *(anna_object_t ***)anna_member_addr_get_mid(this,ANNA_MID_LIST_PAYLOAD) = ptr;
}

anna_object_t **anna_list_get_payload(anna_object_t *this)
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

static anna_object_t *anna_list_get_count(anna_object_t **param)
{
    return anna_int_create(anna_list_get_size(param[0]));
}

static anna_object_t *anna_list_set_count(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    int sz = anna_int_get(param[1]);
    anna_list_set_size(param[0], sz);
    return param[1];
}

static anna_object_t *anna_list_append(anna_object_t **param)
{
    size_t i;

    size_t capacity = anna_list_get_capacity(param[0]);
    size_t size = anna_list_get_size(param[0]);
    size_t size2 = anna_list_get_size(param[1]);
    size_t new_size = size+size2;
    
    if(capacity <= (new_size))
    {
	anna_list_set_capacity(param[0], maxi(8, new_size*2));
    }
    anna_object_t **ptr = anna_list_get_payload(param[0]);
    anna_object_t **ptr2 = anna_list_get_payload(param[1]);
    *(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_LIST_SIZE) = new_size;
    for(i=0; i<size2; i++)
    {
	ptr[size+i]=ptr2[i];
    }
    
    return param[0];
}

static anna_object_t *anna_list_each(anna_object_t **param)
{
    anna_object_t *body_object=param[1];
    
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;

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
	anna_vm_run(body_object, 2, o_param);
    }
    return param[0];
}

static anna_object_t *anna_list_map(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=anna_list_create(object_type);
    body_object=param[1];
        
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;
    anna_list_set_size(result, sz);

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
	anna_object_t *obj = anna_vm_run(body_object, 2, o_param);
	anna_list_set(result, i, obj);
    }
    return result;
}

static anna_object_t *anna_list_filter(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=anna_list_create(anna_list_get_specialization(param[0]));
    body_object=param[1];
        
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;
    int pos=0;
    
    anna_list_set_capacity(result, sz);
    
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
	anna_object_t *ret = anna_vm_run(body_object, 2, o_param);
	if(ret != null_object)
	    anna_list_set(result, pos++, arr[i]);
    }
    anna_list_set_size(result, pos);
    return result;
}

static anna_object_t *anna_list_first(anna_object_t **param)
{
    anna_object_t *body_object=param[1];
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;
    anna_object_t *o_param[2];    
    
    for(i=0;i<sz;i++)
    {
	o_param[0] = anna_int_create(i);
	o_param[1] = arr[i];
	anna_object_t *ret = anna_vm_run(body_object, 2, o_param);
	if(ret != null_object)
	    return arr[i];
    }
    return null_object;
}

static anna_object_t *anna_list_init(anna_object_t **param)
{
    (*anna_member_addr_get_mid(param[0],ANNA_MID_LIST_PAYLOAD))=0;
    (*(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_LIST_SIZE)) = 0;

    size_t sz = anna_list_get_size(param[1]);
    anna_object_t **src = anna_list_get_payload(param[1]);

    anna_list_set_size(param[0], sz);
    anna_object_t **dest = anna_list_get_payload(param[0]);
    memcpy(dest, src, sizeof(anna_object_t *)*sz);
    
    return param[0];
}

static anna_object_t *anna_list_del(anna_object_t **param)
{
    free((*anna_member_addr_get_mid(param[0],ANNA_MID_LIST_PAYLOAD)));
    (*(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_LIST_SIZE)) = 0;
    return param[0];
}

static anna_object_t *anna_list_in(anna_object_t **param)
{
    size_t sz = anna_list_get_size(param[0]);
    anna_object_t **arr = anna_list_get_payload(param[0]);
    size_t i;

    anna_object_t *needle = param[1];
    if(needle == null_object)
    {
	return null_object;
    }
    //anna_object_print(needle);
    
    anna_object_t **eq_obj_ptr = anna_member_addr_get_mid(needle, ANNA_MID_EQ);
    if(eq_obj_ptr)
    {
	for(i=0;i<sz;i++)
	{
	    anna_object_t *o_param[]=
		{
		    needle,
		    arr[i]
		}
	    ;
	    anna_object_t *result = 
		anna_vm_run(*eq_obj_ptr, 2, o_param);
	    
	    if(result != null_object)
	    {
		return anna_int_create(i);
	    }
	}
    }
    return null_object;
}

static anna_object_t *anna_list_i_get_range(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    
    int from = anna_range_get_from(param[1]);
    int step = anna_range_get_step(param[1]);
    int count = anna_range_get_count(param[1]);
    int i;
    
    anna_object_t *res = anna_list_create(anna_list_get_specialization(param[0]));
    anna_list_set_capacity(res, count);
    for(i=0;i<count;i++)
    {
	anna_list_set(
	    res, i, 
	    anna_list_get(
		param[0],
		from + step*i));
	
    }    

    return res;
    
}

static anna_object_t *anna_list_i_set_range(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    
    if(param[2]==null_object)
	return null_object;
    
    int from = anna_range_get_from(param[1]);
    int step = anna_range_get_step(param[1]);
    int to = anna_range_get_to(param[1]);
    int count = anna_range_get_count(param[1]);
    int i;
    
    int count2 = anna_list_get_size(param[2]);

    if(count != count2)
    {
	if(step != 1)
	{
	    return null_object;
	}

	int old_size = anna_list_get_size(param[0]);

	/* If we're assigning past the end of the array, just silently
	 * take the whole array and go on */
	count = mini(count, old_size - from);	
	int new_size = old_size - count + count2;
	anna_object_t **arr;
	if(to > new_size)
	{
	    anna_list_set_capacity(param[0], to);
	    for(i=old_size; i<new_size; i++)
	    {
		anna_list_set(
		    param[0], i, null_object);		
	    }
	    *(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_LIST_SIZE) = new_size;
	    arr = anna_list_get_payload(param[0]);
	}
	else
	{
	    if(new_size > anna_list_get_capacity(param[0]))
	    {
		anna_list_set_capacity(param[0], new_size);
	    }
	    
	    *(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_LIST_SIZE) = new_size;
	    arr = anna_list_get_payload(param[0]);
	    memmove(&arr[from+count2], &arr[from+count], sizeof(anna_object_t *)*abs(old_size - from - count ));
	}
	
	/* Set new size - don't call anna_list_set_size, since that might truncate the list if we're shrinking */

	/* Move the old data */

	/* Copy in the new data */
	for(i=0;i<count2;i++)
	{
	    arr[from+i] = 
		anna_list_get(
		    param[2],
		    i);
	}
    }
    else
    {
	for(i=0;i<count;i++)
	{
	    anna_list_set(
		param[0], from + step*i, 
		anna_list_get(
		    param[2],
		    i));
	}
    }

    return param[0];
}

static void anna_list_type_create_internal(
    anna_stack_template_t *stack,
    anna_type_t *type, 
    anna_type_t *spec)
{
    mid_t mmid;
    anna_function_t *fun;

    anna_member_create(
	type, ANNA_MID_LIST_PAYLOAD,  L"!listPayload",
	0, null_type);
    anna_member_create(
	type, ANNA_MID_LIST_SIZE,  L"!listSize", 
	0, null_type);
    anna_member_create(
	type, ANNA_MID_LIST_CAPACITY,  L"!listCapacity",
	0, null_type);
    anna_member_create(
	type, ANNA_MID_LIST_SPECIALIZATION,  L"!listSpecialization",
	1, null_type);
    (*(anna_type_t **)anna_static_member_addr_get_mid(type,ANNA_MID_LIST_SPECIALIZATION)) = spec;
    
    anna_type_t *a_argv[] = 
	{
	    type,
	    spec
	}
    ;
    
    wchar_t *a_argn[]=
	{
	    L"this", L"value"
	}
    ;

    anna_type_t *l_argv[] = 
	{
	    type,
	    type
	}
    ;
    
    wchar_t *l_argn[]=
	{
	    L"this", L"value"
	}
    ;

    anna_native_method_create(
	type,
	-1,
	L"__init__",
	ANNA_FUNCTION_VARIADIC, 
	&anna_list_init, 
	type,
	2, a_argv, a_argn);
    
    anna_native_method_create(
	type,
	ANNA_MID_DEL,
	L"__del__",
	0,
	&anna_list_del, 
	object_type,
	1, a_argv, a_argn);    

    anna_type_t *i_argv[] = 
	{
	    type,
	    int_type,
	    spec
	}
    ;

    wchar_t *i_argn[]=
	{
	    L"this", L"index", L"value"
	}
    ;

    mmid = anna_native_method_create(
	type,
	-1,
	L"__get__Int__",
	0, 
	&anna_list_get_int, 
	spec,
	2, 
	i_argv, 
	i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(type, mmid));
    anna_function_alias_add(fun, L"__get__");
    
    mmid = anna_native_method_create(
	type, 
	-1,
	L"__set__Int__", 
	0, 
	&anna_list_set_int, 
	spec,
	3,
	i_argv, 
	i_argn);    
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(type, mmid));
    anna_function_alias_add(fun, L"__set__");
    
    anna_native_property_create(
	type,
	-1,
	L"count",
	int_type,
	&anna_list_get_count, 
	&anna_list_set_count);

    anna_native_method_create(
	type, -1, L"__appendAssign__", 0, 
	&anna_list_append, 
	type,
	2, l_argv, l_argn);
    
    anna_type_t *fun_type = anna_function_type_each_create(
	L"!ListIterFunction", spec);

    anna_type_t *e_argv[] = 
	{
	    type,
	    fun_type
	}
    ;
    
    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;    
    
    anna_native_method_create(
	type, -1, L"__each__", 0, 
	&anna_list_each, 
	type,
	2, e_argv, e_argn);
    
    anna_native_method_create(
	type, -1, L"__filter__", 
	0, &anna_list_filter, 
	type,
	2, e_argv, e_argn);

    anna_native_method_create(
	type, -1, L"__first__", 
	0, &anna_list_first, 
	spec,
	2, e_argv, e_argn);  

    /*
      FIXME: It would be nice if map returned something other than
      List<Object>. I guess map needs to be a template function or
      something.
    */
    anna_native_method_create(
	type, -1, L"__map__", 
	0, &anna_list_map, 
	list_type,
	2, e_argv, e_argn);
    
    anna_native_method_create(
	type, -1, L"__in__", 0, 
	&anna_list_in, 
	spec,
	2, a_argv, a_argn);
        
    anna_type_t *range_argv[] = 
	{
	    type,
	    range_type,
	    type
	}
    ;

    wchar_t *range_argn[] =
	{
	    L"this", L"range", L"value"
	}
    ;

    mmid = anna_native_method_create(
	type,
	-1,
	L"__get__Range__",
	0, 
	&anna_list_i_get_range, 
	type,
	2,
	range_argv, 
	range_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(type, mmid));
    anna_function_alias_add(fun, L"__get__");

    mmid = anna_native_method_create(
	type,
	-1,
	L"__set__Range__",
	0, 
	&anna_list_i_set_range, 
	type,
	3,
	range_argv, 
	range_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(type, mmid));
    anna_function_alias_add(fun, L"__set__");

    /*
      Todo:

      __add__, __sub__, __mul__ and friends.
      __select__, __first__, __last__
    */

}

void anna_list_type_create(anna_stack_template_t *stack)
{
    hash_init(&anna_list_specialization, hash_ptr_func, hash_ptr_cmp);
    hash_put(&anna_list_specialization, object_type, list_type);
    anna_list_type_create_internal(stack, list_type, object_type);
}

anna_type_t *anna_list_type_get(anna_type_t *subtype)
{
    anna_type_t *spec = hash_get(&anna_list_specialization, subtype);
    if(!spec)
    {
	string_buffer_t sb = SB_STATIC;
	sb_printf(&sb, L"List<%ls>", subtype->name);
	spec = anna_type_native_create(sb_content(&sb), stack_global);
	sb_destroy(&sb);
	hash_put(&anna_list_specialization, subtype, spec);
	anna_list_type_create_internal(stack_global, spec, subtype);
    }

    return spec;
}




