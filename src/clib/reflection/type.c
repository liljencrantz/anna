
ANNA_VM_NATIVE(anna_type_to_string, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *type = anna_type_unwrap(this);
    return anna_from_obj(anna_string_create(wcslen(type->name), type->name));
}

ANNA_VM_NATIVE(anna_type_is_module, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *type = anna_type_unwrap(this);
    return anna_entry_get_addr_static(type, ANNA_MID_STACK_TYPE_PAYLOAD) ? anna_from_int(1) : null_entry;
}

ANNA_VM_NATIVE(anna_type_i_get_member, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *lst = anna_list_create_imutable(member_type);
    int i;
    anna_type_t *type = anna_type_unwrap(this);
    if(type == null_type)
    {
	return anna_from_obj(lst);
    }
    
    wchar_t **member_name = malloc(sizeof(wchar_t *)*hash_get_count(&type->name_identifier));
    anna_type_get_member_names(type, member_name);
    for(i=0;i<hash_get_count(&type->name_identifier); i++)
    {
	anna_object_t *memb_obj = anna_member_wrap(
		type,
		anna_type_member_info_get(
		    type,
		    member_name[i]));	
	
	anna_list_add(
	    lst,
	    anna_from_obj(memb_obj));
	
    }
    
    return anna_from_obj(lst);
}

static anna_vmstack_t *anna_type_cmp(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *type1 = anna_type_unwrap(this);
    anna_type_t *type2 = anna_type_unwrap(anna_as_obj(param[1]));
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_object(stack, anna_int_create(type1-type2));
    return stack;
}

static anna_vmstack_t *anna_type_abides(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *type1 = anna_type_unwrap(this);
    anna_type_t *type2 = anna_type_unwrap(anna_as_obj(param[1]));
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_entry(stack, anna_abides(type1, type2)?anna_from_int(1):null_entry);
    return stack;
}

static anna_vmstack_t *anna_type_hash(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_vmstack_drop(stack, 2);
    anna_type_t *type = anna_type_unwrap(this);
    anna_vmstack_push_object(stack, anna_int_create(hash_ptr_func(type)));
    return stack;
}

ANNA_VM_NATIVE(anna_type_i_get_attribute, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *f = anna_type_unwrap(this);
    return f->attribute ? anna_from_obj(anna_node_wrap((anna_node_t *)f->attribute)): null_entry;
}

static void anna_type_load()
{
    anna_member_create(type_type, ANNA_MID_TYPE_WRAPPER_PAYLOAD, 0, null_type);

    anna_type_t *argv[] = 
	{
	    type_type, type_type
	}
    ;
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;

    anna_type_document(
	type_type,
	L"The Type type represents an Anna Type. It is mostly used for object introspection.");
    
    anna_member_create_native_property(
	type_type, anna_mid_get(L"name"),
	string_type, &anna_type_to_string, 0, L"The name of this type.");
    anna_member_create_native_property(
	type_type, anna_mid_get(L"isModule"),
	int_type, &anna_type_is_module, 0, L"Is true if this type represents a module.");
    anna_member_create_native_property(
	type_type,
	anna_mid_get(L"member"),
	anna_list_type_get_imutable(member_type),
	&anna_type_i_get_member,
	0, L"A list of all members of this type.");
    
    anna_member_create_native_method(
	type_type, ANNA_MID_HASH_CODE, 0,
	&anna_type_hash, int_type, 1, argv, argn, 0, 0);
    
    anna_member_create_native_method(
	type_type,
	ANNA_MID_CMP,
	0,
	&anna_type_cmp,
	int_type,
	2,
	argv,
	argn, 0, 0);
    anna_member_create_native_method(
	type_type,
	ANNA_MID_TO_STRING,
	0,
	&anna_type_to_string,
	string_type,
	1,
	argv,
	argn, 0, 0);
    
    anna_member_create_native_method(
	type_type, anna_mid_get(L"abides"), 0,
	&anna_type_abides, object_type, 2, argv,
	argn, 0, 0);

    anna_member_create_native_property(
	type_type, anna_mid_get(L"attribute"),
	node_call_type,
	&anna_type_i_get_attribute,
	0,
	L"All attributes specified for this type.");


}
