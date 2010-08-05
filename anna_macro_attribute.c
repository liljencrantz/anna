


static anna_node_t *anna_macro_template_attribute(anna_node_call_t *node, 
						  anna_function_t *function, 
						  anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node, L"template instantiation", 2);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_CALL);

    anna_node_call_t *attribute = 
        (anna_node_call_t *)node->child[0];
    anna_node_t *body = node->child[1];
    
    CHECK_NODE_TYPE(attribute->function, ANNA_NODE_IDENTIFIER);
    CHECK_CHILD_COUNT(attribute, L"template instantiation", 1);	
    CHECK_NODE_IDENTIFIER_NAME(attribute->function, L"template");
    CHECK_NODE_TYPE(attribute->child[0], ANNA_NODE_CALL);
    /*
    anna_node_identifier_t *id = 
	(anna_node_identifier_t *)attribute->function;
    */
    anna_node_call_t *pair = 
	(anna_node_call_t *)attribute->child[0];
    
    CHECK_NODE_IDENTIFIER_NAME(pair->function, L"Pair");
    CHECK_CHILD_COUNT(pair, L"template instantiation", 2);
    CHECK_NODE_TYPE(pair->child[0], ANNA_NODE_IDENTIFIER);
    
    return anna_node_replace(
	body,
	(anna_node_identifier_t *)pair->child[0],
	(anna_node_t *)pair->child[1]);    
/*
  wprintf(L"Result\n");
  anna_node_print(node);
  wprintf(L"\n");    
*/
}

static anna_node_t *anna_macro_extends_attribute(anna_node_call_t *node, 
						  anna_function_t *function, 
						  anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node, L"template instantiation", 2);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_CALL);
    
    anna_node_call_t *attribute =
        (anna_node_call_t *)node->child[0];
    anna_node_call_t *body = 
	(anna_node_call_t *)
	node->child[1];
    
    //anna_node_print(attribute);

    anna_node_prepare_children(attribute, function, parent);
    CHECK_CHILD_COUNT(attribute, L"template instantiation", 1);
    CHECK_NODE_TYPE(attribute->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_TYPE(body->child[3], ANNA_NODE_CALL);

    anna_node_identifier_t *parent_id = 
	(anna_node_identifier_t *)attribute->child[0];
    
    anna_object_t *wrapper = anna_stack_get_str(function->stack_template, parent_id->name);
    if(!wrapper)
    {
	FAIL(parent_id, L"Unknown parent type: %ls", parent_id->name);
    }
    
    anna_type_t *type = anna_type_unwrap(wrapper);
    if(!type)
    {
	FAIL(parent_id, L"Unknown parent type: %ls", parent_id->name);
    }
    
    anna_node_call_t *definition = (anna_node_call_t *)type->definition->child[3];
    
    int i;
    for(i=0; i<definition->child_count; i++)
    {
	anna_node_call_add_child(
	    (anna_node_call_t *)body->child[3],
	    anna_node_clone_deep(definition->child[i]));
    }
    
    return (anna_node_t *)body;
    
}
