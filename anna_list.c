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
#include "anna_string.h"

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

ssize_t anna_list_calc_offset(ssize_t offset, size_t size)
{
    if(offset < 0) {
	if((-offset) > size)
	    return -1;
	
	return size+offset;
    }
    return offset;
}

void anna_list_set(struct anna_object *this, ssize_t offset, anna_entry_t *value)
{
    size_t size = anna_list_get_size(this);
    ssize_t pos = anna_list_calc_offset(offset, size);
//    wprintf(L"Set el %d in list of %d elements\n", pos, size);
    if(pos < 0)
    {
	return;
    }
    if(pos >= size)
    {
//	wprintf(L"Set new size\n");
	anna_list_set_size(this, pos+1);      
    }
    
    anna_entry_t **ptr = anna_list_get_payload(this);
    ptr[pos] = value;  
}

anna_entry_t *anna_list_get(anna_object_t *this, ssize_t offset)
{
    size_t size = anna_list_get_size(this);
    ssize_t pos = anna_list_calc_offset(offset, size);
    if(pos < 0||pos >=size)
    {
	return anna_from_obj(null_object);
    }
    anna_entry_t **ptr = anna_list_get_payload(this);
    return ptr[pos];
}

void anna_list_add(struct anna_object *this, anna_entry_t *value)
{
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
	anna_entry_t **ptr = anna_list_get_payload(this);
	int i;
	for(i=old_size; i<sz; i++)
	{
	    ptr[i] = anna_from_obj(null_object);
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
    anna_entry_t **ptr = anna_list_get_payload(this);
    ptr = realloc(ptr, sizeof(anna_entry_t *)*sz);
    if(!ptr)
    {
	CRASH;
    }    
    (*(size_t *)anna_member_addr_get_mid(this,ANNA_MID_LIST_CAPACITY)) = sz;
    *(anna_entry_t ***)anna_member_addr_get_mid(this,ANNA_MID_LIST_PAYLOAD) = ptr;
}

anna_entry_t **anna_list_get_payload(anna_object_t *this)
{
    return *(anna_entry_t ***)anna_member_addr_get_mid(this,ANNA_MID_LIST_PAYLOAD);
}


static inline anna_entry_t *anna_list_set_int_i(anna_entry_t **param)
{
    ANNA_VM_NULLCHECK(param[1]);
    anna_list_set(anna_as_obj(param[0]), anna_as_int(param[1]), param[2]);
    return param[2];
}
ANNA_VM_NATIVE(anna_list_set_int, 3)

static inline anna_entry_t *anna_list_get_int_i(anna_entry_t **param)
{ 
    ANNA_VM_NULLCHECK(param[1]);
    return anna_list_get(anna_as_obj(param[0]), anna_as_int(param[1]));
}
ANNA_VM_NATIVE(anna_list_get_int, 2)

static inline anna_entry_t *anna_list_get_count_i(anna_entry_t **param)
{
    return anna_from_int(anna_list_get_size(anna_as_obj(param[0])));
}
ANNA_VM_NATIVE(anna_list_get_count, 1)

static inline anna_entry_t *anna_list_get_first_i(anna_entry_t **param)
{
    return anna_list_get(anna_as_obj(param[0]), 0);
}
ANNA_VM_NATIVE(anna_list_get_first, 1)

static inline anna_entry_t *anna_list_get_last_i(anna_entry_t **param)
{
    return anna_list_get(anna_as_obj(param[0]), anna_list_get_size(anna_as_obj(param[0]))-1);
}
ANNA_VM_NATIVE(anna_list_get_last, 1)

static inline anna_entry_t *anna_list_set_count_i(anna_entry_t **param)
{
    ANNA_VM_NULLCHECK(param[1]);
    int sz = anna_as_int(param[1]);
    anna_list_set_size(anna_as_obj(param[0]), sz);
    return param[1];
}
ANNA_VM_NATIVE(anna_list_set_count, 2)

static inline anna_entry_t *anna_list_append_i(anna_entry_t **param)
{
    size_t i;

    size_t capacity = anna_list_get_capacity(anna_as_obj(param[0]));
    size_t size = anna_list_get_size(anna_as_obj(param[0]));
    size_t size2 = anna_list_get_size(anna_as_obj(param[1]));
    size_t new_size = size+size2;
    
    if(capacity <= (new_size))
    {
	anna_list_set_capacity(anna_as_obj(param[0]), maxi(8, new_size*2));
    }
    anna_entry_t **ptr = anna_list_get_payload(anna_as_obj(param[0]));
    anna_entry_t **ptr2 = anna_list_get_payload(anna_as_obj(param[1]));
    *(size_t *)anna_member_addr_get_mid(anna_as_obj(param[0]),ANNA_MID_LIST_SIZE) = new_size;
    for(i=0; i<size2; i++)
    {
	ptr[size+i]=ptr2[i];
    }
    
    return param[0];
}
ANNA_VM_NATIVE(anna_list_append, 2)

/**
   This is the bulk of the each method
 */
static anna_vmstack_t *anna_list_each_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    // Discard the output of the previous method call
    anna_vmstack_pop_object(stack);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_entry_t **param = stack->top - 3;
    // Unwrap and name the params to make things more explicit
    anna_object_t *list = anna_as_obj(param[0]);
    anna_object_t *body = anna_as_obj(param[1]);
    int idx = anna_as_int(param[2]);
    size_t sz = anna_list_get_size(list);
    
    // Are we done or do we need another lap?
    if(idx < sz)
    {
	// Set up params for the next lap of the each body function
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_list_get(list, idx)
	    }
	;
	// Then update our internal lap counter
	param[2] = anna_from_int(idx+1);
	
	// Finally, roll the code point back a bit and push new arguments
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	// Oops, we're done. Drop our internal param list and push the correct output
	anna_vmstack_drop(stack, 4);
	anna_vmstack_push_object(stack, list);
    }
    
    return stack;
}

static anna_vmstack_t *anna_list_each(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *body = anna_vmstack_pop_object(stack);
    anna_object_t *list = anna_vmstack_pop_object(stack);
    anna_vmstack_pop_object(stack);
    size_t sz = anna_list_get_size(list);

    if(sz > 0)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_obj(body),
		anna_from_int(1)
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_list_get(list, 0)
	    }
	;
	
	stack = anna_vm_callback_native(
	    stack,
	    anna_list_each_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_vmstack_push_object(stack, list);
    }
    
    return stack;
}

static anna_vmstack_t *anna_list_map_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    anna_object_t *value = anna_vmstack_pop_object(stack);

    anna_entry_t **param = stack->top - 4;
    anna_object_t *list = anna_as_obj_fast(param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    anna_object_t *res = anna_as_obj_fast(param[3]);
    size_t sz = anna_list_get_size(list);

    anna_list_set(res, idx-1, anna_from_obj(value));

    if(sz > idx)
    {
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_list_get(list, idx)
	    }
	;
	
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	anna_vmstack_drop(stack, 5);
	anna_vmstack_push_object(stack, res);
    }    
    return stack;
}

static anna_vmstack_t *anna_list_map(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *body = anna_vmstack_pop_object(stack);
    anna_object_t *list = anna_vmstack_pop_object(stack);
    anna_vmstack_pop_object(stack);
    if(body == null_object)
    {
	anna_vmstack_push_object(stack, null_object);
    }
    else
    {
	anna_function_t *fun = anna_function_unwrap(body);
	anna_object_t *res = anna_list_create(fun->return_type);
	
	size_t sz = anna_list_get_size(list);
	
	if(sz > 0)
	{
	    anna_entry_t *callback_param[] = 
		{
		    anna_from_obj(list),
		    anna_from_obj(body),
		    anna_from_int(1),
		    anna_from_obj(res)
		}
	    ;
	    
	    anna_entry_t *o_param[] =
		{
		    anna_from_int(0),
		    anna_list_get(list, 0)
		}
	    ;
	    
	    stack = anna_vm_callback_native(
		stack,
		anna_list_map_callback, 4, callback_param,
		body, 2, o_param
		);
	}
	else
	{
	    anna_vmstack_push_object(stack, res);
	}
    }
    
    return stack;
}

static anna_vmstack_t *anna_list_filter_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    anna_object_t *value = anna_vmstack_pop_object(stack);

    anna_entry_t **param = stack->top - 4;
    anna_object_t *list = anna_as_obj_fast(	param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    anna_object_t *res = anna_as_obj_fast(param[3]);
    size_t sz = anna_list_get_size(list);

    if(value != null_object)
    {
	anna_list_add(res, anna_list_get(list, idx-1));
    }
    
    if(sz > idx)
    {
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_list_get(list, idx)
	    }
	;
	
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	anna_vmstack_drop(stack, 5);
	anna_vmstack_push_object(stack, res);
    }    
    return stack;
}

static anna_vmstack_t *anna_list_filter(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *body = anna_vmstack_pop_object(stack);
    anna_object_t *list = anna_vmstack_pop_object(stack);
    anna_object_t *res = anna_list_create(anna_list_get_specialization(list));
    anna_vmstack_pop_object(stack);
    
    size_t sz = anna_list_get_size(list);
    
    if(sz > 0)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_obj(body),
		anna_from_int(1),
		anna_from_obj(res)
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_list_get(list, 0)
	    }
	;
	
	stack = anna_vm_callback_native(
	    stack,
	    anna_list_filter_callback, 4, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_vmstack_push_object(stack, res);
    }

    return stack;
}

static anna_vmstack_t *anna_list_find_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    anna_entry_t *value = anna_vmstack_pop_entry(stack);

    anna_entry_t **param = stack->top - 3;
    anna_object_t *list = anna_as_obj_fast(param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    size_t sz = anna_list_get_size(list);

    if(!ANNA_VM_NULL(value))
    {
	anna_vmstack_drop(stack, 4);
	anna_vmstack_push_entry(stack, anna_list_get(list, idx-1));
    }
    else if(sz > idx)
    {
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_list_get(list, idx)
	    }
	;
	
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	anna_vmstack_drop(stack, 4);
	anna_vmstack_push_object(stack, null_object);
    }
    return stack;
}

static anna_vmstack_t *anna_list_find(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *body = anna_vmstack_pop_object(stack);
    anna_object_t *list = anna_vmstack_pop_object(stack);
    anna_vmstack_pop_object(stack);
    
    size_t sz = anna_list_get_size(list);
    
    if(sz > 0)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_obj(body),
		anna_from_int(1),
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_list_get(list, 0)
	    }
	;
	
	stack = anna_vm_callback_native(
	    stack,
	    anna_list_find_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_vmstack_push_object(stack, null_object);
    }

    return stack;
}


static anna_vmstack_t *anna_list_to_string_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    anna_object_t *value = anna_vmstack_pop_object(stack);

    anna_entry_t **param = stack->top - 3;
    anna_object_t *list = anna_as_obj_fast(param[0]);
    int idx = anna_as_int(param[1]);
    anna_object_t *res = anna_as_obj_fast(param[2]);
    size_t sz = anna_list_get_size(list);

    if(value == null_object)
    {
	value = anna_string_create(4,L"null");
    }
    
    anna_string_append(res, value);
    
    if(sz > idx)
    {
	anna_string_append(res, anna_string_create(2,L", "));
	anna_object_t *o = anna_as_obj(anna_list_get(list, idx));
	param[1] = anna_from_int(idx+1);	
	anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	anna_object_t *meth = anna_as_obj_fast(o->type->static_member[tos_mem->offset]);
	anna_vm_callback_reset(stack, meth, 1, (anna_entry_t **)&o);
    }
    else
    {
	anna_string_append(res, anna_string_create(1,L"]"));
	anna_vmstack_drop(stack, 4);
	anna_vmstack_push_object(stack, res);
    }
    return stack;
}

static anna_vmstack_t *anna_list_to_string(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *list = anna_vmstack_pop_object(stack);
    anna_vmstack_pop_object(stack);

    size_t sz = anna_list_get_size(list);
    
    if(sz > 0)
    {
	anna_object_t *res = anna_string_create(1,L"[");
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_int(1),
		anna_from_obj(res)
	    }
	;
	    
	anna_object_t *o = anna_as_obj(anna_list_get(list, 0));
	anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	anna_object_t *meth = anna_as_obj_fast(o->type->static_member[tos_mem->offset]);
	stack = anna_vm_callback_native(
	    stack,
	    anna_list_to_string_callback, 3, callback_param,
	    meth, 1, (anna_entry_t **)&o
	    );
    }
    else
    {
	anna_vmstack_push_object(stack, anna_string_create(2,L"[]"));
    }
    return stack;
}


static inline anna_entry_t *anna_list_init_i(anna_entry_t **param)
{
    (*anna_member_addr_get_mid(anna_as_obj_fast(param[0]),ANNA_MID_LIST_PAYLOAD))=0;
    (*(size_t *)anna_member_addr_get_mid(anna_as_obj_fast(param[0]),ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_member_addr_get_mid(anna_as_obj_fast(param[0]),ANNA_MID_LIST_SIZE)) = 0;

    size_t sz = anna_list_get_size(anna_as_obj_fast(param[1]));
    anna_entry_t **src = anna_list_get_payload(anna_as_obj_fast(param[1]));

    anna_list_set_size(anna_as_obj_fast(param[0]), sz);
    anna_entry_t **dest = anna_list_get_payload(anna_as_obj_fast(param[0]));
    memcpy(dest, src, sizeof(anna_object_t *)*sz);
    
    return param[0];
}

ANNA_VM_NATIVE(anna_list_init, 2)

static inline anna_entry_t *anna_list_del_i(anna_entry_t **param)
{
    free((*anna_member_addr_get_mid(anna_as_obj_fast(param[0]),ANNA_MID_LIST_PAYLOAD)));
    (*(size_t *)anna_member_addr_get_mid(anna_as_obj_fast(param[0]),ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_member_addr_get_mid(anna_as_obj_fast(param[0]),ANNA_MID_LIST_SIZE)) = 0;
    return param[0];
}

ANNA_VM_NATIVE(anna_list_del, 1)

static inline anna_entry_t *anna_list_push_i(anna_entry_t **param)
{
    anna_list_set(
	anna_as_obj_fast(param[0]), 
	(*(size_t *)anna_member_addr_get_mid(anna_as_obj_fast(param[0]),ANNA_MID_LIST_SIZE)),
	param[1]);
    return param[0];
}

ANNA_VM_NATIVE(anna_list_push, 2)

static inline anna_entry_t *anna_list_pop_i(anna_entry_t **param)
{
    size_t *sz = (size_t *)anna_member_addr_get_mid(anna_as_obj_fast(param[0]),ANNA_MID_LIST_SIZE);
    if(!*sz)
	return anna_from_obj(null_object);
    (*sz)--;
    return (*(anna_entry_t ***)anna_member_addr_get_mid(anna_as_obj_fast(param[0]),ANNA_MID_LIST_PAYLOAD))[*sz];
}

ANNA_VM_NATIVE(anna_list_pop, 1)

static anna_vmstack_t *anna_list_in_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    anna_entry_t *ret = anna_vmstack_pop_entry(stack);
//    wprintf(L"Wee, in callback value: %ls\n", ret==null_object?L"null":L"not null");
    anna_entry_t **param = stack->top - 3;
    anna_object_t *list = anna_as_obj_fast(param[0]);
    anna_object_t *value = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    size_t sz = anna_list_get_size(list);
    
    if(!ANNA_VM_NULL(ret))
    {
	anna_vmstack_drop(stack, 4);
	anna_vmstack_push_entry(stack, anna_from_int(idx-1));
    }
    else if(sz > idx)
    {
	anna_entry_t *o_param[] =
	    {
		anna_from_obj(value),
		anna_list_get(list, idx)
	    }
	;
	
	anna_object_t *fun_object = anna_as_obj_fast(*anna_static_member_addr_get_mid(value->type, ANNA_MID_EQ));
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(stack, fun_object, 2, o_param);
    }
    else
    {
	anna_vmstack_drop(stack, 4);
	anna_vmstack_push_object(stack, null_object);
    }
    return stack;
}

static anna_vmstack_t *anna_list_in(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *value = anna_vmstack_pop_object(stack);
    anna_object_t *list = anna_vmstack_pop_object(stack);
    anna_vmstack_pop_object(stack);
    
    size_t sz = anna_list_get_size(list);
    
    if(sz > 0)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_obj(value),
		anna_from_int(1),
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_obj(value),
		anna_list_get(list, 0)
	    }
	;
	
	anna_object_t *fun_object = anna_as_obj_fast(*anna_static_member_addr_get_mid(value->type, ANNA_MID_EQ));
	stack = anna_vm_callback_native(
	    stack,
	    anna_list_in_callback, 3, callback_param,
	    fun_object, 2, o_param
	    );
    }
    else
    {
	anna_vmstack_push_object(stack, null_object);
    }

    return stack;
}

static inline anna_entry_t *anna_list_i_get_range_i(anna_entry_t **param)
{
    ANNA_VM_NULLCHECK(param[1]);
    anna_object_t *list = anna_as_obj(param[0]);
    anna_object_t *range = anna_as_obj(param[1]);
    int from = anna_range_get_from(range);
    int step = anna_range_get_step(range);
    int count = anna_range_get_count(range);
    int i;
    
    anna_object_t *res = anna_list_create(anna_list_get_specialization(list));
    anna_list_set_capacity(res, count);
    for(i=0;i<count;i++)
    {
	anna_list_set(
	    res, i, 
	    anna_list_get(
		list,
		from + step*i));
    }    

    return anna_from_obj(res);
    
}
ANNA_VM_NATIVE(anna_list_i_get_range, 2)

static inline anna_entry_t *anna_list_i_set_range_i(anna_entry_t **param)
{
    ANNA_VM_NULLCHECK(param[1]);
    
    anna_object_t *repl;
    anna_object_t *list = anna_as_obj(param[0]);
    anna_object_t *range = anna_as_obj(param[1]);
    
    if(unlikely(ANNA_VM_NULL(param[2])))
	repl = anna_list_create(object_type);
    else
	repl = anna_as_obj(param[2]);
    
    int from = anna_range_get_from(range);
    int step = anna_range_get_step(range);
    int to = anna_range_get_to(range);
    int count = anna_range_get_count(range);
    int i;
    
    int count2 = anna_list_get_size(repl);

    if(count != count2)
    {
	if(step != 1)
	{
	    return anna_from_obj(null_object);
	}

	int old_size = anna_list_get_size(list);

	/* If we're assigning past the end of the array, just silently
	 * take the whole array and go on */
	count = mini(count, old_size - from);	
	int new_size = old_size - count + count2;
	anna_entry_t **arr;
	if(to > new_size)
	{
	    anna_list_set_capacity(list, to);
	    for(i=old_size; i<new_size; i++)
	    {
		anna_list_set(
		    list, i, anna_from_obj(null_object));		
	    }
	    *(size_t *)anna_member_addr_get_mid(list,ANNA_MID_LIST_SIZE) = new_size;
	    arr = anna_list_get_payload(list);
	}
	else
	{
	    if(new_size > anna_list_get_capacity(list))
	    {
		anna_list_set_capacity(list, new_size);
	    }
	    
	    *(size_t *)anna_member_addr_get_mid(list,ANNA_MID_LIST_SIZE) = new_size;
	    arr = anna_list_get_payload(list);
	    memmove(&arr[from+count2], &arr[from+count], sizeof(anna_object_t *)*abs(old_size - from - count ));
	}
	
	/* Set new size - don't call anna_list_set_size, since that might truncate the list if we're shrinking */

	/* Move the old data */

	/* Copy in the new data */
	for(i=0;i<count2;i++)
	{
	    arr[from+i] = 
		anna_list_get(
		    repl,
		    i);
	}
    }
    else
    {
	for(i=0;i<count;i++)
	{
	    anna_list_set(
		list, from + step*i, 
		anna_list_get(
		    repl,
		    i));
	}
    }

    return param[0];
}
ANNA_VM_NATIVE(anna_list_i_set_range, 3)

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
    fun = anna_function_unwrap(anna_as_obj_fast(*anna_static_member_addr_get_mid(type, mmid)));
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
    fun = anna_function_unwrap(anna_as_obj_fast(*anna_static_member_addr_get_mid(type, mmid)));
    anna_function_alias_add(fun, L"__set__");
    
    anna_native_property_create(
	type,
	-1,
	L"count",
	int_type,
	&anna_list_get_count, 
	&anna_list_set_count);

    anna_native_property_create(
	type,
	-1,
	L"first",
	spec,
	&anna_list_get_first,
	0);

    anna_native_property_create(
	type,
	-1,
	L"last",
	spec,
	&anna_list_get_last,
	0);

    anna_native_method_create(
	type, -1, L"__appendAssign__", 0, 
	&anna_list_append, 
	type,
	2, l_argv, l_argn);
    
    anna_native_method_create(
	type, -1, L"push", 0, 
	&anna_list_push, 
	type,
	2, a_argv, a_argn);

    anna_native_method_create(
	type, -1, L"pop", 0, 
	&anna_list_pop, 
	spec,
	1, a_argv, a_argn);
    
    anna_type_t *fun_type = anna_function_type_each_create(
	L"!ListIterFunction", int_type, spec);

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
	type, -1, L"__find__", 
	0, &anna_list_find, 
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
	int_type,
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
    fun = anna_function_unwrap(anna_as_obj_fast(*anna_static_member_addr_get_mid(type, mmid)));
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
    fun = anna_function_unwrap(anna_as_obj_fast(*anna_static_member_addr_get_mid(type, mmid)));
    anna_function_alias_add(fun, L"__set__");
    
    anna_native_method_create(
	type,
	ANNA_MID_TO_STRING,
	L"toString",
	0,
	&anna_list_to_string, 
	string_type, 1, a_argv, a_argn);

}

static inline void anna_list_internal_init()
{
    static int init = 0;
    if(likely(init))
	return;
    init=1;
    hash_init(&anna_list_specialization, hash_ptr_func, hash_ptr_cmp);
}

void anna_list_type_create(anna_stack_template_t *stack)
{
    anna_list_internal_init();
    
    hash_put(&anna_list_specialization, object_type, list_type);
    anna_list_type_create_internal(stack, list_type, object_type);
}

anna_type_t *anna_list_type_get(anna_type_t *subtype)
{
    anna_list_internal_init();
    
    anna_type_t *spec = hash_get(&anna_list_specialization, subtype);
    if(!spec)
    {
	string_buffer_t sb;
	sb_init(&sb);
	sb_printf(&sb, L"List«%ls»", subtype->name);
	spec = anna_type_native_create(sb_content(&sb), stack_global);
	sb_destroy(&sb);
	hash_put(&anna_list_specialization, subtype, spec);
	anna_list_type_create_internal(stack_global, spec, subtype);
	spec->flags |= ANNA_TYPE_SPECIALIZED;
	anna_type_copy_object(spec);
    }

    anna_type_copy_object(spec);
    return spec;
}
