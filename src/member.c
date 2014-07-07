//ROOT: src/type.c

anna_object_t *anna_member_wrap(anna_type_t *type, anna_member_t *result)
{
    if(result->wrapper)
	return result->wrapper;
    
    anna_type_t * m_type;
    if(anna_member_is_bound(result))
    {
	m_type = member_method_type;
    }
    else if(anna_member_is_property(result))
    {
	m_type = member_property_type;
    }
    else
    {
	m_type=member_variable_type;
    }
    
    result->wrapper = anna_object_create(m_type);
    memcpy(
	anna_entry_get_addr(
	    result->wrapper, ANNA_MID_MEMBER_PAYLOAD),
	&result, 
	sizeof(anna_member_t *));  
    memcpy(
	anna_entry_get_addr(
	    result->wrapper, ANNA_MID_MEMBER_TYPE_PAYLOAD), 
	&type, 
	sizeof(anna_type_t *));  
    assert(result->wrapper);
    return result->wrapper;
}

anna_member_t *anna_member_unwrap(anna_object_t *wrapper)
{
    return *(anna_member_t **)anna_entry_get_addr(wrapper, ANNA_MID_MEMBER_PAYLOAD);
}

void anna_member_type_set(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *member_type)
{
    anna_member_t *memb = type->mid_identifier[mid];
    assert(memb);
    memb->type = member_type;
}

mid_t anna_member_create(
    anna_type_t *type,
    mid_t mid,
    int storage,
    anna_type_t *member_type)
{
    wchar_t *name = anna_mid_get_reverse(mid);
    
    if((type->flags & ANNA_TYPE_CLOSED) && !(storage & ANNA_MEMBER_STATIC) && !(storage & ANNA_MEMBER_PROPERTY))
    {
	debug(D_CRITICAL, L"Added additional non-static member %ls after closing type %ls\n", anna_mid_get_reverse(mid), type->name);
	CRASH;
    }    

    if(anna_member_get(type, mid))
    {
	if(type == type_type && wcscmp(name, L"!typeWrapperPayload")==0)
	    return mid;
	if(
	    mid == ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD ||
	    mid == ANNA_MID_FUNCTION_WRAPPER_PAYLOAD ||
	    mid == ANNA_MID_FUNCTION_WRAPPER_STACK ||
	    mid == ANNA_MID_STACK_TYPE_PAYLOAD)
	    return mid;
	
	if(type->flags & ANNA_TYPE_MEMBER_DECLARATION_IN_PROGRESS)
	{
	    return mid;
	}
	anna_message(L"Critical: Redeclaring member %ls of type %ls\n",
		name, type->name);
	CRASH;
    }

    anna_type_ensure_mid(type, mid);
        
    anna_member_t * member = anna_slab_alloc(sizeof(anna_member_t));
    memset(member, 0, sizeof(anna_member_t));
    
    member->name = anna_intern(name);
    
    member->type = member_type;
    member->storage = storage;
    if(storage & ANNA_MEMBER_PROPERTY)
    {
	member->offset = -1;
    }
    else
    {
	if(anna_member_is_static(member)) {
	    member->offset = anna_type_static_member_allocate(type);
	    type->static_member[type->static_member_count-1] = null_entry;
	} else {
	    member->offset = type->member_count++;
	}
    }
    
    type->mid_identifier[mid] = member;
    anna_type_calculate_size(type);
    
    if(!(storage & ANNA_MEMBER_PROPERTY))
    {
	type->flags |= ANNA_TYPE_MEMBER_DECLARATION_IN_PROGRESS;
/*
	anna_stack_declare(
	    type->stack,
	    name,
	    member_type,
	    null_entry,
	    0);
*/
	type->flags &= ~ANNA_TYPE_MEMBER_DECLARATION_IN_PROGRESS;
    }
    al_push(&type->member_list, member);

    if(type->flags & ANNA_TYPE_CLOSED)
    {
	anna_type_reseal(type);
    }
    
    member->getter_offset = -1;
    member->setter_offset = -1;
    
    return mid;
}

mid_t anna_member_create_blob(
    anna_type_t *type,
    mid_t mid,
    int storage,
    size_t sz)
{
    mid_t res = anna_member_create(
	type,
	mid,
	storage | (sz << 16) | ANNA_MEMBER_INTERNAL,
	null_type);

    if(storage & ANNA_MEMBER_STATIC)
    {
	anna_type_static_member_allocate(type);
	if(sz > sizeof(void *))
	{
	    debug(D_CRITICAL, L"Static blobs of size larger than a single pointer is currently not implemented.\n");
	    CRASH;
	}
    }
    else
    {
	type->member_count+= ((sz-1)/sizeof(anna_entry_t ));
	anna_type_calculate_size(type);
    }

    return res;
}


anna_member_t *anna_member_get(anna_type_t *type, mid_t mid)
{
    if(mid >= type->mid_count)
    {
	return 0;
    }
    
    return type->mid_identifier[mid];
}

size_t anna_member_create_property(
    anna_type_t *type,
    mid_t mid,
    int storage,
    anna_type_t *property_type,
    ssize_t getter_offset,
    ssize_t setter_offset)
{
    mid = anna_member_create(
	type,
	mid,
	storage | ANNA_MEMBER_PROPERTY,
	property_type);
    anna_member_t *memb = anna_member_get(type, mid);
    
    memb->getter_offset = getter_offset;
    memb->setter_offset = setter_offset;
    return mid;
}

size_t anna_member_create_native_property(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *property_type,
    anna_native_t getter,
    anna_native_t setter,
    wchar_t *doc)
{
    wchar_t *argn[] = 
	{
	    L"this", L"value"
	}
    ;
    anna_type_t *argv[] = 
	{
	    type,
	    property_type
	}
    ;

    size_t getter_mid = -1;
    size_t setter_mid = -1;
    ssize_t getter_offset=-1;
    ssize_t setter_offset=-1;
    string_buffer_t sb;
    sb_init(&sb);

    string_buffer_t sb_doc;
    sb_init(&sb_doc);

    wchar_t *name = anna_mid_get_reverse(mid);

    if(getter)
    {
	sb_printf(&sb, L"!%lsGetter", name);
	
	getter_mid = anna_member_create_native_method(
	    type,
	    anna_mid_get(sb_content(&sb)),
	    0,
	    getter,
	    property_type,
	    1,
	    argv,
	    argn,
	    0, 0);
	anna_member_t *gm = anna_member_get(type, getter_mid);
	getter_offset = gm->offset;
	gm->storage |= ANNA_MEMBER_INTERNAL;
	sb_printf(&sb_doc, L"Getter function for the %ls property.", name);
	
	anna_member_document(
	    type,
	    anna_mid_get(sb_content(&sb)),
	    anna_intern(sb_content(&sb_doc)));
    }
    
    if(setter)
    {
	sb_clear(&sb);
	sb_printf(&sb, L"!%lsSetter", name);
	setter_mid = anna_member_create_native_method(
	    type,
	    anna_mid_get(sb_content(&sb)),
	    0,
	    setter,
	    property_type,
	    2,
	    argv,
	    argn, 0, 0);
	anna_member_t *sm = anna_member_get(type, setter_mid);
	sm->storage |= ANNA_MEMBER_INTERNAL;
	setter_offset = sm->offset;

	sb_clear(&sb_doc);
	sb_printf(&sb_doc, L"Setter function for the %ls property.", name);
	
	anna_member_document(
	    type,
	    anna_mid_get(sb_content(&sb)),
	    anna_intern(sb_content(&sb_doc)));
    }
    sb_destroy(&sb);
    sb_destroy(&sb_doc);
    
    mid = anna_member_create_property(
	type, mid, 0, property_type, 
	getter_offset, setter_offset);
    
    if(doc)
    {
	doc = anna_intern_static(doc);
	anna_member_document(
	    type, mid, doc);
    }
    
    return mid;
}

mid_t anna_member_create_method(
    anna_type_t *type,
    mid_t mid,
    anna_function_t *method)
{
    if(!anna_member_get(type, mid))
    {
	anna_member_create(
	    type,
	    mid,
	    1,
	    anna_function_wrap(method)->type);
    }
    
    anna_member_t *m = type->mid_identifier[mid];
    
    anna_member_set_bound(m, 1);
    type->static_member[m->offset] = 
	anna_from_obj(
	    anna_function_wrap(
		method));
    FIXME("Do we really need to clone these attributes here?");
    m->attribute = (anna_node_call_t *)anna_node_clone_deep((anna_node_t *)method->attribute);
    
    return mid;
}

size_t anna_member_create_native_method(
    anna_type_t *type,
    mid_t mid,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn,
    anna_node_t **argd,
    wchar_t *doc)
{
    wchar_t *name = anna_mid_get_reverse(mid);

    if(!flags) 
    {
	if(!result)
	{
	    CRASH;
	}
	
	if(argc) 
	{
	    assert(argv);
	    assert(argn);
	}
    }
    
    mid = anna_member_create(
	type,
	mid,
	1,
	anna_type_get_function(
	    result, 
	    argc, 
	    argv,
	    argn,
	    argd,
	    flags));
    anna_member_t *m = type->mid_identifier[mid];
    //debug(D_SPAM,L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    anna_member_set_bound(m, 1);
    type->static_member[m->offset] = 
	anna_from_obj(
	    anna_function_wrap(
		anna_native_create(
		    name, flags, func, result, 
		    argc, argv, argn, argd,
		    0)));
    if(doc)
    {
	anna_member_document(type, mid, doc);
    }
    
    return (size_t)mid;
}

size_t anna_member_create_native_type_method(
    anna_type_t *type,
    mid_t mid,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn,
    anna_node_t **argd,
    wchar_t *doc)
{
    wchar_t *name = anna_mid_get_reverse(mid);
    if(!flags) 
    {
	if(!result)
	{
	    CRASH;
	}
	
	if(argc) 
	{
	    assert(argv);
	    assert(argn);
	}
    }
    
    mid = anna_member_create(
	type,
	mid,
	1,
	anna_type_get_function(
	    result, 
	    argc, 
	    argv,
	    argn,
	    argd,
	    flags));
    anna_member_t *m = type->mid_identifier[mid];
    //debug(D_SPAM,L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    type->static_member[m->offset] = 
	anna_from_obj(
	    anna_function_wrap(
		anna_native_create(
		    name, flags, func, result, 
		    argc, argv, argn, argd, 0)));
    if(doc)
    {
	anna_member_document(type, mid, doc);
    }
    return (size_t)mid;
}

void anna_member_document(
    anna_type_t *type,
    mid_t mid,
    wchar_t *doc)
{
    anna_entry_t * e = anna_entry_get_addr_static(type, mid);
    if(e)
    {
	anna_function_t *fun = anna_function_unwrap(anna_as_obj(*e));
	if(fun)
	{
	    anna_function_document(fun, doc);
	    return;
	}
    }
    anna_member_t *memb = anna_member_get(type, mid); 
    if(memb)
    {
	if(!memb->doc && !memb->attribute)
	{
	    memb->doc = doc;
	}
	else
	{
	    anna_node_call_t *attr = anna_node_create_call2(
		0,
		anna_node_create_identifier(0, L"doc"),
		anna_node_create_string_literal(0, wcslen(memb->doc), memb->doc, 0),
		anna_node_create_string_literal(0, wcslen(doc), doc, 0));
	    if(!memb->attribute)
	    {
		memb->attribute = anna_node_create_block2(0);
	    }
	    anna_node_call_push(memb->attribute, (anna_node_t *)attr);
	    memb->doc = 0;
	}
    }
}

void anna_member_document_copy(
    anna_type_t *type,
    mid_t mid,
    anna_node_call_t *src_attribute)
{
    if(!src_attribute)
	return;
    int i;
    array_list_t doc = AL_STATIC;
    anna_attribute_call_all(src_attribute, L"doc", &doc);
    for(i=0; i<al_get_count(&doc); i++)
    {
	anna_node_t *node = al_get(&doc, i);
	if(node->node_type == ANNA_NODE_STRING_LITERAL)
	{
	    anna_node_string_literal_t *str_node =
		(anna_node_string_literal_t *)node;
	    wchar_t *copy = calloc(sizeof(wchar_t), str_node->payload_size+1);
	    memcpy(
		copy, str_node->payload,
		sizeof(wchar_t)* str_node->payload_size);
	    
	    anna_member_document(type, mid, copy);
	}
    }
    al_destroy(&doc);
}

anna_function_type_t *anna_member_bound_function_type(anna_member_t *member)
{
    anna_function_type_t *base = anna_function_type_unwrap(member->type);
    if(!base)
    {
	return 0;
    }
    if(!anna_member_is_bound(member))
    {
	return base;
    }

    return anna_function_type_unwrap(
	anna_type_get_function(
	    base->return_type,
	    base->input_count-1,
	    &base->input_type[1],
	    &base->input_name[1],
	    &base->input_default[1],
	    base->flags));
}

void anna_member_alias(anna_type_t *type, int mid, wchar_t *name)
{
    anna_node_call_t *attr = anna_node_create_call2(
	0,
	anna_node_create_identifier(0, L"alias"),
	anna_node_create_identifier(0, name));
    anna_member_t *memb = anna_member_get(type, mid);
    if(!memb->attribute)
    {
	memb->attribute = anna_node_create_call2(
	    0,
	    anna_node_create_identifier(0, L"__block__"));
    }
    
    anna_node_call_push(memb->attribute, (anna_node_t *)attr);
}

void anna_member_alias_reverse(anna_type_t *type, int mid, wchar_t *name)
{
    anna_node_call_t *attr = anna_node_create_call2(
	0,
	anna_node_create_identifier(0, L"aliasReverse"),
	anna_node_create_identifier(0, name));
    anna_member_t *memb = anna_member_get(type, mid);
    if(!memb->attribute)
    {
	memb->attribute = anna_node_create_call2(
	    0,
	    anna_node_create_identifier(0, L"__block__"));
    }
    
    anna_node_call_push(memb->attribute, (anna_node_t *)attr);
}

