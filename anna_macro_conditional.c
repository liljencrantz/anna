
static anna_node_t *anna_macro_or(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"if macro", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_OR,
	    node->child[0],
	    node->child[1]);
}


static anna_node_t *anna_macro_and(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"if macro", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_AND,
	    node->child[0],
	    node->child[1]);
}

static anna_node_t *anna_macro_if(anna_node_call_t *node)
{

    CHECK_CHILD_COUNT(node,L"if macro", 3);
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_BLOCK(node->child[2]);
    
    return (anna_node_t *)
        anna_node_create_if(
	    &node->location, 
	    node->child[0],
	    (anna_node_call_t *)node->child[1],
	    (anna_node_call_t *)node->child[2]);
}

static anna_node_t *anna_macro_while(anna_node_call_t *node)
{
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_WHILE,
	    node->child[0],
	    node->child[1]);
}

