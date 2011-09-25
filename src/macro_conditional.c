
ANNA_VM_MACRO(anna_macro_or)
{
    CHECK_CHILD_COUNT(node,L"or expression", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_OR,
	    node->child[0],
	    node->child[1]);
}

ANNA_VM_MACRO(anna_macro_and)
{
    CHECK_CHILD_COUNT(node,L"and expression", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_AND,
	    node->child[0],
	    node->child[1]);
}

ANNA_VM_MACRO(anna_macro_if)
{
    if(node->child_count < 2 || node->child_count > 3)
    {
	anna_error((anna_node_t *)node, L"Invalid parameter count");
	return anna_node_create_null(&node->location);
    }
    CHECK_NODE_BLOCK(node->child[1]);
    if(node->child_count == 2)
    {
	return (anna_node_t *)
	    anna_node_create_if(
		&node->location, 
		node->child[0],
		(anna_node_call_t *)node->child[1],
		anna_node_create_block2(&node->location));
    }
    else
    {
	CHECK_NODE_BLOCK(node->child[2]);
	anna_node_if_t *res = anna_node_create_if(
		&node->location, 
		node->child[0],
		(anna_node_call_t *)node->child[1],
		(anna_node_call_t *)node->child[2]);
	res->has_else = 1;
	return (anna_node_t *)res;
    }
}

ANNA_VM_MACRO(anna_macro_else)
{
    CHECK_CHILD_COUNT(node,L"else clause", 1);
    CHECK_NODE_BLOCK(node->child[0]);
    node->flags |= ANNA_NODE_MERGE;
    node->flags |= ANNA_NODE_DONT_EXPAND;
    return (anna_node_t *)node;
}

ANNA_VM_MACRO(anna_macro_while)
{
    CHECK_CHILD_COUNT(node,L"while loop expression", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    anna_node_call_t *block = (anna_node_call_t *) node->child[1];
    block->function = (anna_node_t *)anna_node_create_identifier(&block->function->location, L"__loopBlock__");
    
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_WHILE,
	    node->child[0],
	    node->child[1]);
}

