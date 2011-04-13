
static anna_node_t *anna_macro_or_i(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"if macro", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_OR,
	    node->child[0],
	    node->child[1]);
}
ANNA_VM_MACRO(anna_macro_or)


static anna_node_t *anna_macro_and_i(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"if macro", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_AND,
	    node->child[0],
	    node->child[1]);
}
ANNA_VM_MACRO(anna_macro_and)

static anna_node_t *anna_macro_if_i(anna_node_call_t *node)
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
ANNA_VM_MACRO(anna_macro_if)

static anna_node_t *anna_macro_while_i(anna_node_call_t *node)
{
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_WHILE,
	    node->child[0],
	    node->child[1]);
}
ANNA_VM_MACRO(anna_macro_while)

