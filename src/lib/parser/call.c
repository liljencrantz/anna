
ANNA_VM_NATIVE(anna_node_call_wrapper_i_get_count, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(this);
    return anna_from_int(node->child_count);
}

static void anna_node_call_wrapper_all_children_each(anna_node_t *node, void *aux)
{
    anna_object_t *list = (anna_object_t *)aux;
    anna_list_add(
	list, 
	anna_from_obj(anna_node_wrap(node)));
}

ANNA_VM_NATIVE(anna_node_call_wrapper_i_all_children, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(this);
    anna_object_t *res = 
	anna_list_create_imutable(node_type);
    anna_node_each(
	(anna_node_t *)node,
	anna_node_call_wrapper_all_children_each,
	res);
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_node_call_wrapper_i_get_int, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(this);
    int idx = anna_list_calc_offset(anna_as_int(param[1]), node->child_count);
    if(idx < 0 || idx >= node->child_count)
	return null_entry;
    return anna_from_obj(anna_node_wrap(node->child[idx]));
}

ANNA_VM_NATIVE(anna_node_call_wrapper_i_get_range, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(this);

    anna_object_t *range = anna_as_obj(param[1]);
    int from = anna_range_get_from(range);
    int step = anna_range_get_step(range);
    int to = anna_range_get_to(range);
    int i;

    if(anna_range_get_open(range))
    {
	to = step>0?node->child_count:-1;
    }
    
    anna_object_t *res = 
	anna_list_create_imutable(node_type);

    for(i=from;(step>0)? i<to : i>to; i+=step)
    {
	anna_list_add(
	    res, 
	    anna_from_obj(anna_node_wrap(node->child[i])));
    }
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_node_call_wrapper_i_set_int, 3)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    
    ANNA_ENTRY_NULL_CHECK(param[1]);
    
    if(anna_entry_null(param[2]))
	param[2] = anna_from_obj(anna_node_wrap(anna_node_create_null(0)));
    
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(this);
    int idx = anna_list_calc_offset(anna_as_int(param[1]), node->child_count);
    if(idx < 0 || idx >= node->child_count)
	return param[1];
    anna_node_t *val = anna_node_unwrap(anna_as_obj(param[2]));
    assert(val);
    
    node->child[idx] = val;

    return param[2];
}

ANNA_VM_NATIVE(anna_node_call_wrapper_i_get_function, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(this);

    return anna_from_obj(anna_node_wrap(node->function));
}

ANNA_VM_NATIVE(anna_node_call_wrapper_i_set_function, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(this);
    ANNA_ENTRY_NULL_CHECK(param[1]);

    node->function = anna_node_unwrap(anna_as_obj(param[1]));
    return param[1];    
}

ANNA_VM_NATIVE(anna_node_call_wrapper_i_join_list, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);

    size_t count = 
	anna_list_get_count(anna_as_obj(param[1]));
    
    anna_node_call_t *src = 
	(anna_node_call_t *)anna_node_unwrap(
	    this);
    anna_node_call_t *dst = 
	anna_node_create_call(
	    &src->location,
	    src->function,
	    src->child_count,
	    src->child);
    int i;
    for(i=0;i<count; i++)
    {
	anna_entry_t *n = 
	    anna_list_get(anna_as_obj(param[1]), i);
	anna_node_call_add_child(
	    dst,
	    anna_node_unwrap(anna_as_obj(n)));
    }
    return anna_from_obj(anna_node_wrap((anna_node_t *)dst));
}

ANNA_VM_NATIVE(anna_node_call_wrapper_i_push, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    anna_node_call_t *this = 
	(anna_node_call_t *)anna_node_unwrap(anna_as_obj_fast(param[0]));
    anna_node_t *value = anna_node_unwrap(anna_as_obj_fast(param[1]));
    anna_node_call_add_child(
	this,
	value);
    return param[0];
}

ANNA_VM_NATIVE(anna_node_call_wrapper_i_join_call, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    
    anna_node_call_t *lst1 = 
	(anna_node_call_t *)anna_node_unwrap(
	    anna_as_obj(param[0]));

    anna_node_call_t *lst2 = 
	(anna_node_call_t *)anna_node_unwrap(
	    anna_as_obj(param[1]));

    anna_node_call_t *dst = 
	anna_node_create_call(
	    &lst1->location,
	    lst1->function,
	    lst1->child_count,
	    lst1->child);
    int i;
    for(i=0;i<lst2->child_count; i++)
    {
	anna_node_call_add_child(
	    dst,
	    lst2->child[i]);
    }
    return anna_from_obj(anna_node_wrap((anna_node_t *)dst));
}

ANNA_VM_NATIVE(anna_node_call_wrapper_i_init, 4)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    size_t sz = anna_list_get_count(anna_as_obj(param[3]));
    anna_entry_t **src = anna_list_get_payload(anna_as_obj(param[3]));
    
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    anna_node_t *function = anna_node_unwrap(anna_as_obj(param[2]));
    
    anna_node_call_t *dest = 
	anna_node_create_call(
	    source?&source->location:0,
	    function,
	    0,
	    0);
    int i;
    for(i=0; i<sz; i++)
    {
	if(!anna_entry_null(src[i]))
	{
//	    anna_object_print(anna_as_obj(src[i]));
	    anna_node_call_add_child(dest, anna_node_unwrap(anna_as_obj(src[i])));
	}
	else
	{
	    anna_error(source, L"Element number %d in call list is invalid", i+1);
	}
	
    }
    
    *(anna_node_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	(anna_node_t *)dest;
	
    return param[0];
}

ANNA_VM_NATIVE(anna_node_call_wrapper_append, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_call_t *call = (anna_node_call_t *)anna_node_unwrap(this);
    
    anna_object_t *list = anna_as_obj(param[1]);
    
    if(list == null_object)
    {
	return param[0];
    }

    size_t i;    
    size_t size2 = anna_list_get_count(list);
    for(i=0; i<size2; i++)
    {
	anna_object_t *ch_obj = anna_as_obj(anna_list_get(list, i));
	anna_node_call_add_child(call, anna_node_unwrap(ch_obj));
    }
    
    return param[0];
}


static void anna_call_iterator_update(anna_object_t *iter, int off)
{
    anna_entry_set(iter, ANNA_MID_KEY, anna_from_int(off));
}

ANNA_VM_NATIVE(anna_call_get_iterator, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *this = anna_as_obj(param[0]);
    anna_object_t *iter = anna_object_create(
	anna_type_unwrap((anna_object_t *)anna_entry_get_static(this->type, ANNA_MID_ITERATOR_TYPE)));
    anna_entry_set(iter, ANNA_MID_COLLECTION, param[0]);
    anna_call_iterator_update(iter, 0);
    return anna_from_obj(iter);
}

ANNA_VM_NATIVE(anna_call_iterator_next, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_call_iterator_update(iter, anna_as_int(anna_entry_get(iter, ANNA_MID_KEY))+1);
    return param[0];
}

ANNA_VM_NATIVE(anna_call_iterator_valid, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *this = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    anna_node_call_t *call = (anna_node_call_t *)anna_node_unwrap(this);
    int offset = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));
    return (offset >= 0  && offset < call->child_count) ? anna_from_int(1) : null_entry;
}

ANNA_VM_NATIVE(anna_call_iterator_get_value, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *this = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    anna_node_call_t *call = (anna_node_call_t *)anna_node_unwrap(this);
    int off = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));
    
    if((off >= 0) && (off < call->child_count))
    {
	return anna_from_obj(anna_node_wrap(call->child[off]));
    }
    else
    {
	return null_entry;
    }
}

ANNA_VM_NATIVE(anna_call_iterator_set_value, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);

    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *this = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    anna_node_call_t *call = (anna_node_call_t *)anna_node_unwrap(this);
    int off = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));

    if((off >= 0) && (off < call->child_count))
    {
	call->child[off] = anna_node_unwrap(anna_as_obj(param[1]));
    }
    return param[1];
}


static anna_type_t *anna_call_iterator_create(
    anna_type_t *type)
{
    anna_type_t *iter = anna_type_create(L"Iterator", 0);
    anna_member_create(
	iter, ANNA_MID_COLLECTION, ANNA_MEMBER_IMUTABLE, type);
    anna_member_create(
	iter, ANNA_MID_KEY, ANNA_MEMBER_IMUTABLE, int_type);
    anna_member_create_native_property(
	iter, ANNA_MID_VALUE, node_type,
	&anna_call_iterator_get_value,
	&anna_call_iterator_set_value,
	0);
    anna_type_copy_object(iter);
    
    anna_member_create_native_property(
	iter, ANNA_MID_VALID, object_type,
	&anna_call_iterator_valid,
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
	&anna_call_iterator_next, iter, 1,
	iter_argv, iter_argn, 0, 0);

    anna_util_iterator_iterator(iter);
        
    anna_type_close(iter);
    
    return iter;
}




static void anna_node_create_call_type(
    anna_stack_template_t *stack, 
    anna_type_t *type)
{
    mid_t mmid;

    anna_member_create(
	type,
	ANNA_MID_ITERATOR_TYPE,
	ANNA_MEMBER_STATIC,
	type_type);
    anna_type_t *iter = anna_call_iterator_create(type);
    anna_entry_set_static(
	type, ANNA_MID_ITERATOR_TYPE, 
	anna_from_obj(anna_type_wrap(iter)));
    anna_member_create_native_property(
	type, ANNA_MID_ITERATOR, iter,
	&anna_call_get_iterator, 0, 0);
    
    anna_type_t *argv[] = 
	{
	    type,
	    node_type,
	    node_type,
	    node_type
	}
    ;
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"func", L"param"
	}
    ;    

    anna_member_create_native_method(
	type,
	anna_mid_get(L"__init__"),
	ANNA_FUNCTION_VARIADIC,
	&anna_node_call_wrapper_i_init,
	object_type,
	4, argv, argn, 0, 0);
    
    anna_member_create_native_property(
	type,
	ANNA_MID_COUNT, int_type,
	&anna_node_call_wrapper_i_get_count, 0,
	L"The number of argument nodes in this Call.");
  
    anna_member_create_native_property(
	type,
	anna_mid_get(L"allChildren"), anna_list_type_get_imutable(node_type),
	&anna_node_call_wrapper_i_all_children, 0,
	L"All descendant nodes of this node, both direct and indirect.");
  


    anna_type_t *i_argv[] = 
	{
	    type,
	    int_type,
	    node_type
	}
    ;
    
    wchar_t *i_argn[] =
	{
	    L"this", L"index", L"value"
	}
    ;
    
    anna_type_t *range_argv[] = 
	{
	    type,
	    range_type
	}
    ;

    wchar_t *range_argn[] =
	{
	    L"this", L"range"
	}
    ;

    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__get__Int__"), 0,
	&anna_node_call_wrapper_i_get_int,
	node_type,
	2, i_argv, i_argn, 0, L"Returns the child node at the specified offset.");
    anna_member_alias(type, mmid, L"__get__");

    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__get__Range__"), 0,
	&anna_node_call_wrapper_i_get_range, 
	anna_list_type_get_imutable(node_type), 
	2, range_argv, range_argn, 0, L"Returns the list of nodes in the specified Range.");
    anna_member_alias(type, mmid, L"__get__");

    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__set__Int__"), 0,
	&anna_node_call_wrapper_i_set_int,
	node_type,
	3,
	i_argv,
	i_argn, 0, L"Set the child node at the specified offset to the specified value.");
    anna_member_alias(type, mmid, L"__set__");

    anna_member_create_native_property(
	type,
	anna_mid_get(L"function"), node_type,
	&anna_node_call_wrapper_i_get_function,
	&anna_node_call_wrapper_i_set_function,
	L"The function node of this call.");
    
    anna_type_t *jl_argv[] = 
	{
	    type,
	    anna_list_type_get_any(node_type)
	}
    ;
    
    anna_type_t *jc_argv[] = 
	{
	    type,
	    type
	}
    ;
    
    wchar_t *j_argn[] =
	{
	    L"this", L"list"
	}
    ;
    
    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__join__List__"),
	0,
	&anna_node_call_wrapper_i_join_list,
	type,
	2,
	jl_argv,
	j_argn, 0, L"Create a new Call node, with the function node of this Call node and all the child nodes of both this Call node and the specified List");
    anna_member_alias(type, mmid, L"__join__");

    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__join__Call__"),
	0,
	&anna_node_call_wrapper_i_join_call,
	type,
	2,
	jc_argv,
	j_argn, 0, 0);
    anna_member_alias(type, mmid, L"__join__");

    wchar_t *push_argn[] = 
	{
	    L"this",
	    L"value"
	};

    anna_member_create_native_method(
	type,
	anna_mid_get(L"push"),
	0,
	&anna_node_call_wrapper_i_push,
	type,
	2, argv, push_argn, 
	0, L"Append an additional child node to the end of the Call child list");

    anna_member_create_native_method(
	type,
	anna_mid_get(L"__appendAssign__"), 0,
	&anna_node_call_wrapper_append,
	type,
	2,
	jl_argv,
	j_argn, 0, 0);

    anna_type_copy(type, node_type);
}
