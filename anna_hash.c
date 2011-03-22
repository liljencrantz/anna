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
#include "anna_tt.h"

#include "anna_macro.h"

static hash_table_t anna_hash_specialization;

static inline hash_table_t *h_unwrap(anna_object_t *obj)
{
    return (hash_table_t *)anna_member_addr_get_mid(obj,ANNA_MID_HASH_PAYLOAD);
}

static int anna_hash_func(void *data)
{
    anna_object_t *param[] = {data};
    anna_object_t *fun_object = *anna_static_member_addr_get_mid(param[0]->type, ANNA_MID_HASH_CODE);
    anna_object_t *res = anna_vm_run(fun_object, 1, param);
    if(unlikely(res->type != int_type)){
	return 0;
    }
    return anna_int_get(res);
}

static int anna_hash_cmp(void *data1, void *data2)
{
    anna_object_t *param[] = {data1,data2};
    anna_object_t *fun_object = *anna_static_member_addr_get_mid(param[0]->type, ANNA_MID_EQ);
    anna_object_t *res = anna_vm_run(fun_object, 2, param);
    
    return res != null_object;
}

anna_object_t *anna_hash_create(anna_type_t *spec1, anna_type_t *spec2)
{
    anna_object_t *obj= anna_object_create(anna_hash_type_get(spec1, spec2));
    hash_init(h_unwrap(obj), anna_hash_func, anna_hash_cmp);
    return obj;
}

anna_object_t *anna_hash_create2(anna_type_t *hash_type)
{
    anna_object_t *obj= anna_object_create(hash_type);
    hash_init(h_unwrap(obj), anna_hash_func, anna_hash_cmp);
    return obj;
}

static anna_object_t *anna_hash_init(anna_object_t **param)
{
    hash_init(h_unwrap(param[0]), anna_hash_func, anna_hash_cmp);
/*
    size_t sz = anna_hash_get_size(param[1]);
    anna_object_t **src = anna_hash_get_payload(param[1]);

    anna_hash_set_size(param[0], sz);
    anna_object_t **dest = anna_hash_get_payload(param[0]);
    memcpy(dest, src, sizeof(anna_object_t *)*sz);
*/  
    return param[0];
}


static anna_object_t *anna_hash_set(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object; 
/*
    anna_type_print(param[0]->type);
    anna_object_print(param[0]);
    anna_object_print(param[1]);
    anna_object_print(param[2]);
*/
    hash_put(h_unwrap(param[0]), param[1], param[2]);
    return param[2];
}

static anna_object_t *anna_hash_get(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    anna_object_t *res = hash_get(h_unwrap(param[0]), param[1]);
    return res ? res : null_object;
}

static anna_object_t *anna_hash_get_count(anna_object_t **param)
{
    return anna_int_create(hash_get_count(h_unwrap(param[0])));
}


static anna_type_t *anna_hash_get_key_specialization(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_member_addr_get_mid(
		 obj,
		 ANNA_MID_HASH_SPECIALIZATION1));    
}

static anna_type_t *anna_hash_get_value_specialization(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_member_addr_get_mid(
		 obj,
		 ANNA_MID_HASH_SPECIALIZATION2));    
}

static void anna_hash_each_fun(void *keyp, void *valp, void *funp)
{
    anna_object_t *o_param[]=
	{
	    keyp, valp
	}
    ;   
    anna_vm_run(funp, 2, o_param);
}


static anna_object_t *anna_hash_each(anna_object_t **param)
{
    anna_object_t *body_object=param[1];
    
    hash_foreach2(h_unwrap(param[0]), anna_hash_each_fun, body_object);

    return param[0];
}

static anna_object_t *anna_hash_del(anna_object_t **param)
{
    hash_destroy(h_unwrap(param[0]));
    return param[0];
}

#if 0

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

#endif

static void anna_hash_type_create_internal(
    anna_stack_template_t *stack,
    anna_type_t *type, 
    anna_type_t *spec1,
    anna_type_t *spec2)
{
    anna_member_create(
	type, ANNA_MID_HASH_PAYLOAD,  L"!hashPayload",
	0, null_type);

    int i;
    string_buffer_t sb;
    sb_init(&sb);
    for(i=1; i<(((sizeof(hash_table_t)+1)/sizeof(anna_object_t *))+1);i++)
    {
	sb_clear(&sb);
	sb_printf(&sb, L"!hashPayload%d", i+1);
	anna_member_create(
	    type, anna_mid_get(sb_content(&sb)), sb_content(&sb), 
	    0, null_type);
    }
    sb_destroy(&sb);

    anna_member_create(
	type, ANNA_MID_HASH_SPECIALIZATION1,  L"!hashSpecialization1",
	1, null_type);

    anna_member_create(
	type, ANNA_MID_HASH_SPECIALIZATION2,  L"!hashSpecialization2",
	1, null_type);

    (*(anna_type_t **)anna_static_member_addr_get_mid(type,ANNA_MID_HASH_SPECIALIZATION1)) = spec1;
    (*(anna_type_t **)anna_static_member_addr_get_mid(type,ANNA_MID_HASH_SPECIALIZATION2)) = spec2;
    
    anna_type_t *kv_argv[] = 
	{
	    type,
	    spec1,
	    spec2
	}
    ;
    
    wchar_t *kv_argn[]=
	{
	    L"this", L"key", L"value"
	}
    ;

    anna_native_method_create(
	type, 
	-1,
	L"__set__", 
	0, 
	&anna_hash_set, 
	spec2,
	3,
	kv_argv, 
	kv_argn);    
    
    anna_native_method_create(
	type, 
	-1,
	L"__get__", 
	0, 
	&anna_hash_get, 
	spec2,
	2,
	kv_argv, 
	kv_argn);    

    anna_type_t *i_argv[] = 
	{
	    type,
	    type
	}
    ;
    
    wchar_t *i_argn[]=
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
	2, i_argv, i_argn);
    
    anna_native_property_create(
	type,
	-1,
	L"count",
	int_type,
	&anna_hash_get_count, 
	0);

    anna_type_t *fun_type = anna_function_type_each_create(
	L"!MapIterFunction", spec1, spec2);

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
	type,
	ANNA_MID_DEL,
	L"__del__",
	0,
	&anna_hash_del, 
	object_type,
	1, e_argv, e_argn);

#if 0

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
        
#endif

}

static inline void anna_hash_internal_init()
{
    static int init = 0;
    if(likely(init))
	return;
    init=1;
    hash_init(&anna_hash_specialization, hash_tt_func, hash_tt_cmp);
}

void anna_hash_type_create(anna_stack_template_t *stack)
{
    anna_hash_internal_init();
    hash_put(&anna_hash_specialization, anna_tt_make(object_type, object_type), hash_type);
    anna_hash_type_create_internal(stack, hash_type, object_type, object_type);
}

anna_type_t *anna_hash_type_get(anna_type_t *subtype1, anna_type_t *subtype2)
{
    anna_hash_internal_init();
    anna_tt_t tt = 
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
	hash_put(&anna_hash_specialization, anna_tt_make(subtype1, subtype2), spec);
	anna_hash_type_create_internal(stack_global, spec, subtype1, subtype2);
    }
    return spec;
}




