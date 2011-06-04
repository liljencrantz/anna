
static inline anna_node_t *anna_macro_or_i(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"or expression", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_OR,
	    node->child[0],
	    node->child[1]);
}
ANNA_VM_MACRO(anna_macro_or)


static inline anna_node_t *anna_macro_and_i(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"and expression", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_AND,
	    node->child[0],
	    node->child[1]);
}
ANNA_VM_MACRO(anna_macro_and)

static inline anna_node_t *anna_macro_if_i(anna_node_call_t *node)
{

    CHECK_CHILD_COUNT(node,L"if expression", 3);
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

static inline anna_node_t *anna_macro_while_i(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"while loop expression", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    anna_node_call_t *block = (anna_node_call_t *) node->child[1];
    block->function = anna_node_create_identifier(&block->function->location, L"__loopBlock__");
    
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_WHILE,
	    node->child[0],
	    node->child[1]);
}
ANNA_VM_MACRO(anna_macro_while)

