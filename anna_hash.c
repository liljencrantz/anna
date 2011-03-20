#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_hash.h"
#include "anna_int.h"
#include "anna_stack.h"
#include "anna_type.h"
#include "anna_member.h"
#include "anna_function_type.h"
#include "anna_function.h"
#include "anna_range.h"
#include "anna_vm.h"

#include "anna_macro.h"

static hash_table_t anna_hash_specialization;
struct 
{
    anna_type_t type1;
    anna_type_t type2;
}
    tt_t;


anna_object_t *anna_hash_create(anna_type_t *spec)
{
    anna_object_t *obj= anna_object_create(anna_hash_type_get(spec));
    (*anna_member_addr_get_mid(obj,ANNA_MID_HASH_PAYLOAD))=0;
    (*(size_t *)anna_member_addr_get_mid(obj,ANNA_MID_HASH_CAPACITY)) = 0;    
    (*(size_t *)anna_member_addr_get_mid(obj,ANNA_MID_HASH_SIZE)) = 0;
    return obj;
}

anna_object_t *anna_hash_create2(anna_type_t *hash_type)
{
    anna_object_t *obj= anna_object_create(hash_type);
    (*anna_member_addr_get_mid(obj,ANNA_MID_HASH_PAYLOAD))=0;
    (*(size_t *)anna_member_addr_get_mid(obj,ANNA_MID_HASH_CAPACITY)) = 0;    
    (*(size_t *)anna_member_addr_get_mid(obj,ANNA_MID_HASH_SIZE)) = 0;
    return obj;
}

static anna_type_t *anna_hash_get_specialization(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_member_addr_get_mid(
		 obj,
		 ANNA_MID_HASH_SPECIALIZATION));    
}

static ssize_t calc_offset(ssize_t offset, size_t size)
{
    if(offset < 0) {
	return size-offset;
    }
    return offset;
}

void anna_hash_set(struct anna_object *this, ssize_t offset, struct anna_object *value)
{
    size_t size = anna_hash_get_size(this);
    ssize_t pos = calc_offset(offset, size);
    //wprintf(L"Set el %d in hash of %d elements\n", pos, size);
    if(pos < 0)
    {
	return;
    }
    if(pos >= size)
    {
//	wprintf(L"Set new size\n");
	anna_hash_set_size(this, pos+1);      
    }
    
    anna_object_t **ptr = anna_hash_get_payload(this);
    ptr[pos] = value;  
}

anna_object_t *anna_hash_get(anna_object_t *this, ssize_t offset)
{
    size_t size = anna_hash_get_size(this);
    ssize_t pos = calc_offset(offset, size);
    if(pos < 0||pos >=size)
    {
	return null_object;
    }
    anna_object_t **ptr = anna_hash_get_payload(this);
    return ptr[pos];
}

void anna_hash_add(struct anna_object *this, struct anna_object *value)
{
    size_t capacity = anna_hash_get_capacity(this);
    size_t size = anna_hash_get_size(this);
    anna_hash_set(this, size, value);
}

size_t anna_hash_get_size(anna_object_t *this)
{
    assert(this);
    return *(size_t *)anna_member_addr_get_mid(this,ANNA_MID_HASH_SIZE);
}

void anna_hash_set_size(anna_object_t *this, size_t sz)
{
    size_t old_size = anna_hash_get_size(this);
    size_t capacity = anna_hash_get_capacity(this);
    
    if(sz>old_size)
    {
	if(sz>capacity)
	{
	    anna_hash_set_capacity(this, sz);
	}
	anna_object_t **ptr = anna_hash_get_payload(this);
	int i;
	for(i=old_size; i<sz; i++)
	{
	    ptr[i] = null_object;
	}
    }
    *(size_t *)anna_member_addr_get_mid(this,ANNA_MID_HASH_SIZE) = sz;
}

size_t anna_hash_get_capacity(anna_object_t *this)
{
    return *(size_t *)anna_member_addr_get_mid(this,ANNA_MID_HASH_CAPACITY);
}

void anna_hash_set_capacity(anna_object_t *this, size_t sz)
{
    anna_object_t **ptr = anna_hash_get_payload(this);
    ptr = realloc(ptr, sizeof(anna_object_t *)*sz);
    if(!ptr)
    {
	CRASH;
    }    
    (*(size_t *)anna_member_addr_get_mid(this,ANNA_MID_HASH_CAPACITY)) = sz;
    *(anna_object_t ***)anna_member_addr_get_mid(this,ANNA_MID_HASH_PAYLOAD) = ptr;
}

anna_object_t **anna_hash_get_payload(anna_object_t *this)
{
    return *(anna_object_t ***)anna_member_addr_get_mid(this,ANNA_MID_HASH_PAYLOAD);
}

static anna_object_t *anna_hash_set_int(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    anna_hash_set(param[0], anna_int_get(param[1]), param[2]);
    return param[2];
}

static anna_object_t *anna_hash_get_int(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    return anna_hash_get(param[0], anna_int_get(param[1]));
}

static anna_object_t *anna_hash_get_count(anna_object_t **param)
{
    return anna_int_create(anna_hash_get_size(param[0]));
}

static anna_object_t *anna_hash_set_count(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    int sz = anna_int_get(param[1]);
    anna_hash_set_size(param[0], sz);
    return param[1];
}

static anna_object_t *anna_hash_append(anna_object_t **param)
{
    size_t i;

    size_t capacity = anna_hash_get_capacity(param[0]);
    size_t size = anna_hash_get_size(param[0]);
    size_t size2 = anna_hash_get_size(param[1]);
    size_t new_size = size+size2;
    
    if(capacity <= (new_size))
    {
	anna_hash_set_capacity(param[0], maxi(8, new_size*2));
    }
    anna_object_t **ptr = anna_hash_get_payload(param[0]);
    anna_object_t **ptr2 = anna_hash_get_payload(param[1]);
    *(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_HASH_SIZE) = new_size;
    for(i=0; i<size2; i++)
    {
	ptr[size+i]=ptr2[i];
    }
    
    return param[0];
}

static anna_object_t *anna_hash_each(anna_object_t **param)
{
    anna_object_t *body_object=param[1];
    
    size_t sz = anna_hash_get_size(param[0]);
    anna_object_t **arr = anna_hash_get_payload(param[0]);
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

static anna_object_t *anna_hash_map(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=anna_hash_create(object_type);
    body_object=param[1];
        
    size_t sz = anna_hash_get_size(param[0]);
    anna_object_t **arr = anna_hash_get_payload(param[0]);
    size_t i;
    anna_hash_set_size(result, sz);

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
	anna_hash_set(result, i, obj);
    }
    return result;
}

static anna_object_t *anna_hash_filter(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=anna_hash_create(anna_hash_get_specialization(param[0]));
    body_object=param[1];
        
    size_t sz = anna_hash_get_size(param[0]);
    anna_object_t **arr = anna_hash_get_payload(param[0]);
    size_t i;
    int pos=0;
    
    anna_hash_set_capacity(result, sz);
    
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
	    anna_hash_set(result, pos++, arr[i]);
    }
    anna_hash_set_size(result, pos);
    return result;
}

static anna_object_t *anna_hash_first(anna_object_t **param)
{
    anna_object_t *body_object=param[1];
    size_t sz = anna_hash_get_size(param[0]);
    anna_object_t **arr = anna_hash_get_payload(param[0]);
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

static anna_object_t *anna_hash_init(anna_object_t **param)
{
    (*anna_member_addr_get_mid(param[0],ANNA_MID_HASH_PAYLOAD))=0;
    (*(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_HASH_CAPACITY)) = 0;    
    (*(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_HASH_SIZE)) = 0;

    size_t sz = anna_hash_get_size(param[1]);
    anna_object_t **src = anna_hash_get_payload(param[1]);

    anna_hash_set_size(param[0], sz);
    anna_object_t **dest = anna_hash_get_payload(param[0]);
    memcpy(dest, src, sizeof(anna_object_t *)*sz);
    
    return param[0];
}

static anna_object_t *anna_hash_del(anna_object_t **param)
{
    free((*anna_member_addr_get_mid(param[0],ANNA_MID_HASH_PAYLOAD)));
    (*(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_HASH_CAPACITY)) = 0;    
    (*(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_HASH_SIZE)) = 0;
    return param[0];
}

static anna_object_t *anna_hash_in(anna_object_t **param)
{
    size_t sz = anna_hash_get_size(param[0]);
    anna_object_t **arr = anna_hash_get_payload(param[0]);
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

static anna_object_t *anna_hash_i_get_range(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    
    int from = anna_range_get_from(param[1]);
    int step = anna_range_get_step(param[1]);
    int count = anna_range_get_count(param[1]);
    int i;
    
    anna_object_t *res = anna_hash_create(anna_hash_get_specialization(param[0]));
    anna_hash_set_capacity(res, count);
    for(i=0;i<count;i++)
    {
	anna_hash_set(
	    res, i, 
	    anna_hash_get(
		param[0],
		from + step*i));
	
    }    

    return res;
    
}

static anna_object_t *anna_hash_i_set_range(anna_object_t **param)
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
    
    int count2 = anna_hash_get_size(param[2]);

    if(count != count2)
    {
	if(step != 1)
	{
	    return null_object;
	}

	int old_size = anna_hash_get_size(param[0]);

	/* If we're assigning past the end of the array, just silently
	 * take the whole array and go on */
	count = mini(count, old_size - from);	
	int new_size = old_size - count + count2;
	anna_object_t **arr;
	if(to > new_size)
	{
	    anna_hash_set_capacity(param[0], to);
	    for(i=old_size; i<new_size; i++)
	    {
		anna_hash_set(
		    param[0], i, null_object);		
	    }
	    *(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_HASH_SIZE) = new_size;
	    arr = anna_hash_get_payload(param[0]);
	}
	else
	{
	    if(new_size > anna_hash_get_capacity(param[0]))
	    {
		anna_hash_set_capacity(param[0], new_size);
	    }
	    
	    *(size_t *)anna_member_addr_get_mid(param[0],ANNA_MID_HASH_SIZE) = new_size;
	    arr = anna_hash_get_payload(param[0]);
	    memmove(&arr[from+count2], &arr[from+count], sizeof(anna_object_t *)*abs(old_size - from - count ));
	}
	
	/* Set new size - don't call anna_hash_set_size, since that might truncate the hash if we're shrinking */

	/* Move the old data */

	/* Copy in the new data */
	for(i=0;i<count2;i++)
	{
	    arr[from+i] = 
		anna_hash_get(
		    param[2],
		    i);
	}
    }
    else
    {
	for(i=0;i<count;i++)
	{
	    anna_hash_set(
		param[0], from + step*i, 
		anna_hash_get(
		    param[2],
		    i));
	}
    }

    return param[0];
}

static void anna_hash_type_create_internal(
    anna_stack_template_t *stack,
    anna_type_t *type, 
    anna_type_t *spec1,
    anna_type_t *spec2)
{
    mid_t mmid;
    anna_function_t *fun;

    anna_member_create(
	type, ANNA_MID_HASH_PAYLOAD,  L"!hashPayload",
	0, null_type);

    anna_member_create(
	type, ANNA_MID_HASH_SPECIALIZATION1,  L"!hashSpecialization1",
	1, null_type);

    anna_member_create(
	type, ANNA_MID_HASH_SPECIALIZATION1,  L"!hashSpecialization2",
	1, null_type);

    anna_member_create(
	type, ANNA_MID_HASH_EQ_MID,  L"!eqMid",
	1, null_type);
    
    (*(anna_type_t **)anna_static_member_addr_get_mid(type,ANNA_MID_HASH_SPECIALIZATION)1) = spec1;
    (*(anna_type_t **)anna_static_member_addr_get_mid(type,ANNA_MID_HASH_SPECIALIZATION)2) = spec2;
    
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
	&anna_hash_init, 
	type,
	2, a_argv, a_argn);
    
    anna_native_method_create(
	type,
	ANNA_MID_DEL,
	L"__del__",
	0,
	&anna_hash_del, 
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
	&anna_hash_get_int, 
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
	&anna_hash_set_int, 
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
	&anna_hash_get_count, 
	&anna_hash_set_count);

    anna_native_method_create(
	type, -1, L"__appendAssign__", 0, 
	&anna_hash_append, 
	type,
	2, l_argv, l_argn);
    
    anna_type_t *fun_type = anna_function_type_each_create(
	L"!HashIterFunction", spec);

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
	&anna_hash_each, 
	type,
	2, e_argv, e_argn);
    
    anna_native_method_create(
	type, -1, L"__filter__", 
	0, &anna_hash_filter, 
	type,
	2, e_argv, e_argn);

    anna_native_method_create(
	type, -1, L"__first__", 
	0, &anna_hash_first, 
	spec,
	2, e_argv, e_argn);  

    /*
      FIXME: It would be nice if map returned something other than
      Hash<Object>. I guess map needs to be a template function or
      something.
    */
    anna_native_method_create(
	type, -1, L"__map__", 
	0, &anna_hash_map, 
	hash_type,
	2, e_argv, e_argn);
    
    anna_native_method_create(
	type, -1, L"__in__", 0, 
	&anna_hash_in, 
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
	&anna_hash_i_get_range, 
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
	&anna_hash_i_set_range, 
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

static hash_tt_func(void *data)
{
    tt_t *tt = (tt_t *)data;
    return tt->type1 + tt->type2;
}


static hash_tt_cmp(void *data1, void *data2)
{
    tt_t *tt1 = (tt_t *)data1;
    tt_t *tt2 = (tt_t *)data2;
    return (tt1->type1 == tt2->type1) && (tt1->type2 == tt2->type2);    
}

static inline void anna_hash_internal_init()
{
    static init = 0;
    if(likely(init))
	return;
    init=1;
    hash_init(&anna_hash_specialization, hash_tt_func, hash_tt_cmp);
}

tt_t *make_tt(anna_type_t *type1, anna_type_t *type2)
{
    tt_t *tt = calloc(2, sizeof(anna_type_t));
    tt->type1 = type1;
    tt->type2 = type2;
    return tt;
}

void anna_hash_type_create(anna_stack_template_t *stack)
{
    anna_hash_internal_init();
    hash_put(&anna_hash_specialization, make_tt(object_type, object_type), hash_type);
    anna_hash_type_create_internal(stack, hash_type, object_type, object_type);
}

anna_type_t *anna_hash_type_get(anna_type_t *subtype1, anna_type_t *subtype2)
{
    anna_hash_internal_init();
    tt_t tt = 
	{
	    subtype1, subtype2
	}
    ;
    
    anna_type_t *spec = hash_get(&anna_hash_specialization, &tt);
    if(!spec)
    {
	string_buffer_t sb = SB_STATIC;
	sb_printf(&sb, L"HashMap<%ls,%ls>", subtype1->name, subtype2->name);
	spec = anna_type_native_create(sb_content(&sb), stack_global);
	sb_destroy(&sb);
	hash_put(&anna_hash_specialization, make_tt(subtype1, subtype2), spec);
	anna_hash_type_create_internal(stack_global, spec, subtype1, subtype2);
    }
    return spec;
}




