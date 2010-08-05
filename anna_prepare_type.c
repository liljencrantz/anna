
static anna_node_t *anna_type_member(anna_type_t *type,
				     struct anna_node_call *node, 
				     struct anna_function *function,
				     struct anna_node_list *parent)
{
    CHECK_CHILD_COUNT(node,L"variable declaration", 3);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    anna_node_prepare_children(node, function, parent);
    anna_node_identifier_t *name_identifier = node_cast_identifier(node->child[0]);
    anna_type_t *var_type;
    switch(node->child[1]->node_type) 
    {
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *type_identifier;
	    type_identifier = node_cast_identifier(node->child[1]);
	    anna_object_t *type_wrapper = anna_stack_get_str(function->stack_template, type_identifier->name);
	    assert(type_wrapper);
	    var_type = anna_type_unwrap(type_wrapper);
	    break;
	}
	
	case ANNA_NODE_NULL:	
	    var_type = anna_node_get_return_type(node->child[2], function->stack_template);
	    //wprintf(L"Implicit var dec type: %ls\n", type->name);
	    break;

	default:
	    FAIL(node->child[1], L"Wrong type on second argument to declare - expected an identifier or a null node");
    }
    
    assert(var_type);

    anna_member_create(type, -1, name_identifier->name, 0, var_type);
    return anna_node_create_null(0);
    
    //anna_stack_declare(function->stack_template, name_identifier->name, type, null_object);
    
    /*
      anna_node_t *a_param[2]=
      {
      node->child[0],
      node->child[2]
      }
      ;
    
      return (anna_node_t *)
      anna_node_create_call(&node->location,
      (anna_node_t *)anna_node_create_identifier(&node->location,
      L"__assign__"),
      2,
      a_param);
    */
}

void anna_prepare_type_interface(
    anna_type_t *type)
{
    prepare_item_t it = 
	{
	    0, type, L"Interface preparation"
	}
    ;
    if(anna_prepare_check(&it))
	return;
    
    anna_prepare_type_interface_internal(type);
    anna_prepare_pop();
    
}

static anna_node_t *anna_prepare_type_interface_internal(
    anna_type_t *type)
{
    
    if(anna_type_prepared(type))
	return 0;
    
    type->flags |= ANNA_TYPE_PREPARED_INTERFACE;

    anna_function_t *function;
    
    function = anna_native_create(
	L"!typePrepareFunction",
	ANNA_FUNCTION_MACRO,
	(anna_native_t)(anna_native_function_t)0,
	null_type,
	0,
	0, 
	0,
	0);

    function->stack_template=type->stack;
    
//    wprintf(L"Prepare type %ls\n", type->name);
    //anna_node_print(type->definition);
//	wprintf(L"\n");

    anna_node_call_t *node = (anna_node_call_t *)anna_node_clone_deep((anna_node_t *)type->definition);

    CHECK_CHILD_COUNT(node,L"type macro", 4);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[3]);
    CHECK_NODE_BLOCK(node->child[2]);
    
    anna_node_call_t *attribute_list = 
        (anna_node_call_t *)node->child[2];
    int i;

    array_list_t property_list;
    al_init(&property_list);
    
    for(i=0; i<attribute_list->child_count;i++)
    {
	CHECK_NODE_TYPE(attribute_list->child[i], ANNA_NODE_CALL);
	anna_node_call_t *attribute = 
	    (anna_node_call_t *)attribute_list->child[i];
	CHECK_NODE_TYPE(attribute->function, ANNA_NODE_IDENTIFIER);
	
	anna_node_identifier_t *id = 
	    (anna_node_identifier_t *)attribute->function;
	
	string_buffer_t sb;
	sb_init(&sb);
	sb_append(&sb, L"__");
	sb_append(&sb, id->name);
	sb_append(&sb, L"Attribute__");
	wchar_t *name = sb_content(&sb);
	
	anna_node_call_t *attribute_call_node =
	    anna_node_create_call(&attribute->location,
				  (anna_node_t *)anna_node_create_identifier(&attribute->location,
									     name),
				  0,
				  0);
	
	anna_node_call_add_child(
	    attribute_call_node,
	    (anna_node_t *)attribute);
	anna_node_call_add_child(
	    attribute_call_node, 
	    (anna_node_t *)node);
	anna_function_t *macro_definition = anna_node_macro_get(
	    attribute_call_node, 
	    function->stack_template);
	CHECK(macro_definition, id, L"No such attribute macro found: %ls", name);
	
	anna_node_t *tmp = anna_macro_invoke(
	    macro_definition,
	    attribute_call_node,
	    function,
	    0);
	CHECK_NODE_TYPE(tmp, ANNA_NODE_CALL);
	node = (anna_node_call_t *)tmp;
	/*
(anna_node_call_t *)macro_definition->native.macro(
	    attribute_call_node, 
	    function,
	    parent);
	*/
	CHECK(node->node_type == ANNA_NODE_CALL, attribute_list, L"Attribute call %ls did not return a valid type definition", id->name);
	sb_destroy(&sb);	
    }
    
    int error_count=0;
    
    anna_node_call_t *body = (anna_node_call_t *)node->child[3];
    
    for(i=0; i<body->child_count; i++)
    {
	anna_node_t *item = body->child[i];
	
	if(item->node_type != ANNA_NODE_CALL) 
	{
	    anna_error(
		item,
		L"Only function declarations and variable declarations allowed directly inside a class body" );
	    error_count++;
	    continue;
	}
	
	anna_node_call_t *call = (anna_node_call_t *)item;
	if(call->function->node_type != ANNA_NODE_IDENTIFIER)
	{
	    anna_error(
		call->function,
		L"Only function declarations and variable declarations allowed directly inside a class body" ); 
	    error_count++;
	    continue;
	}
	
	anna_node_identifier_t *declaration = 
	    (anna_node_identifier_t *)call->function;
	
	if(wcscmp(declaration->name, L"__function__")==0)
	{
/*
	    anna_macro_function_internal(type, call, function, 0, 0);
*/

	    anna_function_t *result;
	    result = anna_function_create_from_definition(
		call,
		function->stack_template);
	    result->member_of = type;
	    	    
	    //wprintf(L"Creating method %ls\n", result->name);
	    
//	    result->flags |= ANNA_MID_FUNCTION_WRAPPER_STACK;
	    result->mid = anna_method_create(type, -1, result->name, 0, result);	
	    
/*
  This is a method declaration
 */
	    

	}
	else if(wcscmp(declaration->name, L"__declare__")==0)
	{
	    anna_type_member(type, call, function, 0);
	}
	else if(wcscmp(declaration->name, L"__functionNative__")==0)
	{
	    int i;
	    int argc;
	    wchar_t **argn;
	    anna_type_t **argv;
	    
	    anna_node_identifier_t *name = 
		(anna_node_identifier_t *)call->child[0];

//	    wprintf(L"Prepare member %ls\n", name->name);

//	    anna_stack_print(function->stack_template);
	    
	    
	    anna_node_prepare_child(call, 1, function, 0);
	    
	    anna_type_t *return_type = call->child[1]->node_type == ANNA_NODE_NULL?0:
		anna_prepare_type_from_identifier(
		    call->child[1],
		    function,
		    0);

	    anna_node_call_t *param_list = 
		(anna_node_call_t *)call->child[2];
	    
	    argc = param_list->child_count;
	    argv = malloc(sizeof(anna_type_t *)*argc);
	    argn = malloc(sizeof(wchar_t *)*argc); 
	    for(i=0; i<argc; i++)
	    {
		anna_node_call_t *param =
		    (anna_node_call_t *)param_list->child[i];
		anna_node_identifier_t *param_name = 
		    (anna_node_identifier_t *)param->child[0];
		
		argv[i] = anna_prepare_type_from_identifier(
		    param->child[1],
		    function,
		    0);
		if(anna_type_is_fake(argv[i]))
		{
		    wprintf(
			L"Critical: Preparation resulted in fake function type\n");
		    CRASH;
		}

		argn[i] = param_name->name;
	    }
	    
	    anna_node_int_literal_t *mid = 
		(anna_node_int_literal_t *)call->child[3];

	    anna_node_int_literal_t *flags = 
		(anna_node_int_literal_t *)call->child[4];

	    anna_node_dummy_t *func = 
		(anna_node_dummy_t *)call->child[5];

	    anna_native_method_create(
		type,
		(size_t)mid->payload,
		name->name,
		flags->payload,
		(anna_native_t)(anna_native_function_t)func->payload,
		return_type,
		argc,
		argv,
		argn);
	    free(argv);
	}
	else if(wcscmp(declaration->name, L"__declareNative__")==0)
	{

	    anna_node_identifier_t *name = 
		(anna_node_identifier_t *)call->child[0];

	    anna_type_t *return_type = 
		anna_prepare_type_from_identifier(
		    call->child[1], 	
		    function,
		    0);

	    anna_node_int_literal_t *mid = 
		(anna_node_int_literal_t *)call->child[2];

	    anna_node_int_literal_t *is_static = 
		(anna_node_int_literal_t *)call->child[3];

	    
	    anna_member_create(
		type,
		mid->payload,
		name->name,
		is_static->payload,
		return_type);
	}
	else if(wcscmp(declaration->name, L"__property__")==0)
	{
	    al_push(&property_list, item);
	}
    	else
	{
	    anna_error(call->function,
		       L"Only function declarations and variable declarations allowed directly inside a class body" );
	    error_count++;	    
	}
    }
    for(i=0; i<al_get_count(&property_list); i++)
    {
	anna_node_call_t *prop = al_get(&property_list, i);
	CHECK_CHILD_COUNT(prop,L"property", 3);
	//anna_node_prepare_child(prop, 0, function, 0);
	anna_node_prepare_child(prop, 1, function, 0);
	
	CHECK_NODE_TYPE(prop->child[0], ANNA_NODE_IDENTIFIER);
	CHECK_NODE_TYPE(prop->child[1], ANNA_NODE_IDENTIFIER);
	CHECK_NODE_TYPE(prop->child[2], ANNA_NODE_CALL);
	
	anna_node_identifier_t *name = 
	    (anna_node_identifier_t *)prop->child[0];
	
	anna_type_t *p_type = 
	    anna_prepare_type_from_identifier(
		prop->child[1], 	
		function,
		0);
	
	//wprintf(L"Wee, declare %ls\n", name->name);
	
	anna_member_create(type,
			   -1,
			   name->name,
			   0,
			   p_type);
	type->member_count--;
	type->property_count++;
	anna_member_t *memb = anna_type_member_info_get(type, name->name);
	memb->is_property = 1;
	
	anna_node_call_t *attribute_list = (anna_node_call_t *)prop->child[2];
	int j;
	CHECK_NODE_IDENTIFIER_NAME(attribute_list->function, L"__block__");
	
	for(j=0; j<attribute_list->child_count; j++)
	{
//	   anna_node_print(attribute_list->child[j]);
	   CHECK_NODE_TYPE(attribute_list->child[j], ANNA_NODE_CALL);
	   anna_node_call_t *attribute = (anna_node_call_t *)attribute_list->child[j];
	   CHECK_CHILD_COUNT(attribute, L"setter or getter", 1);
	   CHECK_NODE_TYPE(attribute->function, ANNA_NODE_IDENTIFIER);
	   CHECK_NODE_TYPE(attribute->child[0], ANNA_NODE_IDENTIFIER);
	   
	    anna_node_identifier_t *a_type = 
		(anna_node_identifier_t *)attribute->function;
	    
	    anna_node_identifier_t *m_name = 
		(anna_node_identifier_t *)attribute->child[0];
	    
	    anna_member_t *method = anna_type_member_info_get(type, m_name->name);
	    CHECK(method, m_name, L"Unknown method \"%ls\" in class \"%ls\"", m_name->name, type->name);

	    /*
	      Fixme: Check that [gs]etter has correct signature
	    */

	    if(wcscmp(L"getter", a_type->name) == 0)
	    {
	       memb->getter_offset = method->offset;
	    }
	    else if(wcscmp(L"setter", a_type->name) == 0)
	    {
	       memb->setter_offset = method->offset;
	    }
	    else
	    {
	       FAIL(a_type, L"Unknown attribute");
	    }
	
	}
    }
    
    if(error_count)
	return (anna_node_t *)anna_node_create_null(&node->location);

    if(type->mid_identifier[ANNA_MID_STACK_PAYLOAD] && type != null_type)
    {
	anna_stack_prepare(type);
    }
        
    anna_object_t **constructor_ptr = 
	anna_static_member_addr_get_mid(type, ANNA_MID_INIT_PAYLOAD);
    
    if(constructor_ptr)
    {
	anna_function_t *constructor = 
	    anna_function_unwrap(*constructor_ptr);
	anna_prepare_function_interface(constructor);
	
	anna_type_t **argv= malloc(sizeof(anna_type_t *)*(constructor->input_count));
	wchar_t **argn= malloc(sizeof(wchar_t *)*(constructor->input_count));
		
	argv[0]=type_type;
	argn[0]=L"this";
	
	for(i=1; i<constructor->input_count; i++)
	{
	    argv[i] = constructor->input_type[i];
	    argn[i] = constructor->input_name[i];
	}
    }
    
    memcpy(
	&type->child_function, 
	&function->child_function,
	sizeof(array_list_t));
    
    memcpy(
	&type->child_type, 
	&function->child_type,
	sizeof(array_list_t));
    
/*
  wprintf(L"Base type after transformations\n");
  anna_node_print(type->definition);
  wprintf(L"\n");
*/  
    return (anna_node_t *)anna_node_create_dummy(&node->location,
						 anna_type_wrap(type),
						 0);
/*    
      wprintf(L"Create __call__ for non-native type %ls\n", type->name);
*/  
}

void anna_prepare_type_implementation(anna_type_t *type)
{
    prepare_item_t it = 
	{
	    0, type, L"Implementation preparation"
	}
    ;
    if(anna_prepare_check(&it))
	return;

    int i;
    type->flags |= ANNA_TYPE_PREPARED_IMPLEMENTATION;
//    wprintf(L"Lala, prepare implementation of type %ls\n", type->name);
    
    for(i=0; i<al_get_count(&type->child_function); i++)
    {
	anna_function_t *fun = 
	    (anna_function_t *)al_get(
		&type->child_function, i);
	anna_prepare_function(fun);
    }

    anna_prepare_pop();

}