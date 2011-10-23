
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

ANNA_VM_NATIVE(anna_function_type_i_get_filename, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_function_t *f = anna_function_unwrap(this);
    return f->filename?anna_from_obj(anna_string_create(wcslen(f->filename), f->filename)):null_entry;
}

ANNA_VM_NATIVE(anna_continuation_type_i_get_filename, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_activation_frame_t *frame = (anna_activation_frame_t *)*anna_entry_get_addr(this, ANNA_MID_CONTINUATION_ACTIVATION_FRAME);
    anna_function_t *f = frame->function;
    return f->filename?anna_from_obj(anna_string_create(wcslen(f->filename), f->filename)):null_entry;
}

ANNA_VM_NATIVE(anna_continuation_type_i_get_line, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_activation_frame_t *frame = (anna_activation_frame_t *)*anna_entry_get_addr(this, ANNA_MID_CONTINUATION_ACTIVATION_FRAME);
    int line = anna_function_line(frame->function, frame->code - frame->function->code);
    return line >= 0 ? anna_from_int(line): null_entry;
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
    anna_entry_t *res = anna_from_obj( anna_string_create(sb_length(&sb), sb_content(&sb)));
    sb_destroy(&sb);
    return res;
}

static void anna_function_type_trace_recursive(
    string_buffer_t *sb, anna_activation_frame_t *frame)
{
    if(!frame)
    {
	return;
    }
    anna_function_type_trace_recursive(sb, frame->dynamic_frame);
    if(frame->function->name)
    {
	sb_printf(
	    sb, L"%ls: %ls:%d\n", 
	    frame->function->name, frame->function->filename,
	    anna_function_line(frame->function, frame->code - frame->function->code));
    }
}

ANNA_VM_NATIVE(anna_function_type_trace, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    string_buffer_t sb;
    sb_init(&sb);
    anna_activation_frame_t *frame = (anna_activation_frame_t *)*anna_entry_get_addr(this, ANNA_MID_CONTINUATION_ACTIVATION_FRAME);
    anna_function_type_trace_recursive(&sb, frame);
    anna_entry_t *res = anna_from_obj( anna_string_create(sb_length(&sb), sb_content(&sb)));
    sb_destroy(&sb);
    return res;
}

ANNA_VM_NATIVE(anna_function_type_i_call_count, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    if(null_entry == anna_entry_get(this, ANNA_MID_CONTINUATION_CALL_COUNT))
    {
	return anna_from_int(0);
    }
        
    return anna_entry_get(this, ANNA_MID_CONTINUATION_CALL_COUNT);
}

ANNA_VM_NATIVE(anna_function_type_i_get, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    if(param[1] == null_entry)
    {
	return null_entry;
    }
    
    wchar_t *name = anna_string_payload(anna_as_obj(param[1]));
    anna_activation_frame_t *frame = (anna_activation_frame_t *)*anna_entry_get_addr(this, ANNA_MID_CONTINUATION_ACTIVATION_FRAME);
    anna_entry_t *res = null_entry;
    anna_function_t *fun = frame->function;
    anna_sid_t sid = anna_stack_sid_create(fun->stack_template, name);
    if(sid.frame == 0)
    {
	res = frame->slot[sid.offset];
    }
    
    free(name);
    return res;
}

ANNA_VM_NATIVE(anna_function_type_i_dynamic, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_activation_frame_t *frame = (anna_activation_frame_t *)*anna_entry_get_addr(this, ANNA_MID_CONTINUATION_ACTIVATION_FRAME);
    if(frame->dynamic_frame)
    {
	anna_object_t *cont = anna_continuation_create(
	    (anna_entry_t **)*anna_entry_get_addr(this, ANNA_MID_CONTINUATION_STACK),
	    (size_t)*anna_entry_get_addr(this, ANNA_MID_CONTINUATION_STACK_COUNT),
	    frame->dynamic_frame, 0)->wrapper;
	return anna_from_obj(cont);
    }
    return null_entry;
}

ANNA_VM_NATIVE(anna_function_type_i_static, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_context_t *c_stack = (anna_context_t *)*anna_entry_get_addr(this, ANNA_MID_CONTINUATION_STACK);
    anna_activation_frame_t *frame = c_stack->frame;
    if(frame->static_frame)
    {
	anna_object_t *cont = anna_continuation_create(
	    (anna_entry_t **)*anna_entry_get_addr(this, ANNA_MID_CONTINUATION_STACK),
	    (size_t)*anna_entry_get_addr(this, ANNA_MID_CONTINUATION_STACK_COUNT),
	    frame->static_frame, 0)->wrapper;
	return anna_from_obj(cont);
    }
    return null_entry;
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
	string_type, 1, argv, argn, 0, 0);

    anna_member_create_native_property(
	res, ANNA_MID_NAME, string_type,
	&anna_function_type_i_get_name, 0,
	L"The name of this function.");

    anna_member_create_native_property(
	res, ANNA_MID_OUTPUT_TYPE,
	type_type,
	&anna_function_type_i_get_output,
	0,
	L"The return type of this function.");

    anna_member_create_native_property(
	res, ANNA_MID_INPUT_TYPE,
	anna_list_type_get_imutable(type_type),
	&anna_function_type_i_get_input_type,
	0,
	L"A list of the input types of this function.");
    
    anna_member_create_native_property(
	res, ANNA_MID_INPUT_NAME,
	anna_list_type_get_imutable(string_type),
	&anna_function_type_i_get_input_name,
	0,
	L"A list of the input names of this function.");

    anna_member_create_native_property(
	res, ANNA_MID_DEFAULT_VALUE,
	anna_list_type_get_imutable(node_type),
	&anna_function_type_i_get_default_value,
	0,
	L"A list of the default values for the input parameters of this function. Null means no default value exists.");

    anna_member_create_native_property(
	res, ANNA_MID_ATTRIBUTE,
	node_call_type,
	&anna_function_type_i_get_attributes,
	0,
	L"All attributes specified for this function.");

    anna_member_create_native_property(
	res, ANNA_MID_FILENAME,
	imutable_string_type,
	&anna_function_type_i_get_filename,
	0,
	L"The name of the file in which this function was defined.");

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
    
    anna_member_create(
	res, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD,
	ANNA_MEMBER_ALLOC, null_type);

    anna_member_create(
	res,
	ANNA_MID_FUNCTION_WRAPPER_STACK,
	ANNA_MEMBER_ALLOC,
	null_type);
    
    anna_member_create(
	res, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD,
	ANNA_MEMBER_STATIC, null_type);

    if(key->flags & ANNA_FUNCTION_CONTINUATION)
    {
	anna_member_create(
	    res, ANNA_MID_CONTINUATION_STACK,
	    ANNA_MEMBER_ALLOC, null_type);
	
	anna_member_create(
	    res, ANNA_MID_CONTINUATION_STACK_COUNT,
	    0, null_type);
	
	anna_member_create(
	    res, ANNA_MID_CONTINUATION_ACTIVATION_FRAME,
	    ANNA_MEMBER_ALLOC, null_type);
	
	anna_member_create(
	    res, ANNA_MID_CONTINUATION_CODE_POS,
	    0, null_type);
	
	anna_member_create(
	    res, ANNA_MID_CONTINUATION_CALL_COUNT,
	    0, null_type);
	
	anna_member_create_native_property(
	    res, anna_mid_get(L"callCount"),
	    int_type,
	    &anna_function_type_i_call_count,
	    0,
	    L"The number of times this continuation has been called.");

	anna_type_t *v_argv[] = 
	    {
		res,
		string_type,
		object_type
	    }
	;
    
	wchar_t *v_argn[]=
	    {
		L"this", L"key", L"value"
	    }
	;

	anna_member_create_native_property(
	    res, anna_mid_get(L"trace"), 
	    string_type,
	    &anna_function_type_trace,
	    0, 0);

	anna_member_create_native_method(
	    res, anna_mid_get(L"__get__"), 0,
	    &anna_function_type_i_get, object_type, 2, v_argv, v_argn, 0, 
	    L"Returns the value of the local variable with the specified name.");
	
	anna_member_create_native_property(
	    res, anna_mid_get(L"dynamicFrame"),
	    res,
	    &anna_function_type_i_dynamic,
	    0,
	    L"The continuation of the dynamic scope (the caller) of this continuation.");

	anna_member_create_native_property(
	    res, anna_mid_get(L"staticFrame"),
	    res,
	    &anna_function_type_i_static,
	    0,
	    L"The continuation of the static scope of this continuation.");
	
	anna_member_create_native_property(
	    res, anna_mid_get(L"filename"),
	    imutable_string_type,
	    &anna_continuation_type_i_get_filename,
	    0,
	    L"The name of the file in which the function that this continuation points into was defined in.");

	anna_member_create_native_property(
	    res, anna_mid_get(L"line"),
	    int_type,
	    &anna_continuation_type_i_get_line,
	    0,
	    L"The line number of the code offset of the function that this continuation points into.");

    }
    
    if(key->flags & ANNA_FUNCTION_BOUND_METHOD)
    {
	anna_member_create(res, ANNA_MID_THIS, ANNA_MEMBER_ALLOC, null_type);
	anna_member_create(res, ANNA_MID_METHOD, ANNA_MEMBER_ALLOC, null_type);
    }
    
    *anna_entry_get_addr_static(res, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD) = (anna_entry_t *)key;
    anna_type_close(res);
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
