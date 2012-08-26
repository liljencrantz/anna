/*
static anna_type_t *anna_continuation_variable_of(anna_object_t *wrapper)
{
    return *(anna_type_t **)anna_entry_get_addr(wrapper, ANNA_MID_CONTINUATION_VARIABLE_TYPE_PAYLOAD);
}

ANNA_VM_NATIVE(anna_continuation_variable_i_get_name, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    int offset = anna_as_int(
	anna_entry_get(this, ANNA_MID_OFFSET));
    anna_function_t *continuation = anna_function_unwrap(
	anna_entry_get(this, ANNA_MID_CONTINUATION_VARIABLE_CONTINUATION));
    return anna_from_obj( anna_string_create(wcslen(m->name), m->name));
}

ANNA_VM_NATIVE(anna_continuation_variable_i_get_type, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_continuation_variable_t *m = anna_continuation_variable_unwrap(this);
    return anna_from_obj(anna_type_wrap(m->type));
}
*/

typedef struct
{
    int offset;
    wchar_t *name;
}
hash_search_data_t;

static void anna_hash_search_offset(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    hash_search_data_t *data = (hash_search_data_t *)aux_ptr;

    if( *(int*)val_ptr == data->offset)
    {
	data->name = key_ptr;
    }
}

static wchar_t *anna_continuation_name_from_offset(anna_function_t *fun, int offset)
{
    hash_search_data_t data = 
	{
	    offset, 0 
	}
    ;    
    
    hash_foreach2(&fun->stack_template->member_string_identifier, &anna_hash_search_offset, &data);
    return data.name;
}

ANNA_VM_NATIVE(anna_continuation_variable_i_get_name, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);

    int offset = anna_as_int(
	anna_entry_get(
	    this, ANNA_MID_OFFSET));

    anna_object_t *cont = anna_as_obj(
	anna_entry_get(
	    this, ANNA_MID_CONTINUATION_VARIABLE_CONTINUATION));

    anna_activation_frame_t *frame = 
	(anna_activation_frame_t *)anna_entry_get_obj(
	    cont, ANNA_MID_CONTINUATION_ACTIVATION_FRAME);
    anna_function_t *fun = frame->function;
    wchar_t *name = anna_continuation_name_from_offset(fun, offset);
    
    if(!name)
    {
	return null_entry;
    }
    
    return anna_from_obj( anna_string_create(wcslen(name), name));
}

ANNA_VM_NATIVE(anna_continuation_variable_i_get_type, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);

    int offset = anna_as_int(
	anna_entry_get(
	    this, ANNA_MID_OFFSET));

    anna_object_t *cont = anna_as_obj(
	anna_entry_get(
	    this, ANNA_MID_CONTINUATION_VARIABLE_CONTINUATION));

    anna_activation_frame_t *frame = 
	(anna_activation_frame_t *)anna_entry_get_obj(
	    cont, ANNA_MID_CONTINUATION_ACTIVATION_FRAME);
    anna_function_t *fun = frame->function;
    wchar_t *name = anna_continuation_name_from_offset(fun, offset);
    
    if(!name)
    {
	return null_entry;
    }
    
    return anna_from_obj( 
	anna_type_wrap(
	    anna_stack_get_type(
		fun->stack_template, name)));
}

ANNA_VM_NATIVE(anna_continuation_variable_i_get_value, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);

    int offset = anna_as_int(
	anna_entry_get(
	    this, ANNA_MID_OFFSET));
    
    anna_object_t *cont = anna_as_obj(
	anna_entry_get(
	    this, ANNA_MID_CONTINUATION_VARIABLE_CONTINUATION));
    
    anna_activation_frame_t *frame = (anna_activation_frame_t *)anna_entry_get_obj(cont, ANNA_MID_CONTINUATION_ACTIVATION_FRAME);
    return  frame->slot[offset];
}

static void anna_continuation_variable_load(anna_stack_template_t *stack)
{
    anna_member_create(
	continuation_variable_type, 
	ANNA_MID_OFFSET, 
	ANNA_MEMBER_INTERNAL, int_type);
    anna_member_document(
	continuation_variable_type, 
	ANNA_MID_OFFSET, 
	L"The offset in the activation frame of this variable.");
    
    anna_member_create(
	continuation_variable_type,
	ANNA_MID_CONTINUATION_VARIABLE_CONTINUATION,
	0,
	continuation_type);
    anna_member_document(
	continuation_variable_type, 
	ANNA_MID_CONTINUATION_VARIABLE_CONTINUATION, 
	L"The continuation that this ContinuationVariable is a variable in.");

    anna_member_create_native_property(
	continuation_variable_type, anna_mid_get(L"value"),
	object_type, &anna_continuation_variable_i_get_value, 0,
	L"The value of this variable.");

    anna_member_create_native_property(
	continuation_variable_type, anna_mid_get(L"name"),
	string_type, &anna_continuation_variable_i_get_name, 0,
	L"The name of this variable.");

    anna_member_create_native_property(
	continuation_variable_type, anna_mid_get(L"type"),
	type_type, &anna_continuation_variable_i_get_type, 0,
	L"The name of this variable.");

    anna_type_t *v_argv[] = 
	{
	    continuation_variable_type,
	}
    ;

    wchar_t *v_argn[] =
	{
	    L"this"
	}
    ;
    
    anna_member_create_native_method(
	continuation_variable_type,
	ANNA_MID_INIT,
	0,
	&anna_vm_null_function,
	continuation_variable_type,
	1,
	&continuation_variable_type,
	v_argn, 0, 
	0);
    
    anna_member_create_native_method(
	continuation_variable_type,
	ANNA_MID_TO_STRING, 
	0,
	&anna_continuation_variable_i_get_name, 
	string_type, 
	1, 
	v_argv, v_argn, 0,
	L"Returns a String representation of this member.");

    anna_type_document(
	continuation_variable_type,
	L"The ContinuationVariable type represents a live variable inside a Continuation. It is useful for introspecting the current value of a variable while the function it is defined in is being executed.");
}
