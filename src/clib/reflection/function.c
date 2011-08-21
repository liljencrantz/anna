
anna_type_t *function_type_base = 0;
static int base_constructed = 0;
static array_list_t types=AL_STATIC;

ANNA_VM_NATIVE(anna_function_type_i_get_name, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_function_t *f = anna_function_unwrap(this);
    return anna_from_obj( anna_string_create(wcslen(f->name), f->name));
}

ANNA_VM_NATIVE(anna_function_type_i_get_output, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_function_t *f = anna_function_unwrap(this);
    return anna_from_obj( anna_type_wrap(f->return_type));
}

ANNA_VM_NATIVE(anna_function_type_i_get_input_type, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *lst = anna_list_create_imutable(type_type);
    int i;
    anna_function_t *f = anna_function_unwrap(this);

    for(i=0;i<f->input_count; i++)
    {
	anna_list_add(
	    lst,
	    anna_from_obj(
		anna_type_wrap(
		    f->input_type[i])));
    }
    
    return anna_from_obj( lst);
}

ANNA_VM_NATIVE(anna_function_type_i_get_input_name, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *lst = anna_list_create_imutable(string_type);
    int i;
    anna_function_t *f = anna_function_unwrap(this);

    for(i=0;i<f->input_count; i++)
    {
	anna_list_add(
	    lst,
	    anna_from_obj(
		anna_string_create(
		    wcslen(f->input_name[i]),
		    f->input_name[i])));
    }
    
    return anna_from_obj( lst);
}

ANNA_VM_NATIVE(anna_function_type_i_get_default_value, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *lst = anna_list_create_imutable(string_type);
    int i;
    anna_function_t *f = anna_function_unwrap(this);

    for(i=0;i<f->input_count; i++)
    {
	anna_list_add(
	    lst,
	    f->input_default[i] ? 
	    anna_from_obj(
		anna_node_wrap(f->input_default[i])) :
	    null_entry);
	
    }
    
    return anna_from_obj( lst);
}

ANNA_VM_NATIVE(anna_function_type_i_get_attributes, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_function_t *f = anna_function_unwrap(this);
    
    return anna_from_obj(anna_node_wrap((anna_node_t *)f->attribute));
}

void anna_function_type_print(anna_function_type_t *k)
{
    wprintf(L"%ls <- (", k->return_type?k->return_type->name:L"<null>");
    int i;
    for(i=0;i<k->input_count; i++)
    {
	if(i!=0)
	    wprintf(L", ");
	wprintf(
	    L"%ls %ls",
	    (k->input_type && k->input_type[i])?k->input_type[i]->name:L"<null>",
	    (k->input_name && k->input_name[i])?k->input_name[i]:L"");
    }
    wprintf(L")\n");
}

ANNA_VM_NATIVE(anna_function_type_to_string, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    string_buffer_t sb;
    sb_init(&sb);
    anna_function_t *fun = anna_function_unwrap(this);
    sb_printf(&sb, L"def %ls %ls (", fun->return_type->name, fun->name);
    int i;
    for(i=0; i<fun->input_count; i++)
    {
	sb_printf(&sb, L"%ls%ls %ls", i>0?L", ":L"", fun->input_type[i]->name, fun->input_name[i]);
    }
    sb_printf(&sb, L")");
    return anna_from_obj( anna_string_create(sb_length(&sb), sb_content(&sb)));
}

static void anna_function_load(anna_stack_template_t *stack)
{
    anna_type_t *res = function_type_base;
    
    anna_type_t *argv[] = 
	{
	    res
	}
    ;
    wchar_t *argn[]=
	{
	    L"this"
	}
    ;
    
    anna_member_create_native_method(
	res, ANNA_MID_TO_STRING, 0,
	&anna_function_type_to_string,
	string_type, 1, argv, argn);

    anna_member_create_native_property(
	res, anna_mid_get(L"name"), string_type,
	&anna_function_type_i_get_name, 0,
	L"The name of this function.");

    anna_member_create_native_property(
	res, anna_mid_get(L"outputType"),
	type_type,
	&anna_function_type_i_get_output,
	0,
	L"The return type of this function.");

    anna_member_create_native_property(
	res, anna_mid_get(L"inputType"),
	anna_list_type_get_imutable(type_type),
	&anna_function_type_i_get_input_type,
	0,
	L"A list of the input types of this function.");
    
    anna_member_create_native_property(
	res, anna_mid_get(L"inputName"),
	anna_list_type_get_imutable(string_type),
	&anna_function_type_i_get_input_name,
	0,
	L"A list of the input names of this function.");

    anna_member_create_native_property(
	res, anna_mid_get(L"defaultValue"),
	anna_list_type_get_imutable(node_type),
	&anna_function_type_i_get_default_value,
	0,
	L"A list of the default values for the input parameters of this function. Null means no default value exists.");

    anna_member_create_native_property(
	res, anna_mid_get(L"attribute"),
	node_call_type,
	&anna_function_type_i_get_attributes,
	0,
	L"All attributes specified for this function.");

    anna_type_copy_object(res);

    int i;
    for(i=0; i<al_get_count(&types); i++)
    {
	anna_type_t *child = al_get(&types, i);
	anna_type_copy(child, function_type_base);
    }
    al_destroy(&types);
    base_constructed = 1;

}

void anna_function_type_create(
    anna_function_type_t *key, 
    anna_type_t *res)
{
    //anna_function_type_print(key);
    
    if(base_constructed)
	anna_type_copy(res, function_type_base);
    else
	al_push(&types, res);
    
    anna_type_copy_object(res);
    
    /*
      Non-static member variables
    */
    anna_member_create(
	res, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD,
	ANNA_MEMBER_ALLOC, null_type);
    anna_member_create(
	res,
	ANNA_MID_FUNCTION_WRAPPER_STACK,
	ANNA_MEMBER_ALLOC,
	null_type);
    
    /*
      Static member variables
    */
    anna_member_create(
	res, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD,
	ANNA_MEMBER_STATIC, null_type);
    
    if(key->flags & ANNA_FUNCTION_CONTINUATION)
    {
	anna_member_create(
	    res, ANNA_MID_CONTINUATION_STACK,
	    ANNA_MEMBER_ALLOC, null_type);
	
	anna_member_create(
	    res, ANNA_MID_CONTINUATION_CODE_POS,
	    0, null_type);
    }
    
    if(key->flags & ANNA_FUNCTION_BOUND_METHOD)
    {
	anna_member_create(res, ANNA_MID_THIS, ANNA_MEMBER_ALLOC, null_type);anna_member_create(res,
                                                                                                ANNA_MID_METHOD,
                                                                                                ANNA_MEMBER_ALLOC,
                                                                                                null_type);
    }
    
    *anna_entry_get_addr_static(res, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD) = (anna_entry_t *)key;

    return;
}

__pure anna_function_type_t *anna_function_type_unwrap(anna_type_t *type)
{
    if(!type)
    {
	wprintf(L"Critical: Tried to get function from non-existing type\n");
	CRASH;
    }
    
//    wprintf(L"Find function signature for call %ls\n", type->name);
    
    anna_function_type_t **function_ptr = 
	(anna_function_type_t **)anna_entry_get_addr_static(
	    type,
	    ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    if(function_ptr) 
    {
//	wprintf(L"Got member, has return type %ls\n", (*function_ptr)->return_type->name);
	return *function_ptr;
    }
    return 0;	
    FIXME("Missing validity checking")
}

anna_type_t *anna_function_type_each_create(
    wchar_t *name, 
    anna_type_t *key_type,
    anna_type_t *value_type)
{
    anna_function_type_t *each_key = malloc(sizeof(anna_function_type_t) + 2*sizeof(anna_type_t *));
    each_key->return_type = object_type;
    each_key->input_count = 2;
    each_key->flags = 0;

    each_key->input_name = malloc(sizeof(wchar_t *)*2);
    each_key->input_name[0] = L"key";
    each_key->input_name[1] = L"value";

    each_key->input_type[0] = key_type;
    each_key->input_type[1] = value_type;    
    anna_type_t *fun_type = anna_type_native_create(name, stack_global);
    anna_function_type_create(each_key, fun_type);
    return fun_type;
}
