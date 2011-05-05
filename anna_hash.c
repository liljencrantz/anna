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
#include "anna_pair.h"
#include "anna_list.h"

#include "anna_macro.h"

#define ANNA_HASH_MINSIZE 16

typedef struct
{
    int hash;
    anna_entry_t *key;
    anna_entry_t *value;
}
    anna_hash_entry_t;

typedef struct {
    size_t fill;
    size_t used;
    size_t mask;
    anna_hash_entry_t *table;
    anna_hash_entry_t small_table[ANNA_HASH_MINSIZE];
} anna_hash_t;

typedef anna_vmstack_t *(*ahi_callback_t)(anna_vmstack_t *stack, anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, anna_hash_entry_t *hash_entry);

static hash_table_t anna_hash_specialization;

static inline int hash_entry_is_used(anna_hash_entry_t *e)
{
    return !!e->key;
}

static inline int hash_entry_is_dummy(anna_hash_entry_t *e)
{
    return !e->key && !!e->value ;
}

static inline void hash_entry_clear(anna_hash_entry_t *e)
{
    e->key = 0;
}

static inline anna_hash_t *ahi_unwrap(anna_object_t *obj)
{
    return (anna_hash_t *)anna_entry_get_addr(obj,ANNA_MID_HASH_PAYLOAD);
}

static inline size_t anna_hash_get_size(anna_object_t *this)
{
    return ahi_unwrap(this)->used;
}

static inline anna_entry_t *anna_hash_get_key_from_idx(anna_object_t *this, int idx)
{
    return ahi_unwrap(this)->table[idx].key;
}

static inline anna_entry_t *anna_hash_get_value_from_idx(anna_object_t *this, int idx)
{
    return ahi_unwrap(this)->table[idx].value;
}

static inline ssize_t anna_hash_get_next_idx(anna_object_t *this, ssize_t idx)
{
    anna_hash_t *hash = ahi_unwrap(this);
    for(; idx <= hash->mask; idx++)
    {
	if(hash_entry_is_used(&hash->table[idx]))
	{
	    return idx;
	}
    }
    return -1;
}

static inline ssize_t anna_hash_get_next_non_dummy(anna_object_t *this, ssize_t idx)
{
    anna_hash_t *hash = ahi_unwrap(this);
    int pos;
    for(;; idx++)
    {
	pos = idx & hash->mask;
	anna_hash_entry_t *e = &hash->table[pos];
	if(!hash_entry_is_dummy(e))
	{
	    return pos;
	}
    }
}

static inline void ahi_init(anna_hash_t *this)
{
    this->fill = this->used = 0;
    this->mask = ANNA_HASH_MINSIZE-1;
    this->table = &this->small_table[0];
    memset(&this->small_table[0], 0, sizeof(anna_hash_entry_t) * ANNA_HASH_MINSIZE);
}

static void anna_hash_resize(anna_hash_t *this, size_t new_sz)
{
//    wprintf(L"Weee, resize to %d\n", new_sz);
    size_t old_sz = this->mask+1;
    size_t new_mask = new_sz-1;
    
    anna_hash_entry_t *new_table;
    if( new_sz == ANNA_HASH_MINSIZE) 
    {
	new_table = &this->small_table[0];
	memset(new_table, 0, new_sz * sizeof(anna_hash_entry_t));
    }
    else
    {
	new_table = calloc(1, new_sz * sizeof(anna_hash_entry_t));
    }
    int i, j;
    for(i=0; i<old_sz; i++){
	anna_hash_entry_t *e = &this->table[i];
	if(hash_entry_is_used(e))
	{
	    int pos;
	    anna_hash_entry_t *new_e;
	    for(j=0; 1; j++)
	    {
		pos = (e->hash+j) & new_mask;
		new_e= &new_table[pos];
		if(!hash_entry_is_used(new_e))
		{
		    break;
		}
	    }
	    memcpy(new_e, e, sizeof(anna_hash_entry_t));
	}
    }
    if(this->table != &this->small_table[0])
    {
	free(this->table);
    }
    this->table = new_table;
    this->mask = new_mask;
}

static void anna_hash_check_resize(anna_hash_t *this)
{
    size_t old_sz = this->mask+1;
//    if(old_sz <= 64)
//	wprintf(L"%d: %f > %d?\n", old_sz, 0.1*old_sz, this->used);
    if(0.75*old_sz < this->used)
    {
	size_t new_sz = 4 * old_sz;
	anna_hash_resize(this, new_sz);
    }
    else if( 0.1*old_sz > this->used)
    {
	size_t new_sz = old_sz;
	do
	{
	    new_sz /= 4;
	}
	while(0.1*new_sz > this->used && new_sz >= ANNA_HASH_MINSIZE);
	anna_hash_resize(this, new_sz);
    }
}

static anna_vmstack_t *ahi_search_callback2(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t *eq = anna_vmstack_pop_entry(stack);
    int idx = anna_vmstack_pop_int(stack);
    int hash = anna_vmstack_pop_int(stack);
    anna_entry_t *aux = anna_vmstack_pop_entry(stack);
    ahi_callback_t callback = (ahi_callback_t)anna_as_blob(anna_vmstack_pop_entry(stack));
    anna_entry_t *hash_obj = anna_vmstack_pop_entry(stack);
    anna_entry_t *key = anna_vmstack_pop_entry(stack);


    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash_obj));

    if(ANNA_VM_NULL(eq))
    {


	idx++;
	int pos = anna_hash_get_next_non_dummy(anna_as_obj(hash_obj), hash+idx);
	anna_hash_entry_t *e = &this->table[pos];
	idx = pos-hash;
//	wprintf(L"Position is non-empty, and keys are not equal. Check next position: %d (idx %d).\n", pos, idx);
	
	if(hash_entry_is_used(e))
	{
//	    wprintf(L"Position %d (idx %d) is non-empty, check if keys are equal\n", pos, idx);
	    anna_entry_t *callback_param[] = 
		{
		    key,
		    hash_obj,
		    anna_from_blob(callback),
		    aux,
		    anna_from_int(hash),
		    anna_from_int(idx)
		}
	    ;
	    
	    anna_entry_t *o_param[] = 
		{
		    key,
		    this->table[pos].key
		}
	    ;
	    
	    anna_object_t *fun_object = 
		anna_as_obj_fast(
		    anna_entry_get_static(
			anna_as_obj(key)->type, 
			ANNA_MID_EQ));
	    return anna_vm_callback_native(
		stack,
		ahi_search_callback2, 6, callback_param,
		fun_object, 2, o_param);
	}
	else
	{
	    return callback(stack, key, hash, hash_obj, aux, &this->table[pos]);
	}
    }
    else
    {
	int pos = (hash+idx) & this->mask;
//	wprintf(L"Position %d is non-empty, but keys are equal. Found match, run callback function!\n", pos);
	return callback(stack, key, hash, hash_obj, aux, &this->table[pos]);
    }
}

static anna_vmstack_t *ahi_search_callback(anna_vmstack_t *stack, anna_object_t *me)
{
    int hash = anna_vmstack_pop_int(stack);
    anna_entry_t *aux = anna_vmstack_pop_entry(stack);
    ahi_callback_t callback = (ahi_callback_t)anna_as_blob(anna_vmstack_pop_entry(stack));
    anna_entry_t *hash_obj = anna_vmstack_pop_entry(stack);
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash_obj));
    anna_vmstack_pop_entry(stack);
    
    
    int idx = 0;
    int pos = anna_hash_get_next_non_dummy(anna_as_obj(hash_obj), hash);
    anna_hash_entry_t *e = &this->table[pos];
    idx = pos-hash;

//    wprintf(L"Calculated hash value %d, maps to position %d\n", hash, pos);
    if(hash_entry_is_used(e))
    {
//	wprintf(L"Position %d (idx %d) is non-empty, check if keys are equal\n", pos, idx);
	anna_entry_t *callback_param[] = 
	    {
		key,
		hash_obj,
		anna_from_blob(callback),
		aux,
		anna_from_int(hash),
		anna_from_int(idx)		
	    }
	;

	anna_entry_t *o_param[] = 
	    {
		key,
		this->table[pos].key
	    }
	;
	
	anna_object_t *fun_object = anna_as_obj_fast(
	    anna_entry_get_static(
		anna_as_obj(key)->type, ANNA_MID_EQ));
	return anna_vm_callback_native(
	    stack,
	    ahi_search_callback2, 6, callback_param,
	    fun_object, 2, o_param
	    );
    }
    else
    {
//	wprintf(L"Position %d (idx %d) is empty\n", pos, idx);
	return callback(stack, key, hash, hash_obj, aux, &this->table[pos]);
    }
}

static inline anna_vmstack_t *ahi_search(
    anna_vmstack_t *stack,
    anna_entry_t *key,
    anna_entry_t *hash,
    ahi_callback_t callback,
    anna_entry_t *aux)
{
    anna_entry_t *callback_param[] = 
	{
	    key,
	    hash,
	    anna_from_blob(callback),
	    aux
	}
    ;
    
    anna_object_t *o = anna_as_obj(key);
//    wprintf(L"Search for object of type %ls in hash table\n", o->type->name);
    anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_HASH_CODE);
    anna_object_t *meth = anna_as_obj_fast(o->type->static_member[tos_mem->offset]);
    
    return anna_vm_callback_native(
	stack,
	ahi_search_callback, 4, callback_param,
	meth, 1, (anna_entry_t **)&o
	);
}


static inline hash_table_t *h_unwrap(anna_object_t *obj)
{
    return (hash_table_t *)anna_entry_get_addr(obj,ANNA_MID_HASH_PAYLOAD);
}

anna_object_t *anna_hash_create(anna_type_t *spec1, anna_type_t *spec2)
{
    anna_object_t *obj= anna_object_create(anna_hash_type_get(spec1, spec2));
    ahi_init(ahi_unwrap(obj));    
    return obj;
}

anna_object_t *anna_hash_create2(anna_type_t *hash_type)
{
    anna_object_t *obj= anna_object_create(hash_type);
    ahi_init(ahi_unwrap(obj));    
    return obj;
}

static __attribute__((aligned(8))) anna_vmstack_t *anna_hash_init_callback(
    anna_vmstack_t *stack, 
    anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, 
    anna_hash_entry_t *hash_entry)
{
    anna_entry_t ** data = anna_as_blob(aux);
    anna_object_t *list = anna_as_obj(data[0]);
    int idx = anna_as_int(data[1]);
        
//    wprintf(L"Init callback\n");
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash));
    if(!hash_entry_is_used(hash_entry))
    {
	this->used++;
	if(!hash_entry_is_dummy(hash_entry))
	{
	    this->fill++;
	}
    }
    
//    wprintf(L"Hash table now has %d used slots and %d dummy slots\n", this->used, this->fill-this->used);
    
    hash_entry->hash = hash_code;
    hash_entry->key = key;
    
    anna_object_t *pair = anna_as_obj_fast(anna_list_get(list, idx));
    hash_entry->value = anna_pair_get_second(pair);


    int i;
    size_t sz = anna_list_get_size(list);
    if(sz > idx)
    {
	
	for(i=idx+1; i<sz; i++)
	{
	    pair = anna_as_obj_fast(anna_list_get(list, i));
	    if(likely(pair != null_object))
	    {
		anna_entry_t *key = anna_pair_get_first(pair);
		if(!ANNA_VM_NULL(key))
		{
//		    wprintf(L"LALAJ, insert pos %d in init\n", i);
		    data[1] = anna_from_int(i);
		    return ahi_search(
			stack,
			key,
			hash,
			anna_hash_init_callback,
			anna_from_blob(data));
		}
	    }
	}
    }	
    
    free(data);
    anna_vmstack_push_entry(stack, hash);
    return stack;
}

static anna_vmstack_t *anna_hash_init(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *list = anna_vmstack_pop_object(stack);
    anna_object_t *this = anna_vmstack_pop_object(stack);
    ahi_init(ahi_unwrap(this));
    size_t i;
    
    if(likely(list != null_object))
    {
	size_t sz = anna_list_get_size(list);
	if(sz > 0)
	{
	    anna_entry_t ** data = malloc(2*sizeof(anna_entry_t *));
	    data[0] = anna_from_obj(list);

	    for(i=0; i<sz; i++)
	    {
		anna_object_t *pair = anna_as_obj_fast(anna_list_get(list, i));
		if(likely(pair != null_object))
		{
		    anna_entry_t *key = anna_pair_get_first(pair);
		    if(!ANNA_VM_NULL(key))
		    {
			data[1] = anna_from_int(i);
			return ahi_search(
			    stack,
			    key,
			    anna_from_obj(this),
			    anna_hash_init_callback,
			    anna_from_blob(data));
		    }
		}
	    }
	}	
    }
    anna_vmstack_push_object(stack, this);
    return stack;
}

static __attribute__((aligned(8))) anna_vmstack_t *anna_hash_set_callback(
    anna_vmstack_t *stack, 
    anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, 
    anna_hash_entry_t *hash_entry)
{
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash));
    if(!hash_entry_is_used(hash_entry))
    {
	this->used++;
	if(!hash_entry_is_dummy(hash_entry))
	{
	    this->fill++;
	}
    }
    
    hash_entry->hash = hash_code;
    hash_entry->key = key;
    hash_entry->value = aux;
    anna_vmstack_push_entry(stack, aux);
    return stack;
}

static inline anna_vmstack_t *anna_hash_set(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t *val = anna_vmstack_pop_entry(stack);
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_entry_t *this = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_entry(stack);
    
    int i;
    anna_hash_t *h = ahi_unwrap(anna_as_obj_fast(this));

    if(ANNA_VM_NULL(key))
    {
	anna_vmstack_push_object(stack, null_object);
	return stack;
    }

    anna_hash_check_resize(ahi_unwrap(anna_as_obj_fast(this)));
    
    return ahi_search(
	stack,
	key,
	this,
	anna_hash_set_callback,
	val);
}

static __attribute__((aligned(8))) anna_vmstack_t *anna_hash_get_callback(
    anna_vmstack_t *stack, 
    anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, 
    anna_hash_entry_t *hash_entry)
{
    if(!hash_entry_is_used(hash_entry))
    {
	anna_vmstack_push_object(stack, null_object);	
    }
    else
    {
	anna_vmstack_push_entry(stack, hash_entry->value);	
    }
    
    return stack;
}

static inline anna_vmstack_t *anna_hash_get(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_entry_t *this = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_entry(stack);
    
    if(ANNA_VM_NULL(key))
    {
	anna_vmstack_push_object(stack, null_object);
	return stack;
    }
    
    return ahi_search(
	stack,
	key,
	this,
	anna_hash_get_callback,
	0);
}


static __attribute__((aligned(8))) anna_vmstack_t *anna_hash_in_callback(
    anna_vmstack_t *stack, 
    anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, 
    anna_hash_entry_t *hash_entry)
{
    if(!hash_entry_is_used(hash_entry))
    {
	anna_vmstack_push_object(stack, null_object);	
    }
    else
    {
	anna_vmstack_push_entry(stack, hash_entry->key);	
    }
    
    return stack;
}

static inline anna_vmstack_t *anna_hash_in(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_entry_t *this = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_entry(stack);
    
    if(ANNA_VM_NULL(key))
    {
	anna_vmstack_push_object(stack, null_object);
	return stack;
    }
    
    return ahi_search(
	stack,
	key,
	this,
	anna_hash_in_callback,
	0);
}

static __attribute__((aligned(8))) anna_vmstack_t *anna_hash_remove_callback(
    anna_vmstack_t *stack, 
    anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, 
    anna_hash_entry_t *hash_entry)
{
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash));

    if(hash_entry_is_used(hash_entry))
    {
	anna_vmstack_push_entry(stack, hash_entry->key);	
	hash_entry_clear(hash_entry);
	this->used--;
    }
    else
    {
	wprintf(L"Failed to remove element %d %d\n", anna_as_int(key), hash_entry->key);
	anna_vmstack_push_object(stack, null_object);	
    }
    
    return stack;
}

static inline anna_vmstack_t *anna_hash_remove(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_entry_t *this = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_entry(stack);

    if(ANNA_VM_NULL(key))
    {
	anna_vmstack_push_object(stack, null_object);
	return stack;
    }
    
    return ahi_search(
	stack,
	key,
	this,
	anna_hash_remove_callback,
	0);
}

static inline anna_entry_t *anna_hash_get_count_i(anna_entry_t **param)
{
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(param[0]));
    return anna_from_int(this->used);
}
ANNA_VM_NATIVE(anna_hash_get_count, 1)

/**
   This is the bulk of the each method
 */
static anna_vmstack_t *anna_hash_each_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    // Discard the output of the previous method call
    anna_vmstack_pop_object(stack);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_entry_t **param = stack->top - 3;
    // Unwrap and name the params to make things more explicit
    anna_object_t *hash = anna_as_obj(param[0]);
    anna_object_t *body = anna_as_obj(param[1]);
    int idx = anna_as_int(param[2]);
    
    int next_idx = anna_hash_get_next_idx(hash, idx);
    
    if(next_idx >= 0)
    {
	// Set up params for the next lap of the each body function
	anna_entry_t *o_param[] =
	    {
		anna_hash_get_key_from_idx(hash, next_idx),
		anna_hash_get_value_from_idx(hash, next_idx),
	    }
	;
	// Then update our internal lap counter
	param[2] = anna_from_int(next_idx+1);
	
	// Finally, roll the code point back a bit and push new arguments
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	// Oops, we're done. Drop our internal param list and push the correct output
	anna_vmstack_drop(stack, 4);
	anna_vmstack_push_object(stack, hash);
    }
    
    return stack;
}

static anna_vmstack_t *anna_hash_each(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *body = anna_vmstack_pop_object(stack);
    anna_object_t *hash = anna_vmstack_pop_object(stack);
    anna_vmstack_pop_object(stack);
    size_t sz = anna_hash_get_size(hash);

    if(sz > 0)
    {
	int next_idx = anna_hash_get_next_idx(hash, 0);
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(hash),
		anna_from_obj(body),
		anna_from_int(next_idx+1)
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_hash_get_key_from_idx(hash, next_idx),
		anna_hash_get_value_from_idx(hash, next_idx),
	    }
	;
	
	stack = anna_vm_callback_native(
	    stack,
	    anna_hash_each_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_vmstack_push_object(stack, hash);
    }
    
    return stack;
}


static anna_vmstack_t *anna_hash_map_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    anna_object_t *value = anna_vmstack_pop_object(stack);
    
    anna_entry_t **param = stack->top - 4;
    anna_object_t *hash = anna_as_obj_fast(param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    anna_object_t *res = anna_as_obj_fast(param[3]);
    size_t sz = anna_hash_get_size(hash);
    
    anna_list_add(res, anna_from_obj(value));
    int next_idx = anna_hash_get_next_idx(hash, idx);
    
    if(next_idx >= 0)
    {
	anna_entry_t *o_param[] =
	    {
		anna_hash_get_key_from_idx(hash, next_idx),
		anna_hash_get_value_from_idx(hash, next_idx)
	    }
	;
	
	param[2] = anna_from_int(next_idx+1);
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	anna_vmstack_drop(stack, 5);
	anna_vmstack_push_object(stack, res);
    }    
    return stack;
}

static anna_vmstack_t *anna_hash_map(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *body = anna_vmstack_pop_object(stack);
    anna_object_t *hash = anna_vmstack_pop_object(stack);
    anna_vmstack_pop_object(stack);
    if(body == null_object)
    {
	anna_vmstack_push_object(stack, null_object);
    }
    else
    {
	anna_function_t *fun = anna_function_unwrap(body);
	anna_object_t *res = anna_list_create(fun->return_type);
	
	size_t sz = anna_hash_get_size(hash);
	
	if(sz > 0)
	{
	    int next_idx = anna_hash_get_next_idx(hash, 0);
	    anna_entry_t *callback_param[] = 
		{
		    anna_from_obj(hash),
		    anna_from_obj(body),
		    anna_from_int(next_idx+1),
		    anna_from_obj(res)
		}
	    ;
	    
	    anna_entry_t *o_param[] =
		{
		    anna_hash_get_key_from_idx(hash, next_idx),
		    anna_hash_get_value_from_idx(hash, next_idx)
		}
	    ;
	    
	    stack = anna_vm_callback_native(
		stack,
		anna_hash_map_callback, 4, callback_param,
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


/*
static anna_type_t *anna_hash_get_key_specialization(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_entry_get_addr(
		 obj,
		 ANNA_MID_HASH_SPECIALIZATION1));    
}

static anna_type_t *anna_hash_get_value_specialization(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_entry_get_addr(
		 obj,
		 ANNA_MID_HASH_SPECIALIZATION2));    
}
*/
/*
static inline anna_object_t *anna_hash_del_i(anna_object_t **param)
{
    hash_destroy(h_unwrap(param[0]));
    return param[0];
}
ANNA_VM_NATIVE(anna_hash_del, 1)
*/

static void anna_hash_type_create_internal(
    anna_stack_template_t *stack,
    anna_type_t *type, 
    anna_type_t *spec1,
    anna_type_t *spec2)
{
    anna_member_create_blob(
	type, ANNA_MID_HASH_PAYLOAD,  L"!hashPayload",
	0, sizeof(anna_hash_t));
    
    anna_member_create(
	type, ANNA_MID_HASH_SPECIALIZATION1,  L"!hashSpecialization1",
	1, null_type);

    anna_member_create(
	type, ANNA_MID_HASH_SPECIALIZATION2,  L"!hashSpecialization2",
	1, null_type);

    (*(anna_type_t **)anna_entry_get_addr_static(type,ANNA_MID_HASH_SPECIALIZATION1)) = spec1;
    (*(anna_type_t **)anna_entry_get_addr_static(type,ANNA_MID_HASH_SPECIALIZATION2)) = spec2;
    
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
	    anna_pair_type_get(spec1, spec2)
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
	type, -1, L"__map__", 
	0, &anna_hash_map, 
	list_type,
	2, e_argv, e_argn);
    
/*
    anna_native_method_create(
	type,
	ANNA_MID_DEL,
	L"__del__",
	0,
	&anna_hash_del, 
	object_type,
	1, e_argv, e_argn);
*/	

    anna_native_method_create(
	type, -1, L"__in__", 0, 
	&anna_hash_in, 
	spec1,
	2, kv_argv, kv_argn);

    anna_native_method_create(
	type, -1, L"remove", 0, 
	&anna_hash_remove, 
	spec2,
	2, kv_argv, kv_argn);
    
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
	string_buffer_t sb;
	sb_init(&sb);
	sb_printf(&sb, L"HashMap«%ls,%ls»", subtype1->name, subtype2->name);
	spec = anna_type_native_create(sb_content(&sb), stack_global);
	sb_destroy(&sb);
	hash_put(&anna_hash_specialization, anna_tt_make(subtype1, subtype2), spec);
	anna_hash_type_create_internal(stack_global, spec, subtype1, subtype2);
	spec->flags |= ANNA_TYPE_SPECIALIZED;
	anna_type_copy_object(spec);
    }
    return spec;
}
