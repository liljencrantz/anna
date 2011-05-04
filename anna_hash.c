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

static ahi_callback_t *set_callback=0;

static hash_table_t anna_hash_specialization;

static inline int hash_entry_is_used(anna_hash_entry_t *e)
{
    return !!e->key;
}

static inline int hash_entry_is_dummy(anna_hash_entry_t *e)
{
    return !!e->value;
}

static inline void hash_entry_clear(anna_hash_entry_t *e)
{
    e->key = 0;
}

static inline anna_hash_t *ahi_unwrap(anna_object_t *obj)
{
    return (anna_hash_t *)anna_entry_get_addr(obj,ANNA_MID_HASH_PAYLOAD);
}

static inline void ahi_init(anna_hash_t *this)
{
    this->fill = this->used = 0;
    this->mask = ANNA_HASH_MINSIZE-1;
    this->table = &this->small_table[0];
    memset(&this->small_table[0], 0, sizeof(anna_hash_entry_t) * ANNA_HASH_MINSIZE);
}

static anna_vmstack_t *ahi_search_callback2(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t *eq = anna_vmstack_pop_entry(stack);
    int idx = anna_vmstack_pop_int(stack);
    int hash = anna_vmstack_pop_int(stack);
    anna_entry_t *aux = anna_vmstack_pop_entry(stack);
    ahi_callback_t *callback = (ahi_callback_t)anna_as_blob(anna_vmstack_pop_entry(stack));
    anna_entry_t *hash_obj = anna_vmstack_pop_entry(stack);
    anna_entry_t *key = anna_vmstack_pop_entry(stack);


    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash_obj));

    if(ANNA_VM_NULL(eq))
    {


	idx++;
	int pos = (hash+idx) & this->mask;

	wprintf(L"Position is non-empty, and keys are not equal. Check next position: %d.\n", pos);
	
	if(this->table[pos].key)
	{
	    wprintf(L"Position %d is non-empty, check if keys are equal\n", pos);
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
	    wprintf(L"Position %d is empty, there is no match! Run callback function %d\n", pos, *callback);
	    return (*callback)(stack, key, hash, hash_obj, aux, &this->table[pos]);
	}
    }
    else
    {
	int pos = (hash+idx) & this->mask;
	wprintf(L"Position %d is non-empty, but keys are equal. Found match, run callback function!\n", pos);
	return (*callback)(stack, key, hash, hash_obj, aux, &this->table[pos]);
    }
}

static anna_vmstack_t *ahi_search_callback(anna_vmstack_t *stack, anna_object_t *me)
{
    int hash = anna_vmstack_pop_int(stack);
    anna_entry_t *aux = anna_vmstack_pop_entry(stack);
    ahi_callback_t *callback = (ahi_callback_t)anna_as_blob(anna_vmstack_pop_entry(stack));
    anna_entry_t *hash_obj = anna_vmstack_pop_entry(stack);
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash_obj));
    anna_vmstack_pop_entry(stack);
    int pos = hash & this->mask;

    wprintf(L"Calculated hash value %d, maps top position %d\n", hash, pos);

    if(this->table[pos].key)
    {
	wprintf(L"Position %d is non-empty, check if keys are equal\n", pos);
	anna_entry_t *callback_param[] = 
	    {
		key,
		hash_obj,
		anna_from_blob(callback),
		aux,
		anna_from_int(hash),
		anna_from_int(0)		
	    }
	;

	anna_entry_t *o_param[] = 
	    {
		key,
		this->table[pos].key
	    }
	;
	
	anna_object_t *fun_object = anna_as_obj_fast(anna_entry_get_static(anna_as_obj(key)->type, ANNA_MID_EQ));
	return anna_vm_callback_native(
	    stack,
	    ahi_search_callback2, 6, callback_param,
	    fun_object, 2, o_param
	    );
    }
    else
    {
	wprintf(L"Position %d is empty, there is no match! Run callback function %d\n", pos, *callback);
	return (*callback)(stack, key, hash, hash_obj, aux, &this->table[pos]);
    }
}

static inline anna_vmstack_t *ahi_search(
    anna_vmstack_t *stack,
    anna_entry_t *key,
    anna_entry_t *hash,
    ahi_callback_t *callback,
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
    wprintf(L"Search for object of type %ls in hash table\n", o->type->name);
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

static inline anna_entry_t *anna_hash_init_i(anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    ahi_init(ahi_unwrap(this));
/*
    size_t i;
    if(likely(param[1] != null_object))
    {
	size_t sz = anna_list_get_size(param[1]);
    
	for(i=0; i<sz; i++)
	{
	    anna_object_t *pair = anna_list_get(param[1], i);
	    if(likely(pair != null_object))
	    {
		anna_object_t *key = anna_pair_get_first(pair);
		if(likely(key != null_object))
		    hash_put(hash, anna_pair_get_first(pair), anna_pair_get_second(pair));
	    }
	}	
    }
*/
    return param[0];
}
ANNA_VM_NATIVE(anna_hash_init, 2)

static anna_vmstack_t *anna_hash_set_callback(
    anna_vmstack_t *stack, 
    anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, 
    anna_hash_entry_t *hash_entry)
{
    wprintf(L"La la la %d %d\n", hash, hash_entry);
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash));
    if(!hash_entry_is_used(hash_entry))
    {
	this->used++;
	if(!hash_entry_is_dummy(hash_entry))
	{
	    this->fill++;
	}
    }

    wprintf(L"Hash table now has %d used slots and %d dummy slots\n", this->used, this->fill-this->used);
    
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
    
    if(ANNA_VM_NULL(key))
    {
	anna_vmstack_push_object(stack, null_object);
	return stack;
    }
    
    wprintf(L"Do a hash search with callback functino %d\n", &anna_hash_set_callback);
    
    return ahi_search(
	stack,
	key,
	this,
	set_callback,
	val);
}

static inline anna_entry_t *anna_hash_get_i(anna_entry_t **param)
{
    return 0;
}
ANNA_VM_NATIVE(anna_hash_get, 2)
/*
static inline anna_object_t *anna_hash_get_count_i(anna_object_t **param)
{
    return anna_int_create(hash_get_count(h_unwrap(param[0])));
}
ANNA_VM_NATIVE(anna_hash_get_count, 1)
*/
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
static void anna_hash_each_fun(void *keyp, void *valp, void *funp)
{
    anna_object_t *o_param[]=
	{
	    keyp, valp
	}
    ;   
    anna_vm_run(funp, 2, o_param);
}


static anna_object_t *anna_hash_each_i(anna_object_t **param)
{
    anna_object_t *body_object=param[1];
    
    hash_foreach2(h_unwrap(param[0]), anna_hash_each_fun, body_object);

    return param[0];
}
ANNA_VM_NATIVE(anna_hash_each, 2)

static inline anna_object_t *anna_hash_del_i(anna_object_t **param)
{
    hash_destroy(h_unwrap(param[0]));
    return param[0];
}
ANNA_VM_NATIVE(anna_hash_del, 1)
*/
/*

static inline anna_object_t *anna_hash_in_i(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    anna_object_t *res = hash_get(h_unwrap(param[0]), param[1]);
    return res ? anna_from_int(1) : null_object;
}
ANNA_VM_NATIVE(anna_hash_in, 2)
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
/*    
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


    anna_native_method_create(
	type, -1, L"__in__", 0, 
	&anna_hash_in, 
	int_type,
	2, kv_argv, kv_argn);
*/      

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
    set_callback = malloc(sizeof(ahi_callback_t));
    *set_callback = &anna_hash_set_callback;
    
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
