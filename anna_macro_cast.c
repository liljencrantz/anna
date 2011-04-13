
static anna_node_t *anna_macro_cast_i(
    anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"cast", 2);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    node->function = anna_node_create_null(&node->location);
    
    node->node_type = ANNA_NODE_CAST;
    return (anna_node_t *)node;
}

ANNA_VM_MACRO(anna_macro_cast)
