
static hash_table_t anna_pair_specialization;
static array_list_t anna_pair_additional_methods = AL_STATIC;

static void add_pair_method(void *key, void *value, void *aux)
{
    anna_type_t *pair = (anna_type_t *)value;
    anna_function_t *fun = (anna_function_t *)aux;
    anna_member_create_method(pair, anna_mid_get(fun->name), fun);
}

void anna_pair_add_method(anna_function_t *fun)
{
    al_push(&anna_pair_additional_methods, fun);
    hash_foreach2(&anna_pair_specialization, &add_pair_method, fun);
}

static void anna_pair_add_all_extra_methods(anna_type_t *pair)
{
    int i;
    for(i=0; i<al_get_count(&anna_pair_additional_methods); i++)
    {
	anna_function_t *fun = (anna_function_t *)al_get(&anna_pair_additional_methods, i);
//	anna_message(L"Add function %ls to type %ls\n", fun->name, pair->name);
	anna_member_create_method(pair, anna_mid_get(fun->name), fun);
    }
}


anna_object_t *anna_pair_create(anna_entry_t *first, anna_entry_t *second)
{
    anna_object_t *obj= anna_object_create(anna_pair_type_get(anna_as_obj(first)->type, anna_as_obj(second)->type));
    anna_pair_set_first(obj, first);
    anna_pair_set_second(obj, second);
    return obj;
}

ANNA_VM_NATIVE(anna_pair_init, 3)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_pair_set_first(this, param[1]);
    anna_pair_set_second(this, param[2]);
    return param[0];
}

ANNA_VM_NATIVE(anna_pair_get_first_i, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    return anna_pair_get_first(this);
}

ANNA_VM_NATIVE(anna_pair_get_second_i, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    return anna_pair_get_second(this);
}

ANNA_VM_NATIVE(anna_pair_set_first_i, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_pair_set_first(this, param[1]);
    return param[1];
}

ANNA_VM_NATIVE(anna_pair_set_second_i, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_pair_set_second(this, param[1]);
    return param[1];
}
/*
static anna_type_t *anna_pair_get_specialization1(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_entry_get_addr(
		 obj,
		 ANNA_MID_PAIR_SPECIALIZATION1));    
}

static anna_type_t *anna_pair_get_specialization2(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_entry_get_addr(
		 obj,
		 ANNA_MID_PAIR_SPECIALIZATION2));    
}
*/
static void anna_pair_type_create_internal(
    anna_type_t *type, 
    anna_type_t *spec1,
    anna_type_t *spec2)
{
    anna_member_create(
	type, ANNA_MID_PAIR_FIRST, 0, spec1);
    anna_member_create(
	type, ANNA_MID_PAIR_SECOND, 0, spec2);
    anna_member_create(
	type,
	ANNA_MID_PAIR_SPECIALIZATION1,
	1,
	null_type);
    anna_member_create(
	type,
	ANNA_MID_PAIR_SPECIALIZATION2,
	1,
	null_type);
    
    (*(anna_type_t **)anna_entry_get_addr_static(type,ANNA_MID_PAIR_SPECIALIZATION1)) = spec1;
    (*(anna_type_t **)anna_entry_get_addr_static(type,ANNA_MID_PAIR_SPECIALIZATION2)) = spec2;
    
    anna_type_t *argv[] = 
	{
	    type,
	    spec1,
	    spec2
	}
    ;
    
    wchar_t *argn[]=
	{
	    L"this", L"second", L"first"
	}
    ;

    anna_member_create_native_method(
	type, anna_mid_get(L"__init__"), 0,
	&anna_pair_init, type, 3, argv, argn, 0, 0);
    
    anna_member_create_native_property(
	type, anna_mid_get(L"first"), spec1,
	&anna_pair_get_first_i,
	&anna_pair_set_first_i, L"The first value of the pair.");
    anna_member_create_native_property(
	type,
	anna_mid_get(L"second"),
	spec2,
	&anna_pair_get_second_i,
	&anna_pair_set_second_i, L"The second value of the pair.");
    anna_pair_add_all_extra_methods(type);

    anna_type_document(
	type,
	L"A Pair represents a mapping from a key to a value.");


    anna_type_close(type);    
}

static inline void anna_pair_internal_init()
{
    static int init = 0;
    if(likely(init))
	return;
    init=1;
    hash_init(&anna_pair_specialization, hash_tt_func, hash_tt_cmp);
}

void anna_pair_type_create()
{
    anna_pair_internal_init();
    hash_put(&anna_pair_specialization, anna_tt_make(object_type, object_type), pair_type);
    anna_pair_type_create_internal(pair_type, object_type, object_type);
}

anna_type_t *anna_pair_type_get(anna_type_t *subtype1, anna_type_t *subtype2)
{
    anna_pair_internal_init();
    anna_tt_t tt = 
	{
	    subtype1, subtype2
	}
    ;
    
    anna_type_t *spec = hash_get(&anna_pair_specialization, &tt);
    if(!spec)
    {
	string_buffer_t sb;
	sb_init(&sb);
	sb_printf(&sb, L"Pair«%ls,%ls»", subtype1->name, subtype2->name);
	spec = anna_type_create(sb_content(&sb), 0);
	sb_destroy(&sb);
	hash_put(&anna_pair_specialization, anna_tt_make(subtype1, subtype2), spec);
	anna_pair_type_create_internal(spec, subtype1, subtype2);
	spec->flags |= ANNA_TYPE_SPECIALIZED;
	anna_type_copy_object(spec);
    }
    return spec;
}
