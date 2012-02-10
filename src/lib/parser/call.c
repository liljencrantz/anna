
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
	node,
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

/**
   This is the bulk of the each method
 */
static void anna_node_call_wrapper_each_callback(
    anna_context_t *stack)
{    
    // Discard the output of the previous method call
    anna_context_pop_object(stack);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_entry_t **param = stack->top - 3;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    // Unwrap and name the params to make things more explicit
    anna_node_call_t *call = (anna_node_call_t *)anna_node_unwrap(this);
    anna_object_t *body = anna_as_obj(param[1]);
    int idx = anna_as_int(param[2]);
    size_t sz = call->child_count;
    
    // Are we done or do we need another lap?
    if(idx < sz)
    {
	// Set up params for the next lap of the each body function
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_from_obj(anna_node_wrap(call->child[idx]))
	    }
	;
	// Then update our internal lap counter
	param[2] = anna_from_int(idx+1);
	
	// Finally, roll the code point back a bit and push new arguments
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	// Oops, we're done. Drop our internal param list and push the correct output
	anna_context_drop(stack, 4);
	anna_context_push_entry(stack, param[0]);
    }
}

static void anna_node_call_wrapper_each(anna_context_t *stack)
{
    anna_entry_t *body = anna_context_pop_entry(stack);
    anna_node_call_t *call = (anna_node_call_t *)anna_node_unwrap(anna_context_pop_object(stack));
    anna_context_pop_entry(stack);
    size_t sz = call->child_count;

    if(sz > 0)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(anna_node_wrap((anna_node_t *)call)),
		body,
		anna_from_int(1)
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_from_obj(anna_node_wrap(call->child[0]))
	    }
	;
	
	anna_vm_callback_native(
	    stack,
	    anna_node_call_wrapper_each_callback, 3, callback_param,
	    anna_as_obj_fast(body), 2, o_param
	    );
    }
    else
    {
	anna_context_push_object(stack, anna_node_wrap((anna_node_t *)call));
    }
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

static void anna_node_create_call_type(
    anna_stack_template_t *stack, 
    anna_type_t *type)
{
    mid_t mmid;

    anna_type_copy(type, node_type);

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
    
    anna_type_t *fun_type = anna_type_get_iterator(
	L"!CallIterFunction", int_type, node_type);

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
	type,
	anna_mid_get(L"__each__"), 0,
	&anna_node_call_wrapper_each,
	type,
	2, e_argv, e_argn, 0, 0);

    anna_member_create_native_property(
	type,
	anna_mid_get(L"count"), int_type,
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
    
}
