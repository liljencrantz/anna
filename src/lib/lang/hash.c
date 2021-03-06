//ROOT: src/lib/lang/lang.c
/*

  The implementation of a hashmap for Anna. Because of Annas support
  of continuations, this code is very hard to read or maintain. 

  The basic algorithm and implementation strategy is borrowed from
  Python's dict implementation, but because of the above mentioned
  continuation support, the code is custom written.
*/

/**
   The size of the builtin table used for very small hashes to avoid a
   memory allocation.
 */
#define ANNA_HASH_MINSIZE 4

/**
  The factor to increase hash size by when running full. Must be a
  power of two.
 */
#define ANNA_HASH_SIZE_STEP 2

/**
   The maximum allowed fill rate of the hash when inserting. Once this
   is reached, the hash is resized.  
*/
#define ANNA_HASH_USED_MAX 0.7

/**
   The minimum allowed fill rate of the hash when inserting. If this
   is reached during insertion, the hash is resized. Note that resizes
   only happen on insertions. This makes sure that if we remove most
   keys from a hash, one at a time, it won't resize until all keys are
   removed and we start inserting again.  
*/
#define ANNA_HASH_USED_MIN 0.1


typedef struct
{
    int hash;
    anna_entry_t key;
    anna_entry_t value;
}
    anna_hash_entry_t;

typedef struct {
    /** Number of storage slots occupied either by an actual value or
	a placeholder. */
    size_t fill;
    /** Number of key/value pairs actually stored */
    size_t used;
    /** 
	One less than the number of storage slots in the array pointed
	to by the table member.
     */
    size_t mask;
    anna_entry_t default_value;
    anna_hash_entry_t *table;
    anna_hash_entry_t small_table[ANNA_HASH_MINSIZE];
} anna_hash_t;

typedef void (*ahi_callback_t)(
    anna_context_t *context, anna_entry_t key,
    int hash_code, anna_entry_t hash,
    anna_entry_t aux, anna_hash_entry_t *hash_entry);

static hash_table_t anna_hash_specialization;
static array_list_t anna_hash_additional_methods = AL_STATIC;

static anna_function_t *anna_hash_specialize(
    anna_type_t *type, anna_function_t *fun)
{
    anna_node_call_t *spec = anna_node_create_call2(
	0, anna_node_create_null(0), 
	anna_node_create_dummy(
	    0, 
	    anna_type_wrap(
		anna_hash_get_key_type(type))),
	anna_node_create_dummy(
	    0,
	    anna_type_wrap(
		anna_hash_get_value_type(type))));
    anna_function_t *res = anna_function_compile_specialization(
	fun, spec);
    
    return res;
}

static void anna_hash_add_method_internal(
    anna_type_t *type, anna_function_t *fun)
{
    if(wcscmp(fun->name, L"__cmp__")==0)
    {
	if((mid_t)-1 == anna_type_find_comparator(
	       anna_hash_get_key_type(type)))
	    return;
	if((mid_t)-1 == anna_type_find_comparator(
	       anna_hash_get_value_type(type)))
	    return;
    }
    if(wcscmp(fun->name, L"hashCode")==0)
    {
	if((mid_t)-1 == anna_type_find_hash_code(
	       anna_hash_get_key_type(type)))
	    return;
	if((mid_t)-1 == anna_type_find_hash_code(
	       anna_hash_get_value_type(type)))
	    return;
    }
    anna_function_t *fun_spec = anna_hash_specialize(type, fun);
    if(fun_spec)
    {
	anna_member_create_method(
	    type, anna_mid_get(fun->name), fun_spec);
    }
}

static void add_hash_method(void *key, void *value, void *aux)
{
    anna_type_t *hash = (anna_type_t *)value;
    anna_function_t *fun = (anna_function_t *)aux;
    anna_hash_add_method_internal(hash, fun);
}

void anna_hash_add_method(anna_function_t *fun)
{
    al_push(&anna_hash_additional_methods, fun);
    hash_foreach2(&anna_hash_specialization, &add_hash_method, fun);
}

static void anna_hash_add_all_extra_methods(anna_type_t *hash)
{
    int i;
    for(i=0; i<al_get_count(&anna_hash_additional_methods); i++)
    {
	anna_function_t *fun =
	    (anna_function_t *)al_get(&anna_hash_additional_methods, i);
	anna_hash_add_method_internal(hash, fun);
    }
}

static inline int hash_entry_is_used(anna_hash_entry_t *e)
{
    return !!anna_as_obj_fast(e->key);
}

static inline int hash_entry_is_unused_and_not_dummy(anna_hash_entry_t *e)
{
    return !anna_as_obj_fast(e->value);
}

static inline int hash_entry_is_dummy(anna_hash_entry_t *e)
{
    return anna_entry_null_ptr(e->key) && !anna_entry_null_ptr(e->value);
}

static inline void hash_entry_clear(anna_hash_entry_t *e)
{
    e->key = anna_from_obj(0);
}

static inline void hash_entry_clear_full(anna_hash_entry_t *e)
{
    e->value = anna_from_obj(0);
    e->key = anna_from_obj(0);
}

static inline anna_hash_t *ahi_unwrap(anna_object_t *obj)
{
    return (anna_hash_t *)anna_entry_get_addr(obj,ANNA_MID_HASH_PAYLOAD);
}

#if 0
static void ahi_validate(anna_hash_t *hash)
{
    assert(hash->used < hash->mask+1);
    assert(hash->fill < hash->mask+1);
    assert(hash->fill >= hash->used);
}
#endif
static inline size_t anna_hash_get_count(anna_object_t *this)
{
    return ahi_unwrap(this)->used;
}

static inline size_t anna_hash_get_version(anna_object_t *this)
{
    return ahi_unwrap(this)->used;
}

static inline anna_entry_t anna_hash_get_key_from_idx(
    anna_object_t *this, int idx)
{
    return ahi_unwrap(this)->table[idx].key;
}

static inline anna_entry_t anna_hash_get_value_from_idx(
    anna_object_t *this, int idx)
{
    return ahi_unwrap(this)->table[idx].value;
}

__attr_unused static void anna_hash_print(anna_hash_t *this)
{
    int i;
    for(i=0; i<=this->mask; i++)
    {
	anna_message(L"%d:\t", i);
	if(hash_entry_is_used(&this->table[i]))
	{
	    anna_message(L"%d", this->table[i].hash);	    
	    anna_entry_t e = this->table[i].key;
	    if(anna_is_int_small(e))
		anna_message(L": %d", anna_as_int(e));	    
	    else if(anna_is_obj(e))
	    {
		anna_object_t *o = anna_as_obj_fast(e);
		if(o->type == string_type)
		{
		    anna_message(L": %ls", anna_string_payload(o));
		}
		else
		{
		    anna_message(L": %ls", o->type->name);
		}
	    }

	}
	else if(hash_entry_is_dummy(&this->table[i]))
	{
	    anna_message(L"dummy");	    
	}
	else
	{
	    anna_message(L"empty");	    
	}
	
	anna_message(L"\n");
    }
    
}

static inline ssize_t anna_hash_get_next_idx(
    anna_object_t *this, ssize_t idx)
{
    if(idx == -1)
    {
	return -1;
    }
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

static inline ssize_t anna_hash_get_next_non_dummy(
    anna_object_t *this, ssize_t idx)
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

static inline void ahi_init(anna_hash_t *this, int set_default)
{
    this->fill = this->used = 0;
    this->mask = ANNA_HASH_MINSIZE-1;
    this->table = &this->small_table[0];
    memset(
	&this->small_table[0], 0, 
	sizeof(anna_hash_entry_t) * ANNA_HASH_MINSIZE);
    if(set_default)
    {
	this->default_value = null_entry;
    }
}


static void anna_hash_resize(anna_hash_t *this, size_t new_sz)
{
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
	new_table = calloc(new_sz, sizeof(anna_hash_entry_t));
    }
    int i, j;
    for(i=0; i<old_sz; i++){
	anna_hash_entry_t *e = &this->table[i];
	if(hash_entry_is_used(e))
	{
	    anna_hash_entry_t *new_e;
	    for(j=0; 1; j++)
	    {
		new_e= &new_table[(e->hash+j) & new_mask];
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
//    anna_message(L"After\n");
//    anna_hash_print(this);
}

static void anna_hash_check_resize(anna_hash_t *this)
{
    size_t old_sz = this->mask+1;
//    if(old_sz <= 64)
//	anna_message(L"%d: %f > %d?\n", old_sz, ANNA_HASH_USED_MIN*old_sz, this->used);
    if(ANNA_HASH_USED_MAX*old_sz < this->used+1)
    {
	size_t new_sz = ANNA_HASH_SIZE_STEP * old_sz;
	anna_hash_resize(this, new_sz);
    }
    else if( ANNA_HASH_USED_MIN*old_sz > this->used+1)
    {
	size_t new_sz = old_sz;
	do
	{
	    new_sz /= ANNA_HASH_SIZE_STEP;
	}
	while(ANNA_HASH_USED_MIN*new_sz > (this->used+1) && new_sz >= ANNA_HASH_MINSIZE);
	anna_hash_resize(this, new_sz);
    }
}

static void ahi_search_callback2(anna_context_t *context);
static void ahi_search_callback2_internal(
    anna_context_t *context, 
    anna_entry_t key,
    anna_entry_t hash_obj,
    ahi_callback_t callback,
    anna_entry_t aux,
    int hash,
    int idx,
    int dummy_idx,
    anna_entry_t eq);


static inline void ahi_search_callback2_next(
    anna_context_t *context, 
    anna_entry_t hash_obj, 
    anna_entry_t key,
    ahi_callback_t callback,
    anna_entry_t aux,
    int hash,
    int idx,
    int dummy_idx,
    int pos)
{
    
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash_obj));

    if(anna_is_obj(key))
    {
	anna_object_t *o = anna_as_obj_fast(key);
	if((o->type == mutable_string_type) ||
	   (o->type == imutable_string_type))
	{
	    if(anna_is_obj(this->table[pos].key))
	    {
		anna_object_t *o2 = anna_as_obj_fast(this->table[pos].key);
		if((o2->type == mutable_string_type) ||
		   (o2->type == imutable_string_type))
		{
		    int eq = anna_string_cmp(o, o2);
		    return ahi_search_callback2_internal(
			context,
			key,
			hash_obj,
			callback,
			aux,
			hash,
			idx,
			dummy_idx,
			anna_from_int(eq));
		}
	    }
	    
	}
	
    }
    else
    {
	if(anna_is_int_small(key) && anna_is_int_small(this->table[pos].key))
	{
	    int eq = (int)sign(anna_as_int(key) - anna_as_int(this->table[pos].key));
	    return ahi_search_callback2_internal(
		context,
		key,
		hash_obj,
		callback,
		aux,
		hash,
		idx,
		dummy_idx,
		anna_from_int(eq));
	}
    }
    
    anna_entry_t callback_param[] = 
	{
	    key,
	    hash_obj,
	    anna_from_blob(callback),
	    aux,
	    anna_from_int(hash),
	    anna_from_int(idx),
	    anna_from_int(dummy_idx)
	}
    ;
    
    anna_entry_t o_param[] = 
	{
	    key,
	    this->table[pos].key
	}
    ;
    
    anna_object_t *fun_object = anna_as_obj_fast(
	anna_entry_get_static(
	    anna_as_obj(key)->type, 
	    (mid_t)(long)anna_entry_get_static_ptr(
		anna_as_obj_fast(hash_obj)->type,
		ANNA_MID_COMPARATOR)));
    anna_vm_callback_native(
	context,
	ahi_search_callback2, 7, callback_param,
	fun_object, 2, o_param
	);
}

static void ahi_search_callback2_internal(
    anna_context_t *context, 
    anna_entry_t key,
    anna_entry_t hash_obj,
    ahi_callback_t callback,
    anna_entry_t aux,
    int hash,
    int idx,
    int dummy_idx,
    anna_entry_t eq)
{
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash_obj));

    if(anna_entry_null(eq) || anna_as_int(eq) != 0)
    {
	idx++;
	if(dummy_idx == -1)
	{
	    int d_pos = (hash+idx) & this->mask;
	    if(hash_entry_is_dummy(&this->table[d_pos]))
	    {
		dummy_idx = d_pos;
	    }
	}
	int pos = anna_hash_get_next_non_dummy(anna_as_obj(hash_obj), hash+idx);
	anna_hash_entry_t *e = &this->table[pos];
	idx = pos-hash;
	//anna_message(L"Position is non-empty, and keys are not equal. Check next position: %d (idx %d).\n", pos, idx);
	
	if(hash_entry_is_used(e))
	{
	    //  anna_message(L"Position %d (idx %d) is non-empty, check if keys are equal\n", pos, idx);
	    ahi_search_callback2_next(
		context, 
		hash_obj,
		key,
		callback,
		aux,
		hash,
		idx,
		dummy_idx,
		pos);
	}
	else
	{	    
//	    anna_message(L"Position %d (idx %d) is empty. Yay, object does not exist\n", pos, idx);
	    callback(context, key, hash, hash_obj, aux, &this->table[(dummy_idx != -1 )? dummy_idx:pos]);
	}
    }
    else
    {
	int pos = (hash+idx) & this->mask;
//	anna_message(L"Position %d is non-empty, but keys are equal. Found match, run callback function!\n", pos);
	callback(context, key, hash, hash_obj, aux, &this->table[pos]);
    }
}

static void ahi_search_callback2(anna_context_t *context)
{
    anna_entry_t eq = anna_context_pop_entry(context);
    int dummy_idx = anna_context_pop_int(context);
    int idx = anna_context_pop_int(context);
    int hash = anna_context_pop_int(context);
    anna_entry_t aux = anna_context_pop_entry(context);
    ahi_callback_t callback = (ahi_callback_t)anna_as_blob(anna_context_pop_entry(context));
    anna_entry_t hash_obj = anna_context_pop_entry(context);
    anna_entry_t key = anna_context_pop_entry(context);
    ahi_search_callback2_internal(
	context, key, hash_obj, callback, aux, hash, idx, dummy_idx, eq);
}

/**
   Add a bit of extra bit mangling to all hash values.
 */
static inline int anna_hash_mangle(int a)
{
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    a = a & ANNA_INT_FAST_MAX;
    return a;
}

static inline void ahi_search_callback_internal(
    anna_context_t *context, 
    anna_entry_t hash_obj, 
    anna_entry_t key,
    ahi_callback_t callback,
    anna_entry_t aux,
    int hash)
{
    hash = anna_hash_mangle(hash);
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash_obj));
    int pos = anna_hash_get_next_non_dummy(anna_as_obj(hash_obj), hash);
    anna_hash_entry_t *e = &this->table[pos];
    int idx = pos-hash;
    
    //anna_message(L"Calculated hash value %d, maps to position %d, first non-dummy is %d\n", hash, hash & this->mask, pos);
    if(hash_entry_is_used(e))
    {
	int d_pos = hash & this->mask;
	
//	anna_message(L"Position %d (idx %d) is non-empty, check if keys are equal\n", pos, idx);
	ahi_search_callback2_next(
	    context, 
	    hash_obj,
	    key,
	    callback,
	    aux,
	    hash,
	    idx,
	    hash_entry_is_dummy(&this->table[d_pos]) ? d_pos : -1,
	    pos);
    }
    else
    {
//	anna_message(L"Position %d (idx %d) is empty\n", pos, idx);
	callback(context, key, hash, hash_obj, aux, &this->table[pos]);
    }
}

static void ahi_search_callback(anna_context_t *context)
{
    int hash = anna_context_pop_int(context);
    anna_entry_t aux = anna_context_pop_entry(context);
    ahi_callback_t callback = (ahi_callback_t)anna_as_blob(anna_context_pop_entry(context));
    anna_entry_t hash_obj = anna_context_pop_entry(context);
    anna_entry_t key = anna_context_pop_entry(context);
    anna_context_pop_entry(context);
    
    ahi_search_callback_internal(
	context,
	hash_obj,
	key,
	callback,
	aux, hash);
}

static inline void ahi_search(
    anna_context_t *context,
    anna_entry_t key,
    anna_entry_t hash,
    ahi_callback_t callback,
    anna_entry_t aux)
{
//    ahi_validate(ahi_unwrap(anna_as_obj(hash)));
    if(anna_is_obj(key))
    {
	anna_object_t *o = anna_as_obj_fast(key);
	if((o->type == mutable_string_type) ||
	   (o->type == imutable_string_type))
	{
	    ahi_search_callback_internal(
		context,
		hash,
		key,
		callback,
		aux,
		anna_string_hash(o));
	    return;
	}
    }
    else if(anna_is_int_small(key))
    {
//	anna_message(L"Search for Int in hash table\n");
	ahi_search_callback_internal(
	    context,
	    hash,
	    key,
	    callback,
	    aux,
	    anna_as_int(key));
	return;
    }
    
    anna_entry_t callback_param[] = 
	{
	    key,
	    hash,
	    anna_from_blob(callback),
	    aux
	}
    ;
    
    anna_object_t *o = anna_as_obj(key);
//    anna_message(L"Search for object of type %ls in hash table\n", o->type->name);
    anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_HASH_CODE);
    anna_object_t *meth = anna_as_obj_fast(o->type->static_member[tos_mem->offset]);
    anna_vm_callback_native(
	context,
	ahi_search_callback, 4, callback_param,
	meth, 1, (anna_entry_t *)&o
	);
}


static inline hash_table_t *h_unwrap(anna_object_t *obj)
{
    return (hash_table_t *)anna_entry_get_addr(obj,ANNA_MID_HASH_PAYLOAD);
}

anna_object_t *anna_hash_create(anna_type_t *spec1, anna_type_t *spec2)
{
    return anna_object_create(anna_hash_type_get(spec1, spec2));
}

anna_object_t *anna_hash_create2(anna_type_t *hash_type)
{
    return  anna_object_create(hash_type);
}

static void anna_hash_type_init(anna_object_t *obj)
{
    ahi_init(ahi_unwrap(obj), 1);
    obj->flags |= ANNA_OBJECT_HASH;
 }


static inline void anna_hash_set_entry(anna_hash_t *this, anna_hash_entry_t *hash_entry, int hash_code, anna_entry_t key, anna_entry_t value)
{
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
    hash_entry->value = value;
}

static void anna_hash_init(anna_context_t *context)
{
    anna_object_t *list = anna_context_pop_object(context);
    anna_object_t *this = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    
    if((list != null_object) && anna_list_get_count(list) != 0)
    {
	anna_entry_t argv[] =
	    {
		anna_from_obj(this),
		anna_from_obj(list)
	    }
	;
	anna_entry_t fun = *anna_entry_get_addr(this, anna_mid_get(L"__setAll__"));
	anna_vm_callback(
	    context,
	    anna_as_obj(fun), 2, argv);
	return;
    }
    anna_context_push_object(context, this);
}

static __attribute__((aligned(8))) void anna_hash_set_callback(
    anna_context_t *context, 
    anna_entry_t key, int hash_code, anna_entry_t hash, anna_entry_t aux, 
    anna_hash_entry_t *hash_entry)
{
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash));
    anna_hash_set_entry(this, hash_entry, hash_code, key, aux);    
    anna_context_push_entry(context, aux);
}

static inline void anna_hash_set(anna_context_t *context)
{
    anna_entry_t val = anna_context_pop_entry(context);
    anna_entry_t key = anna_context_pop_entry(context);
    anna_entry_t this = anna_context_pop_entry(context);
    anna_context_pop_entry(context);
    
    if(anna_entry_null(key))
    {
	anna_context_push_object(context, null_object);
	return;
    }
    
    anna_hash_check_resize(ahi_unwrap(anna_as_obj_fast(this)));
    
    ahi_search(
	context,
	key,
	this,
	anna_hash_set_callback,
	val);
}

static __attribute__((aligned(8))) void anna_hash_get_callback(
    anna_context_t *context, 
    anna_entry_t key, int hash_code, anna_entry_t hash, anna_entry_t aux, 
    anna_hash_entry_t *hash_entry)
{
    if(!hash_entry_is_used(hash_entry))
    {
	anna_context_push_object(
	    context,
	    anna_as_obj(ahi_unwrap(anna_as_obj_fast(hash))->default_value));	
    }
    else
    {
	anna_context_push_entry(context, hash_entry->value);	
    }
}

static inline void anna_hash_get(anna_context_t *context)
{
    anna_entry_t key = anna_context_pop_entry(context);
    anna_entry_t this = anna_context_pop_entry(context);
    anna_context_pop_entry(context);
    
    if(anna_entry_null(key))
    {
	anna_context_push_object(context, null_object);
    }
    else 
    {
	ahi_search(
	    context,
	    key,
	    this,
	    anna_hash_get_callback,
	    anna_from_obj(0));
    }
}



static __attribute__((aligned(8))) void anna_hash_in_callback(
    anna_context_t *context, 
    anna_entry_t key, int hash_code, anna_entry_t hash, anna_entry_t aux, 
    anna_hash_entry_t *hash_entry)
{
    if(!hash_entry_is_used(hash_entry))
    {
	anna_context_push_object(context, null_object);	
    }
    else
    {
	anna_context_push_entry(context, hash_entry->key);	
    }
}

static inline void anna_hash_in(anna_context_t *context)
{
    anna_entry_t key = anna_context_pop_entry(context);
    anna_entry_t this = anna_context_pop_entry(context);
    anna_context_pop_entry(context);
    
    if(anna_entry_null(key))
    {
	anna_context_push_object(context, null_object);
    }
    else
    {
	ahi_search(
	    context,
	    key,
	    this,
	    anna_hash_in_callback,
	    anna_from_obj(0));
    }
}

static __attribute__((aligned(8))) void anna_hash_remove_callback(
    anna_context_t *context, 
    anna_entry_t key, int hash_code, anna_entry_t hash, anna_entry_t aux, 
    anna_hash_entry_t *hash_entry)
{
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash));

//    anna_message(L"Returned element %d\n", hash_entry - this->table);    
    
    if(hash_entry_is_used(hash_entry))
    {
	anna_context_push_entry(context, hash_entry->key);	
	int next_idx = (hash_entry - this->table + 1) & this->mask;
	anna_hash_entry_t *next = &this->table[next_idx];
	if(hash_entry_is_unused_and_not_dummy(next))
	{
	    hash_entry_clear_full(hash_entry);
	    this->fill--;
	}
	else  /**/
	{
	    hash_entry_clear(hash_entry);
	}
	this->used--;
    }
    else
    {
//	anna_message(L"Failed to remove element %ls with hash code %d (%d), which maps to pos %d\n", anna_string_payload(key), hash_code, anna_hash_mangle(anna_string_hash(key)), hash_code & this->mask);

	anna_context_push_object(context, null_object);	
    }
}

static inline void anna_hash_remove(anna_context_t *context)
{
    anna_entry_t key = anna_context_pop_entry(context);
    anna_entry_t this = anna_context_pop_entry(context);
    anna_context_pop_entry(context);
//    anna_hash_print(ahi_unwrap(this));
    if(anna_entry_null(key))
    {
	anna_context_push_object(context, null_object);
	return;
    }
    
    ahi_search(
	context,
	key,
	this,
	anna_hash_remove_callback,
	anna_from_obj(0));
}

ANNA_VM_NATIVE(anna_hash_get_count_method, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(param[0]));
    return anna_from_int(this->used);
}

ANNA_VM_NATIVE(anna_hash_clear, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(param[0]));
    if(this->table != &this->small_table[0])
    {
	free(this->table);
    }
    ahi_init(this, 0);
    
    return param[0];
}

ANNA_VM_NATIVE(anna_hash_get_default, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(param[0]));
    return this->default_value;
}

ANNA_VM_NATIVE(anna_hash_set_default, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(param[0]));
    return this->default_value = param[1];
}

ANNA_VM_NATIVE(anna_hash_get_keys, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *this_obj = anna_as_obj_fast(param[0]);
    anna_hash_t *this = ahi_unwrap(this_obj);
    anna_object_t *res = anna_list_create_mutable(
	anna_hash_get_key_type(this_obj->type));
    int i;
    size_t sz = this->mask+1;
    
    for(i=0; i<sz; i++){
	anna_hash_entry_t *e = &this->table[i];
	if(hash_entry_is_used(e))
	{
	    anna_list_push(
		res,
		e->key);
	}
    }
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_hash_get_values, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *this_obj = anna_as_obj_fast(param[0]);
    anna_hash_t *this = ahi_unwrap(this_obj);
    anna_object_t *res = anna_list_create_mutable(
	anna_hash_get_value_type(this_obj->type));
    int i;
    size_t sz = this->mask+1;
    
    for(i=0; i<sz; i++){
	anna_hash_entry_t *e = &this->table[i];
	if(hash_entry_is_used(e))
	{
	    anna_list_push(
		res,
		e->value);
	}
    }
    return anna_from_obj(res);
}

static void anna_hash_del(anna_object_t *victim)
{
    anna_hash_t *this = ahi_unwrap(victim);
    if(this->table != this->small_table)
    {
	free(this->table);
    }
}

anna_type_t *anna_hash_get_key_type(anna_type_t *type)
{
    if(!anna_entry_get_addr_static(type,ANNA_MID_HASH_SPECIALIZATION1))
    {
	return 0;
    }    
    return (*(anna_type_t **)anna_entry_get_addr_static(type,ANNA_MID_HASH_SPECIALIZATION1));
}

anna_type_t *anna_hash_get_value_type(anna_type_t *type)
{
    if(!anna_entry_get_addr_static(type,ANNA_MID_HASH_SPECIALIZATION2))
    {
	return 0;
    }
    return (*(anna_type_t **)anna_entry_get_addr_static(type,ANNA_MID_HASH_SPECIALIZATION2));
}

static void anna_hash_iterator_update(anna_object_t *iter)
{
    anna_object_t *hash_obj = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    anna_hash_t *hash = ahi_unwrap(hash_obj);
    int current_idx = anna_as_int(anna_entry_get(iter, ANNA_MID_OFFSET));
    current_idx = anna_hash_get_next_idx(hash_obj, current_idx);
    
    if(current_idx == -1)
    {
	anna_entry_set(iter, ANNA_MID_KEY, null_entry);
	anna_entry_set(iter, ANNA_MID_OFFSET, anna_from_int(current_idx));
    }
    else
    {
	anna_entry_set(iter, ANNA_MID_KEY, hash->table[current_idx].key);
	anna_entry_set(iter, ANNA_MID_OFFSET, anna_from_int(current_idx+1));
    }
}

ANNA_VM_NATIVE(anna_hash_get_iterator, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *hash = anna_as_obj(param[0]);
    anna_object_t *iter = anna_object_create(
	anna_type_unwrap(anna_entry_get_static_obj(hash->type, ANNA_MID_ITERATOR_TYPE)));
    anna_entry_set(iter, ANNA_MID_COLLECTION, param[0]);
    anna_entry_set(iter, ANNA_MID_VERSION_ID, anna_from_int(anna_hash_get_version(hash)));
    
    anna_entry_set(iter, ANNA_MID_OFFSET, anna_from_int(0));
    anna_hash_iterator_update(iter);
    return anna_from_obj(iter);
}

ANNA_VM_NATIVE(anna_hash_iterator_next, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_hash_iterator_update(iter);
    return param[0];
}

ANNA_VM_NATIVE(anna_hash_iterator_get_value, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *hash_obj = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    anna_hash_t *hash = ahi_unwrap(hash_obj);
    int current_idx = anna_as_int(anna_entry_get(iter, ANNA_MID_OFFSET))-1;
    
    if(current_idx == -1)
    {
	return null_entry;
    }
    else
    {
	return hash->table[current_idx].value;
    }
}

ANNA_VM_NATIVE(anna_hash_iterator_set_value, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *hash_obj = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    anna_hash_t *hash = ahi_unwrap(hash_obj);
    int current_idx = anna_as_int(anna_entry_get(iter, ANNA_MID_OFFSET))-1;
    
    if(current_idx != -1)
    {
	hash->table[current_idx].value = param[1];
    }

    return param[1];
}

ANNA_VM_NATIVE(anna_hash_iterator_valid, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    int current_idx = anna_as_int(anna_entry_get(iter, ANNA_MID_OFFSET));
    return (current_idx == -1) ? null_entry : anna_from_int(1);
}


static anna_type_t *anna_hash_iterator_create(
    anna_type_t *type,
    anna_type_t *spec1,
    anna_type_t *spec2)
{
    
    anna_type_t *iter = anna_type_create(L"Iterator", 0);
    anna_member_create(
	iter, ANNA_MID_COLLECTION, ANNA_MEMBER_IMUTABLE, type);    
    anna_member_create(
	iter, ANNA_MID_KEY, ANNA_MEMBER_IMUTABLE, spec1);

    anna_member_create_native_property(
	iter, ANNA_MID_VALUE, spec2,
	&anna_hash_iterator_get_value,
	&anna_hash_iterator_set_value, 0);
    
    anna_member_create(
	iter, ANNA_MID_VERSION_ID, ANNA_MEMBER_IMUTABLE, int_type);    
    anna_member_create(
	iter, ANNA_MID_OFFSET, ANNA_MEMBER_IMUTABLE | ANNA_MEMBER_INTERNAL, int_type);    
    anna_type_copy_object(iter);
    
    anna_member_create_native_property(
	iter, ANNA_MID_VALID, any_type,
	&anna_hash_iterator_valid,
	0, 0);

    anna_type_t *iter_argv[] = 
	{
	    iter
	}
    ;
    
    wchar_t *iter_argn[]=
	{
	    L"this"
	}
    ;

    anna_member_create_native_method(
	iter,
	ANNA_MID_NEXT_ASSIGN, 0,
	&anna_hash_iterator_next, iter, 1,
	iter_argv, iter_argn, 0, 0);

    anna_util_iterator_iterator(iter);
        
    anna_type_close(iter);
    
    return iter;
}

static void anna_hash_type_create_internal(
    anna_type_t *type, 
    anna_type_t *spec1,
    anna_type_t *spec2)
{
    anna_member_create_blob(
	type, ANNA_MID_HASH_PAYLOAD, 0,
	sizeof(anna_hash_t));
    
    anna_member_create(
	type, ANNA_MID_HASH_SPECIALIZATION1, 1, null_type);
    anna_member_create(
	type, ANNA_MID_HASH_SPECIALIZATION2, 1, null_type);
    anna_member_create(
	type, ANNA_MID_COMPARATOR, 1, null_type);

    anna_entry_set_static_ptr(type,ANNA_MID_HASH_SPECIALIZATION1, spec1);
    anna_entry_set_static_ptr(type,ANNA_MID_HASH_SPECIALIZATION2, spec2);
    mid_t cmp_mid = anna_type_find_comparator(spec1);

    if(cmp_mid == (mid_t)-1)
    {
	anna_error(0, L"The type %ls can't be used as a HashMap key, it does not provide a valid comparison function.", spec1->name);
	assert(0);
    }
    else
    {
	anna_entry_set_static(type,ANNA_MID_COMPARATOR, (anna_entry_t )(long)cmp_mid);
    }
    
    anna_member_create(
	type,
	ANNA_MID_ITERATOR_TYPE,
	ANNA_MEMBER_STATIC,
	type_type);
    anna_type_t *iter = anna_hash_iterator_create(type, spec1, spec2);
    anna_entry_set_static(
	type, ANNA_MID_ITERATOR_TYPE, 
	anna_from_obj(anna_type_wrap(iter)));
    anna_member_create_native_property(
	type, ANNA_MID_ITERATOR, iter,
	&anna_hash_get_iterator, 0,
	L"Returns an Iterator for this collection.");
    
    anna_type_set_initializer(type, &anna_hash_type_init);

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

    anna_member_create_native_method(
	type, ANNA_MID_SET_OP, 
	0, &anna_hash_set, spec2, 
	3, kv_argv, kv_argn, 0,
	L"Assigns the specified value to the specified key.");

    anna_member_create_native_method(
	type, ANNA_MID_CLEAR, 
	0, &anna_hash_clear, type, 
	1, kv_argv, kv_argn, 0,
	L"Clear the HashMap, removing all mappings from it. The result is an empty HashMap.");

    anna_member_create_native_method(
	type, ANNA_MID_GET_OP,
	0, &anna_hash_get,
	spec2, 2, kv_argv, kv_argn, 0,
	L"Returns the value associated with the specified key.");

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
    
    anna_member_create_native_method(
	type, ANNA_MID_INIT,
	ANNA_FUNCTION_VARIADIC, &anna_hash_init,
	type, 2, i_argv, i_argn, 0, 0);

    anna_member_create_native_property(
	type, ANNA_MID_COUNT, int_type,
	&anna_hash_get_count_method, 0,
	L"The number of elements in this Map.");

    anna_member_create_native_property(
	type, anna_mid_get(L"default"), spec2,
	&anna_hash_get_default, &anna_hash_set_default,
	L"The default value returned by this map if the specified key does not exist.");

    anna_member_create_native_property(
	type, anna_mid_get(L"keys"), anna_list_type_get_mutable(spec1),
	&anna_hash_get_keys, 0,
	L"Returns a list containing all the keys of this HashMap.");

    anna_member_create_native_property(
	type, anna_mid_get(L"values"), anna_list_type_get_mutable(spec2),
	&anna_hash_get_values, 0,
	L"Returns a list containing all the values of this HashMap.");

    anna_type_finalizer_add(
	type, anna_hash_del);

    anna_member_create_native_method(
	type, ANNA_MID_IN, 0,
	&anna_hash_in, spec1, 2, kv_argv,
	kv_argn, 0, 
	L"Returns true if the specified key exists in the Hash");

    anna_member_create_native_method(
	type,
	anna_mid_get(L"remove"),
	0,
	&anna_hash_remove,
	spec2,
	2,
	kv_argv,
	kv_argn, 0,
	L"Remove the mapping with the specified key from the map");
    
    anna_hash_add_all_extra_methods(type);

    anna_type_document(
	type,
	L"A HashMap represents a set of mappings from keys to values.");
    
    anna_type_document(
	type,
	L"Internally, the HashMap type is implemented as a Hash table. Every key must implement the equality comapison method and the hashCode method.");
    
    anna_type_document(
	type,
	L"Setting new mappings and getting the corresponding value for a specified key both have an O(1) amortized time, provided that the keys correctly implement the hashCode method.");

    anna_type_close(type);    
}

static inline void anna_hash_internal_init()
{
    static int init = 0;
    if(likely(init))
	return;
    init=1;
    hash_init(&anna_hash_specialization, hash_tt_func, hash_tt_cmp);
}

ANNA_VM_NATIVE(anna_hash_key_cmp, 2)
{
    return null_entry;
}

ANNA_VM_NATIVE(anna_hash_key_hash, 1)
{
    return null_entry;
}

static anna_type_t *anna_hash_specialize_node(
    anna_type_t *base, anna_node_call_t *call,
    anna_stack_template_t *stack)
{
    if(call->child_count != 2)
    {
	anna_error((anna_node_t *)call, L"Invalid number of template arguments to hash specialization");
	return 0;
    }
    
    call->child[0] = anna_node_calculate_type(call->child[0]);	
    call->child[1] = anna_node_calculate_type(call->child[1]);	
    anna_type_t *spec1 = anna_node_resolve_to_type(call->child[0], stack);
    anna_type_t *spec2 = anna_node_resolve_to_type(call->child[1], stack);

    if(spec1 && spec2)
    {
	return anna_hash_type_get(spec1, spec2);
    }
    
    anna_error((anna_node_t *)call, L"HashMap specializations can not be resolved into types");
    return 0;
}

void anna_hash_type_create()
{
    anna_type_t *hk_argv[] = 
	{
	    hash_key_type,
	    hash_key_type
	};
    wchar_t *hk_argn[] = 
	{
	    L"this", L"other"
	};

    anna_type_document(
	hash_key_type,
	L"An abstract type representing containing all the members needed in order to be used as a key in a HashMap.");


    anna_member_create_native_method(
	hash_key_type,
	ANNA_MID_INIT,
	0,
	&anna_vm_null_function,
	hash_key_type,
	1,
	hk_argv,
	hk_argn, 0, 
	0);
    

    anna_member_create_native_method(
	hash_key_type,
	ANNA_MID_CMP,
	0,
	&anna_hash_key_cmp,
	int_type,
	2,
	hk_argv,
	hk_argn, 0, 
	L"Comparison method. Should return a negative number, zero or a positive number if the compared object is smaller than, equal to or greater than the object being called, respectively. If the objects can't be compared, null should be returned.");
    anna_member_create_native_method(
	hash_key_type, ANNA_MID_HASH_CODE, 0,
	&anna_hash_key_hash, int_type, 1, hk_argv,
	hk_argn, 0,
	L"Hash function. Should always return the same number for the same object and should also return the same number for two equal objects.");

    anna_hash_internal_init();
    hash_type->specialization_function = anna_hash_specialize_node;
    hash_put(&anna_hash_specialization, anna_tt_make(hash_key_type, any_type), hash_type);
    anna_alloc_mark_permanent(hash_type);
    anna_hash_type_create_internal(hash_type, hash_key_type, any_type);
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
	spec = anna_type_create(sb_content(&sb), 0);
	sb_destroy(&sb);
	hash_put(&anna_hash_specialization, anna_tt_make(subtype1, subtype2), spec);
	anna_alloc_mark_permanent(spec);
	anna_hash_type_create_internal(spec, subtype1, subtype2);
	spec->flags |= ANNA_TYPE_SPECIALIZED;
	anna_type_copy_object(spec);
    }
    return spec;
}

void anna_hash_mark(anna_object_t *obj)
{
    anna_hash_t *this = ahi_unwrap(obj);
//    anna_message(L"HASHMARK %ls %d %d %d %d\n", obj->type->name, obj, null_entry, this->mask, this->table);
    int i;
    for(i=0; i<=this->mask; i++)
    {
	if(hash_entry_is_used(&this->table[i]))
	{
	    anna_alloc_mark_entry(this->table[i].key);
	    anna_alloc_mark_entry(this->table[i].value);
	}
    }
}

