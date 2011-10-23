

anna_object_t *anna_member_wrap(anna_type_t *type, anna_member_t *result)
{
    if(result->wrapper)
	return result->wrapper;
    
    anna_type_t * m_type;
    if(result->is_bound_method)
    {
	m_type = member_method_type;
    }
    else if(result->is_property)
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
    if((type->flags & ANNA_TYPE_CLOSED) && !(storage & ANNA_MEMBER_STATIC) && !(storage & ANNA_MEMBER_VIRTUAL))
    {
	debug(D_CRITICAL, L"Added additional non-static member %ls after closing type %ls\n", anna_mid_get_reverse(mid), type->name);
	CRASH;
    }    

    wchar_t *name = anna_mid_get_reverse(mid);
    
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
	    return anna_mid_get(name);
	}
	wprintf(L"Critical: Redeclaring member %ls of type %ls\n",
		name, type->name);
	CRASH;
    }

    anna_type_ensure_mid(type, mid);
        
    anna_member_t * member = calloc(1,sizeof(anna_member_t));
    
    member->name = anna_intern(name);
    
    member->type = member_type;
    member->is_static = !!(storage & ANNA_MEMBER_STATIC);
    member->storage = storage;
    if(storage & ANNA_MEMBER_VIRTUAL)
    {
	member->offset = -1;
    }
    else
    {
	if(member->is_static) {
	    member->offset = anna_type_static_member_allocate(type);
	    type->static_member[type->static_member_count-1] = null_entry;
	} else {
	    member->offset = type->member_count++;
	}
    }
    
    type->mid_identifier[mid] = member;
//    wprintf(L"Create member named %ls to type %ls\n", name, type->name);
        
    anna_type_calculate_size(type);

    if(!(storage & ANNA_MEMBER_VIRTUAL))
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
	storage,
	null_type);    

    if(storage & ANNA_MEMBER_STATIC)
    {
	anna_type_static_member_allocate(type);
    }
    else
    {
	type->member_count+= ((sz-1)/sizeof(anna_entry_t *));
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

anna_member_t *anna_member_method_search(
    anna_type_t *type,
    mid_t mid, 
    anna_node_call_t *call,
    int is_reverse)
{
    debug(D_SPAM, L"SEARCH for match to %ls in type %ls\n", anna_mid_get_reverse(mid), type->name);
    int i;

    wchar_t *alias_name = anna_mid_get_reverse(mid);
    
    wchar_t *match=0;
    int fault_count=0;

    for(i=0; i<anna_type_get_member_count(type); i++)
    {
	anna_member_t *member = anna_type_get_member_idx(type, i);
//	debug(D_ERROR, L"Check %ls %d %d %ls\n", members[i],
//	      member->is_static, member->offset, member->type->name);
	if(member->is_static && member->offset>=0 && member->type != null_type)
	{
	    anna_object_t *mem_val = anna_as_obj(type->static_member[member->offset]);
	    anna_function_t *mem_fun = anna_function_unwrap(mem_val);
	    
	    if(!mem_fun)
	    {
		continue;
	    }
	    
	    anna_function_type_t *mem_fun_type = anna_function_type_unwrap(
		member->type);
	    
	    int has_alias = is_reverse ? anna_function_has_alias_reverse(mem_fun, alias_name):anna_function_has_alias(mem_fun, alias_name);
	    has_alias |= (!is_reverse && wcscmp(member->name, alias_name)==0);
	    
	    if(has_alias)
	    {
		int j;
		int off = !!member->is_bound_method && !(call->access_type & ANNA_NODE_ACCESS_STATIC_MEMBER);
		
		if(mem_fun->input_count != call->child_count+off)
		    continue;	    
		//debug(D_SPAM, L"YAY, right number of arguments (%d)\n", argc);
		
		debug(D_SPAM, L"Check %ls against %ls\n",call->child[0]->return_type->name, mem_fun->input_type[off]->name);
		int my_fault_count = 0;
		int ok1 = anna_node_validate_call_parameters(
		    call, mem_fun_type, off, 0);
		int ok2 = 1;
		
		if(ok1)
		{
		    anna_node_call_t *call_copy = (anna_node_call_t *)anna_node_clone_shallow((anna_node_t *)call);
		    anna_node_call_map(call_copy, mem_fun_type, off);
		    
		    for(j=0; j<call->child_count; j++)
		    {
			if(anna_abides(call_copy->child[j]->return_type, mem_fun->input_type[j+off]))
			{
			    my_fault_count += 
				anna_abides_fault_count(mem_fun->input_type[j+off], call_copy->child[j]->return_type);
			}
			else
			{
			    ok2=0;
			    debug(D_SPAM, L"Argument %d, %ls does not match %ls!\n", j, 
				  call_copy->child[j]->return_type->name, mem_fun->input_type[j+off]->name);
			}
			
		    }
		}
		
		if(ok1 && ok2){
		    debug(D_SPAM, L"Match!\n");
		    
		    if(!match || my_fault_count < fault_count)
		    {
			match = member->name;
			fault_count = my_fault_count;
		    }
		}
	    }
	}
	else
	{
	    debug(D_SPAM, L"Not a function\n");
	}	
    }
    
    if(match)
    {
	debug(D_SPAM, L"Match: %ls\n", match);
    }
    
    return match ? anna_member_get(type, anna_mid_get(match)):0;
    
}

size_t anna_member_create_property(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *property_type,
    ssize_t getter_offset,
    ssize_t setter_offset)
{
    mid = anna_member_create(
	type,
	mid,
	ANNA_MEMBER_VIRTUAL,
	property_type);
    anna_member_t *memb = anna_member_get(type, mid);
    
    memb->is_property=1;
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
	type, mid, property_type, 
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
    
    m->is_bound_method=1;
    type->static_member[m->offset] = 
	anna_from_obj(
	    anna_function_wrap(
		method));
    
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
	anna_type_for_function(
	    result, 
	    argc, 
	    argv,
	    argn,
	    argd,
	    flags));
    anna_member_t *m = type->mid_identifier[mid];
    //debug(D_SPAM,L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    m->is_bound_method=1;
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
	anna_type_for_function(
	    result, 
	    argc, 
	    argv,
	    argn,
	    argd,
	    flags));
    anna_member_t *m = type->mid_identifier[mid];
    //debug(D_SPAM,L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    m->is_bound_method=0;
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

    anna_entry_t ** e = anna_entry_get_addr_static(type, mid);
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
	anna_node_call_t *attr = anna_node_create_call2(
	    0,
	    anna_node_create_identifier(0, L"doc"),
	    anna_node_create_string_literal(0, wcslen(doc), doc));
       if(!memb->attribute)
       {
           memb->attribute = anna_node_create_block2(0);
       }
       anna_node_call_add_child(memb->attribute, (anna_node_t *)attr);
       
//	al_push(&memb->doc, doc);
    }
    
}
