

anna_node_t *anna_node_call_prepare(
    anna_node_call_t *node, 
    anna_function_t *function,
    anna_node_list_t *parent)
{
    
    anna_node_list_t list = 
	{
	    (anna_node_t *)node, 0, parent
	}
    ;

    int i;
/*
    wprintf(L"Prepare call node:\n");
    anna_node_print((anna_node_t *)node);
*/
    if(node->node_type == ANNA_NODE_CALL)
    {       
	anna_function_t *macro_definition = anna_node_macro_get(node, function->stack_template);
	
	if(macro_definition)
	{       
	    //wprintf(L"Macro\n");
	    anna_node_t *macro_output;
	    //ANNA_PREPARED(node->function);
	    
	    macro_output = anna_macro_invoke(
		macro_definition, node, function, parent);
	    return anna_node_prepare(macro_output, function, parent);
	}
	else {
	    //wprintf(L"Plain function\n");
	}
	
	node->function = anna_node_prepare(node->function, function, parent);
	anna_type_t *func_type = anna_node_get_return_type(node->function, function->stack_template);       
	if(func_type == type_type)
	{
	    /*
	      Constructor!
	    */
	    node->node_type = ANNA_NODE_CONSTRUCT;
	    node->function = (anna_node_t *)anna_node_create_dummy(
		&node->location, 
		anna_node_invoke(node->function, function->stack_template),
		0);
	    node->function = anna_node_prepare(node->function, function, &list);
	    
	    for(i=0; i<node->child_count; i++)
	    {
		list.idx = i;
		node->child[i] = anna_node_prepare(node->child[i], function, &list);	 
	    }
	   
	    return (anna_node_t *)node;
	}
    }
    else 
    {
	node->function = anna_node_prepare(node->function, function, &list);
    }

    for(i=0; i<node->child_count; i++)
    {
	list.idx = i;
	node->child[i] = anna_node_prepare(node->child[i], function, &list);	 
    }
    return (anna_node_t *)node;
}


anna_node_t *anna_node_prepare(
    anna_node_t *this, 
    anna_function_t *function, 
    anna_node_list_t *parent)
{
    anna_node_list_t list = 
	{
	    this, 0, parent
	}
    ;

    ANNA_PREPARED(this);    
/*
    wprintf(L"Prepare node:\n");
    
    anna_node_print(this);
*/  
    switch(this->node_type)
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	    //wprintf(L"It's a call\n");
	    return anna_node_call_prepare((anna_node_call_t *)this, function, parent);

	case ANNA_NODE_RETURN:
	{
	    anna_node_return_t * result = (anna_node_return_t *)this;
	    result->payload=anna_node_prepare(result->payload, function, &list);
	    return (anna_node_t *)result;
	}
	
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *this2 =(anna_node_identifier_t *)this;
	    /*
	    if(anna_node_identifier_is_function(
		   this2,
		   function->stack_template))
	    {
		this2 = anna_node_create_identifier_trampoline(
		    &this2->location, 
		    this2->name);
		ANNA_PREPARED(this2);    
	    }
	    */
	    this2->sid = anna_stack_sid_create(function->stack_template, this2->name);
/*
	    if(wcscmp(this2->name, L"print")==0)
	    {
		wprintf(L"Preparing print node with stack trace:\n");
		
		anna_stack_print_trace(function->stack_template);
	    }
*/	    
/*
	    if(wcscmp(this2->name,L"print")==0)
	    {
		anna_stack_print_trace(function->stack_template);
		//CRASH;
		
	    }
*/	    
	    return (anna_node_t *)this2;
	}	

	case ANNA_NODE_IDENTIFIER_TRAMPOLINE:
	{
	    anna_node_identifier_t *this2 =(anna_node_identifier_t *)this;
	    this2->sid = anna_stack_sid_create(function->stack_template, this2->name);
/*
	    if(wcscmp(this2->name,L"print")==0)
		anna_stack_print_trace(function->stack_template);
*/
	    return (anna_node_t *)this2;
	}	

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_get_t * result = (anna_node_member_get_t *)this;	    
	    result->object = anna_node_prepare(result->object, function, &list);
	    return this;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t * result = (anna_node_assign_t *)this;	    
	    result->value = anna_node_prepare(result->value, function, &list);
	    return this;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_set_t * result = (anna_node_member_set_t *)this;	    
	    result->value = anna_node_prepare(result->value, function, &list);
	    result->object = anna_node_prepare(result->object, function, &list);
	    return this;
	}


	case ANNA_NODE_IMPORT:
	{
	    anna_node_import_t * result = (anna_node_import_t *)this;	    
	    int import_ok = 0;
	    
	    if(result->payload->node_type == ANNA_NODE_CALL)
	    {
		anna_node_call_t *call = (anna_node_call_t *)result->payload;
		if(check_node_identifier_name(call->function, L"__memberGet__"))
		{
		    if(call->child_count == 2)
		    {
			if((call->child[0]->node_type == ANNA_NODE_IDENTIFIER) &&
			   (call->child[1]->node_type == ANNA_NODE_IDENTIFIER))
			{
			    anna_node_identifier_t *module_id = 
				(anna_node_identifier_t *)call->child[0];
			    anna_node_identifier_t *field = 
				(anna_node_identifier_t *)call->child[1];
			    anna_function_t *module = anna_module_load(module_id->name);
			    anna_prepare_function_interface(module);
			    anna_object_t *item = anna_stack_get_str(module->stack_template,
								     field->name);
			    if(item)
			    {
				anna_stack_declare(
				    function->stack_template,
				    field->name,
				    item->type,
				    item);
				import_ok = 1;
			    }			    
			}			
		    }		    
		}		
	    }
	    else if(result->payload->node_type == ANNA_NODE_IDENTIFIER)
	    {
		anna_node_identifier_t *module_id = 
		    (anna_node_identifier_t *)result->payload;
		anna_function_t *module = anna_module_load(module_id->name);
		anna_node_import_data data = 
		    {
			module->stack_template,
			function->stack_template
		    };
		
		hash_foreach2(
		    &module->stack_template->member_string_identifier, 
		    &anna_node_import_item, &data);
		import_ok = 1;		
	    }

	    
	    if(!import_ok)
	    {
		anna_error(this, L"Invalid import");
	    }
	    
	    return anna_node_create_null(0);
	}


	case ANNA_NODE_TRAMPOLINE:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_BLOB:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_NULL:
	    return this;   

	default:
	    wprintf(L"Unknown node of type %d during node preparation\n", this->node_type);
	    CRASH;
    }
}

void anna_node_prepare_children(anna_node_call_t *in, anna_function_t *func, anna_node_list_t *parent)
{
    int i;
    for(i=0; i< in->child_count; i++)
	in->child[i] = anna_node_prepare(in->child[i], func, parent);
}

void anna_node_prepare_child(anna_node_call_t *in, int idx, anna_function_t *func, anna_node_list_t *parent)
{
    in->child[idx] = anna_node_prepare(in->child[idx], func, parent);
}

