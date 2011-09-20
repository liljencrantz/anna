/*

  The implementation of a hashmap for Anna. Because of Annas support
  of continuations, this code is very hard to read or maintain. 

  The basic algorithm and implementation strategy is borrowed from
  Python's dict implementation.
 */

/**
   The size of the builtin table used for very small hashes to avoid a memory allocation.
 */
#define ANNA_HASH_MINSIZE 16

/**
  The factor to increase hash size by when running full. Must be a power of two.
 */
#define ANNA_HASH_SIZE_STEP 2

/**
   The maximum allowed fill rate of the hash when inserting
 */
#define ANNA_HASH_USED_MAX 0.7

/**
   The minimum allowed fill rate of the hash when inserting
 */
#define ANNA_HASH_USED_MIN 0.2


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

typedef void (*ahi_callback_t)(anna_vmstack_t *stack, anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, anna_hash_entry_t *hash_entry);

static hash_table_t anna_hash_specialization;
static array_list_t anna_hash_additional_methods = AL_STATIC;

static void add_hash_method(void *key, void *value, void *aux)
{
    anna_type_t *hash = (anna_type_t *)value;
    anna_function_t *fun = (anna_function_t *)aux;
//    wprintf(L"Add function %ls to type %ls\n", fun->name, hash->name);
    anna_member_create_method(hash, anna_mid_get(fun->name), fun);
}

void anna_hash_add_method(anna_function_t *fun)
{
//    wprintf(L"Function %ls to all hash types\n", fun->name);
    al_push(&anna_hash_additional_methods, fun);
    hash_foreach2(&anna_hash_specialization, &add_hash_method, fun);
}

static void anna_hash_add_all_extra_methods(anna_type_t *hash)
{
    int i;
    for(i=0; i<al_get_count(&anna_hash_additional_methods); i++)
    {
	anna_function_t *fun = (anna_function_t *)al_get(&anna_hash_additional_methods, i);
//	wprintf(L"Add function %ls to type %ls\n", fun->name, hash->name);
	anna_member_create_method(hash, anna_mid_get(fun->name), fun);
    }
}



static inline int hash_entry_is_used(anna_hash_entry_t *e)
{
    return !!e->key;
}

static inline int hash_entry_is_unused_and_not_dummy(anna_hash_entry_t *e)
{
    return !e->value;
}

static inline int hash_entry_is_dummy(anna_hash_entry_t *e)
{
    return !e->key && !!e->value ;
}

static inline void hash_entry_clear(anna_hash_entry_t *e)
{
    e->key = 0;
}

static inline void hash_entry_clear_full(anna_hash_entry_t *e)
{
    e->value = 0;
    e->key = 0;
}

static inline anna_hash_t *ahi_unwrap(anna_object_t *obj)
{
    return (anna_hash_t *)anna_entry_get_addr(obj,ANNA_MID_HASH_PAYLOAD);
}

static inline size_t anna_hash_get_count(anna_object_t *this)
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

__attr_unused static void anna_hash_print(anna_hash_t *this)
{
    int i;
    for(i=0; i<=this->mask; i++)
    {
	wprintf(L"%d:\t", i);
	if(hash_entry_is_used(&this->table[i]))
	{
	    wprintf(L"%d", this->table[i].hash);	    
	    anna_entry_t *e = this->table[i].key;
	    if(anna_is_int_small(e))
		wprintf(L": %d", anna_as_int(e));	    
	    else if(anna_is_obj(e))
	    {
		anna_object_t *o = anna_as_obj_fast(e);
		if(o->type == string_type)
		{
		    wprintf(L":aaaaa %ls", anna_string_payload(o));		    
		}
		else
		{
		    wprintf(L": %ls", o->type->name);		    
		}
	    }

	}
	else if(hash_entry_is_dummy(&this->table[i]))
	{
	    wprintf(L"dummy");	    
	}
	else
	{
	    wprintf(L"empty");	    
	}
	
	wprintf(L"\n");
    }
    
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
//    wprintf(L"Weee, resize to %d\nBefore:\n", new_sz);
//    anna_hash_print(this);
    
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
//    wprintf(L"After\n");
//    anna_hash_print(this);
}

static void anna_hash_check_resize(anna_hash_t *this)
{
    size_t old_sz = this->mask+1;
//    if(old_sz <= 64)
//	wprintf(L"%d: %f > %d?\n", old_sz, ANNA_HASH_USED_MIN*old_sz, this->used);
    if(ANNA_HASH_USED_MAX*old_sz < this->used)
    {
	size_t new_sz = ANNA_HASH_SIZE_STEP * old_sz;
	anna_hash_resize(this, new_sz);
    }
    else if( ANNA_HASH_USED_MIN*old_sz > this->used)
    {
	size_t new_sz = old_sz;
	do
	{
	    new_sz /= ANNA_HASH_SIZE_STEP;
	}
	while(ANNA_HASH_USED_MIN*new_sz > this->used && new_sz >= ANNA_HASH_MINSIZE);
	anna_hash_resize(this, new_sz);
    }
}

static void ahi_search_callback2(anna_vmstack_t *stack);
static void ahi_search_callback2_internal(
    anna_vmstack_t *stack, 
    anna_entry_t *key,
    anna_entry_t *hash_obj,
    ahi_callback_t callback,
    anna_entry_t *aux,
    int hash,
    int idx,
    int dummy_idx,
    anna_entry_t *eq);


static inline void ahi_search_callback2_next(
    anna_vmstack_t *stack, 
    anna_entry_t *hash_obj, 
    anna_entry_t *key,
    ahi_callback_t callback,
    anna_entry_t *aux,
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
		    int eq = anna_string_cmp(o, o2)==0;
		    return ahi_search_callback2_internal(
			stack,
			key,
			hash_obj,
			callback,
			aux,
			hash,
			idx,
			dummy_idx,
			eq ? anna_from_int(1):null_entry);
		}
	    }
	    
	}
	
    }
    else
    {
	if(anna_is_int_small(key) && anna_is_int_small(this->table[pos].key))
	{
	    int eq = anna_as_int(key) == anna_as_int(this->table[pos].key);
	    return ahi_search_callback2_internal(
		stack,
		key,
		hash_obj,
		callback,
		aux,
		hash,
		idx,
		dummy_idx,
		eq ? anna_from_int(1):null_entry);	
	}
    }
    
    anna_entry_t *callback_param[] = 
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
    
    anna_entry_t *o_param[] = 
	{
	    key,
	    this->table[pos].key
	}
    ;
    
    anna_object_t *fun_object = anna_as_obj_fast(
	anna_entry_get_static(
	    anna_as_obj(key)->type, ANNA_MID_EQ));

    anna_vm_callback_native(
	stack,
	ahi_search_callback2, 7, callback_param,
	fun_object, 2, o_param
	);    

}

static void ahi_search_callback2_internal(
    anna_vmstack_t *stack, 
    anna_entry_t *key,
    anna_entry_t *hash_obj,
    ahi_callback_t callback,
    anna_entry_t *aux,
    int hash,
    int idx,
    int dummy_idx,
    anna_entry_t *eq)
{
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash_obj));

    if(anna_entry_null(eq))
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
	//wprintf(L"Position is non-empty, and keys are not equal. Check next position: %d (idx %d).\n", pos, idx);
	
	if(hash_entry_is_used(e))
	{
	    //  wprintf(L"Position %d (idx %d) is non-empty, check if keys are equal\n", pos, idx);
	    ahi_search_callback2_next(
		stack, 
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
//	    wprintf(L"Position %d (idx %d) is empty. Yay, object does not exist\n", pos, idx);
	    callback(stack, key, hash, hash_obj, aux, &this->table[(dummy_idx != -1 )? dummy_idx:pos]);
	}
    }
    else
    {
	int pos = (hash+idx) & this->mask;
//	wprintf(L"Position %d is non-empty, but keys are equal. Found match, run callback function!\n", pos);
	callback(stack, key, hash, hash_obj, aux, &this->table[pos]);
    }
}

static void ahi_search_callback2(anna_vmstack_t *stack)
{
    anna_entry_t *eq = anna_vmstack_pop_entry(stack);
    int dummy_idx = anna_vmstack_pop_int(stack);
    int idx = anna_vmstack_pop_int(stack);
    int hash = anna_vmstack_pop_int(stack);
    anna_entry_t *aux = anna_vmstack_pop_entry(stack);
    ahi_callback_t callback = (ahi_callback_t)anna_as_blob(anna_vmstack_pop_entry(stack));
    anna_entry_t *hash_obj = anna_vmstack_pop_entry(stack);
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    ahi_search_callback2_internal(
	stack, key, hash_obj, callback, aux, hash, idx, dummy_idx, eq);
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
    anna_vmstack_t *stack, 
    anna_entry_t *hash_obj, 
    anna_entry_t *key,
    ahi_callback_t callback,
    anna_entry_t *aux,
    int hash)
{
    hash = anna_hash_mangle(hash);
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash_obj));
    int idx = 0;
    int pos = anna_hash_get_next_non_dummy(anna_as_obj(hash_obj), hash);
    anna_hash_entry_t *e = &this->table[pos];
    idx = pos-hash;
    
    //wprintf(L"Calculated hash value %d, maps to position %d, first non-dummy is %d\n", hash, hash & this->mask, pos);
    if(hash_entry_is_used(e))
    {
	int d_pos = hash & this->mask;
	
	int dummy_idx = -1;
	if(hash_entry_is_dummy(&this->table[d_pos]))
	{
	    dummy_idx = d_pos;
	}
	
//	wprintf(L"Position %d (idx %d) is non-empty, check if keys are equal\n", pos, idx);
	ahi_search_callback2_next(
	    stack, 
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
//	wprintf(L"Position %d (idx %d) is empty\n", pos, idx);
	callback(stack, key, hash, hash_obj, aux, &this->table[pos]);
    }
    
}

static void ahi_search_callback(anna_vmstack_t *stack)
{
    int hash = anna_vmstack_pop_int(stack);
    anna_entry_t *aux = anna_vmstack_pop_entry(stack);
    ahi_callback_t callback = (ahi_callback_t)anna_as_blob(anna_vmstack_pop_entry(stack));
    anna_entry_t *hash_obj = anna_vmstack_pop_entry(stack);
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_entry(stack);
    
    ahi_search_callback_internal(
	stack,
	hash_obj,
	key,
	callback,
	aux, hash);
}

static inline void ahi_search(
    anna_vmstack_t *stack,
    anna_entry_t *key,
    anna_entry_t *hash,
    ahi_callback_t callback,
    anna_entry_t *aux)
{
    if(anna_is_obj(key))
    {
	anna_object_t *o = anna_as_obj_fast(key);
	if((o->type == mutable_string_type) ||
	   (o->type == imutable_string_type))
	{
	    ahi_search_callback_internal(
		stack,
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
//	wprintf(L"Search for Int in hash table\n");
	ahi_search_callback_internal(
	    stack,
	    hash,
	    key,
	    callback,
	    aux,
	    anna_as_int(key));
	return;
    }
    
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
    
    anna_vm_callback_native(
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
    obj->flags |= ANNA_OBJECT_HASH;
    return obj;
}

anna_object_t *anna_hash_create2(anna_type_t *hash_type)
{
    anna_object_t *obj= anna_object_create(hash_type);
    ahi_init(ahi_unwrap(obj));    
    obj->flags |= ANNA_OBJECT_HASH;
    return obj;
}

static inline void anna_hash_set_entry(anna_hash_t *this, anna_hash_entry_t *hash_entry, int hash_code, anna_entry_t *key, anna_entry_t *value)
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
/*
static __attribute__((aligned(8))) anna_vmstack_t *anna_hash_init_callback(
    anna_vmstack_t *stack, 
    anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, 
    anna_hash_entry_t *hash_entry);

static inline anna_vmstack_t *anna_hash_init_search_pair(
    anna_vmstack_t *stack, anna_entry_t *this,  
    anna_object_t *list, 
    int start_offset, size_t sz,
    anna_entry_t * data )
{    
    int i;

    for(i=start_offset; i<sz; i++)
    {
	anna_object_t *pair = anna_as_obj_fast(anna_list_get(list, i));
	if(likely(pair != null_object))
	{
	    anna_entry_t *key = anna_pair_get_first(pair);
	    if(!anna_entry_null(key))
	    {
		anna_list_set(anna_as_obj_fast(data), 1, anna_from_int(i));
		return ahi_search(
		    stack,
		    key,
		    this,
		    anna_hash_init_callback,
		    data);
	    }
	}
    }

    return 0;	
}

static __attribute__((aligned(8))) anna_vmstack_t *anna_hash_init_callback(
    anna_vmstack_t *stack, 
    anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *data, 
    anna_hash_entry_t *hash_entry)
{
    anna_object_t *list = anna_as_obj(anna_list_get(anna_as_obj_fast(data), 0));
    int idx = anna_as_int((anna_list_get(anna_as_obj_fast(data), 1)));
    
//    wprintf(L"Init callback\n");
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash));
    
    anna_object_t *pair = anna_as_obj_fast(anna_list_get(list, idx));
    anna_hash_set_entry(this, hash_entry, hash_code, key, anna_pair_get_second(pair));
    
//    wprintf(L"Hash table now has %d used slots and %d dummy slots\n", this->used, this->fill-this->used);    
    
    size_t sz = anna_list_get_count(list);
    if(sz > idx)
    {	
	anna_vmstack_t *new_stack = anna_hash_init_search_pair(
	    stack, hash, list, idx + 1, sz, data);
	if(new_stack)
	{
	    return new_stack;
	}
    }
    
    anna_vmstack_push_entry(stack, hash);
    return stack;
}
*/
static void anna_hash_init(anna_vmstack_t *stack)
{
    anna_object_t *list = anna_vmstack_pop_object(stack);
    anna_object_t *this = anna_vmstack_pop_object(stack);
    anna_vmstack_pop_entry(stack);
    ahi_init(ahi_unwrap(this));
    this->flags |= ANNA_OBJECT_HASH;
    
    if(likely(list != null_object))
    {
	anna_entry_t *argv[] =
	    {
		anna_from_obj(this),
		anna_from_obj(list)
	    }
	;
	anna_entry_t *fun = *anna_entry_get_addr(this, anna_mid_get(L"__setAll__"));
	anna_vm_callback(
	    stack,
	    anna_as_obj(fun), 2, argv);
	return;
/*
	size_t sz = anna_list_get_count(list);
	if(sz > 0)
	{
	    if(ANNA_HASH_USED_MAX * ANNA_HASH_MINSIZE < sz)
	    {		
		size_t new_sz = ANNA_HASH_MINSIZE;
		do
		{
		    new_sz *= ANNA_HASH_SIZE_STEP;
		}
		while(ANNA_HASH_USED_MAX * new_sz < sz);
		anna_hash_resize(ahi_unwrap(this), new_sz);
	    }
	    
	    anna_entry_t * data = anna_from_obj(anna_list_create_mutable(object_type));
	    anna_list_set(anna_as_obj_fast(data), 0, anna_from_obj(list));
	    
	    anna_vmstack_t *new_stack = anna_hash_init_search_pair(
		stack, anna_from_obj(this),
		list, 0, sz, data);
	    
	    if(new_stack)
	    {
		return new_stack;
	    }
	}
*/
    }
    anna_vmstack_push_object(stack, this);
}

static __attribute__((aligned(8))) void anna_hash_set_callback(
    anna_vmstack_t *stack, 
    anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, 
    anna_hash_entry_t *hash_entry)
{
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash));

    anna_hash_set_entry(this, hash_entry, hash_code, key, aux);
    
    anna_vmstack_push_entry(stack, aux);
}

static inline void anna_hash_set(anna_vmstack_t *stack)
{
    anna_entry_t *val = anna_vmstack_pop_entry(stack);
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_entry_t *this = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_entry(stack);

    if(anna_entry_null(key))
    {
	anna_vmstack_push_object(stack, null_object);
	return;
    }
    
    anna_hash_check_resize(ahi_unwrap(anna_as_obj_fast(this)));
    
    ahi_search(
	stack,
	key,
	this,
	anna_hash_set_callback,
	val);
}

static __attribute__((aligned(8))) void anna_hash_get_callback(
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
}

static inline void anna_hash_get(anna_vmstack_t *stack)
{
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_entry_t *this = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_entry(stack);
    
    if(anna_entry_null(key))
    {
	anna_vmstack_push_object(stack, null_object);
    }
    else 
    {
	ahi_search(
	    stack,
	    key,
	    this,
	    anna_hash_get_callback,
	    0);
    }
}



static __attribute__((aligned(8))) void anna_hash_in_callback(
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
}

static inline void anna_hash_in(anna_vmstack_t *stack)
{
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_entry_t *this = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_entry(stack);
    
    if(anna_entry_null(key))
    {
	anna_vmstack_push_object(stack, null_object);
    }
    else
    {
	ahi_search(
	    stack,
	    key,
	    this,
	    anna_hash_in_callback,
	    0);
    }
}

static __attribute__((aligned(8))) void anna_hash_remove_callback(
    anna_vmstack_t *stack, 
    anna_entry_t *key, int hash_code, anna_entry_t *hash, anna_entry_t *aux, 
    anna_hash_entry_t *hash_entry)
{
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(hash));

//    wprintf(L"Returned element %d\n", hash_entry - this->table);    
    
    if(hash_entry_is_used(hash_entry))
    {
	anna_vmstack_push_entry(stack, hash_entry->key);	
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
//	wprintf(L"Failed to remove element %ls with hash code %d (%d), which maps to pos %d\n", anna_string_payload(key), hash_code, anna_hash_mangle(anna_string_hash(key)), hash_code & this->mask);

	anna_vmstack_push_object(stack, null_object);	
    }
}

static inline void anna_hash_remove(anna_vmstack_t *stack)
{
    anna_entry_t *key = anna_vmstack_pop_entry(stack);
    anna_entry_t *this = anna_vmstack_pop_entry(stack);
    anna_vmstack_pop_entry(stack);
//    anna_hash_print(ahi_unwrap(this));
    if(anna_entry_null(key))
    {
	anna_vmstack_push_object(stack, null_object);
	return;
    }
    
    ahi_search(
	stack,
	key,
	this,
	anna_hash_remove_callback,
	0);
}

ANNA_VM_NATIVE(anna_hash_get_count_method, 1)
{
    anna_hash_t *this = ahi_unwrap(anna_as_obj_fast(param[0]));
    return anna_from_int(this->used);
}

/**
   This is the bulk of the each method
 */
static void anna_hash_each_callback(anna_vmstack_t *stack)
{    
    // Discard the output of the previous method call
    anna_vmstack_pop_entry(stack);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_entry_t **param = stack->top - 3;
    // Unwrap and name the params to make things more explicit
    anna_object_t *hash = anna_as_obj(param[0]);
    anna_object_t *body = anna_as_obj(param[1]);
    int idx = anna_as_int(param[2]);
    
    int next_idx = anna_hash_get_next_idx(hash, idx);
    
    if((next_idx >= 0) && !(stack->frame->flags & ANNA_ACTIVATION_FRAME_BREAK))
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
}

static void anna_hash_each(anna_vmstack_t *stack)
{
    anna_object_t *body = anna_vmstack_pop_object(stack);
    anna_object_t *hash = anna_vmstack_pop_object(stack);
    anna_vmstack_pop_entry(stack);
    size_t sz = anna_hash_get_count(hash);

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
	
	anna_vm_callback_native(
	    stack,
	    anna_hash_each_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_vmstack_push_object(stack, hash);
    }
}


static void anna_hash_map_callback(anna_vmstack_t *stack)
{
    anna_entry_t *value = anna_vmstack_pop_entry(stack);
    
    anna_entry_t **param = stack->top - 4;
    anna_object_t *hash = anna_as_obj_fast(param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    anna_object_t *res = anna_as_obj_fast(param[3]);
    
    anna_list_add(res, value);
    int next_idx = anna_hash_get_next_idx(hash, idx);
    
    if((next_idx >= 0) && !(stack->frame->flags & ANNA_ACTIVATION_FRAME_BREAK))
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
}

static void anna_hash_map(anna_vmstack_t *stack)
{
    anna_object_t *body = anna_vmstack_pop_object(stack);
    anna_object_t *hash = anna_vmstack_pop_object(stack);
    anna_vmstack_pop_entry(stack);
    if(body == null_object)
    {
	anna_vmstack_push_object(stack, null_object);
    }
    else
    {
	anna_function_t *fun = anna_function_unwrap(body);
	anna_object_t *res = anna_list_create_mutable(fun->return_type);
	
	size_t sz = anna_hash_get_count(hash);
	
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
	    
	    anna_vm_callback_native(
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
}

ANNA_VM_NATIVE(anna_hash_del, 1)
{
    anna_object_t *hash = anna_as_obj(param[0]);
    anna_hash_t *this = ahi_unwrap(hash);
    if(this->table != this->small_table)
    {
	free(this->table);
    }
    return param[0];
}

static void anna_hash_type_create_internal(
    anna_type_t *type, 
    anna_type_t *spec1,
    anna_type_t *spec2)
{
    anna_member_create_blob(type, ANNA_MID_HASH_PAYLOAD, 0,
                            sizeof(anna_hash_t));
    
    anna_member_create(type, ANNA_MID_HASH_SPECIALIZATION1, 1, null_type);
    anna_member_create(
	type, ANNA_MID_HASH_SPECIALIZATION2, 1, null_type);

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

    anna_member_create_native_method(
	type, anna_mid_get(L"__set__"), 
	0, &anna_hash_set, spec2, 
	3, kv_argv, kv_argn, 0, 0);
    anna_member_document(
	type, anna_mid_get(L"__set__"),
	L"Assigns the specified value to the specified key.");

    anna_member_create_native_method(
	type, anna_mid_get(L"__get__"),
	0, &anna_hash_get,
	spec2, 2, kv_argv, kv_argn, 0, 0);    
    anna_member_document(
	type, anna_mid_get(L"__get__"),
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
	type, anna_mid_get(L"__init__"),
	ANNA_FUNCTION_VARIADIC, &anna_hash_init,
	type, 2, i_argv, i_argn, 0, 0);

    anna_member_create_native_property(
	type, anna_mid_get(L"count"), int_type,
	&anna_hash_get_count_method, 0,
	L"The number of elements in this Map.");

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
    
    anna_member_create_native_method(
	type, anna_mid_get(L"__each__"), 0,
	&anna_hash_each, type, 2, e_argv, e_argn, 0, 0);

    anna_member_create_native_method(
	type,
	anna_mid_get(L"__map__"),
	0,
	&anna_hash_map,
	mutable_list_type,
	2,
	e_argv,
	e_argn, 0, 0);
    
    anna_member_create_native_method(
	type, ANNA_MID_DEL, 0, &anna_hash_del,
	object_type, 1, e_argv, e_argn, 0, 0);
    
    anna_member_create_native_method(
	type, anna_mid_get(L"__in__"), 0,
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
	L"Remove the specified key from the hash");
    
    anna_hash_add_all_extra_methods(type);
    
}

static inline void anna_hash_internal_init()
{
    static int init = 0;
    if(likely(init))
	return;
    init=1;
    hash_init(&anna_hash_specialization, hash_tt_func, hash_tt_cmp);
}

void anna_hash_type_create()
{
    anna_hash_internal_init();
    hash_put(&anna_hash_specialization, anna_tt_make(object_type, object_type), hash_type);
    anna_hash_type_create_internal(hash_type, object_type, object_type);
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
	spec = anna_type_native_create(sb_content(&sb), 0);
	sb_destroy(&sb);
	hash_put(&anna_hash_specialization, anna_tt_make(subtype1, subtype2), spec);
	anna_hash_type_create_internal(spec, subtype1, subtype2);
	spec->flags |= ANNA_TYPE_SPECIALIZED;
	anna_type_copy_object(spec);
    }
    return spec;
}

void anna_hash_mark(anna_object_t *obj)
{
    anna_hash_t *this = ahi_unwrap(obj);
//    wprintf(L"HASHMARK %ls %d %d %d %d\n", obj->type->name, obj, null_entry, this->mask, this->table);
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

