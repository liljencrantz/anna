

static anna_node_t *anna_macro_cast(
    anna_node_call_t *node, 
    anna_function_t *function, 
    anna_node_list_t *parent)
{
    return (anna_node_t *)anna_node_call_create(
	&node->location,
	(anna_node_t *)anna_node_identifier_create(
	    &node->function->location,
	    L"List"), 
	node->child_count,
	node->child);
}

static anna_node_t *anna_macro_is(
    anna_node_call_t *node, 
    anna_function_t *function, 
    anna_node_list_t *parent)
{
    return (anna_node_t *)anna_node_call_create(
	&node->location,
	(anna_node_t *)anna_node_identifier_create(
	    &node->function->location,
	    L"List"), 
	node->child_count,
	node->child);
}

static anna_node_t *anna_macro_as(
    anna_node_call_t *node, 
    anna_function_t *function, 
    anna_node_list_t *parent)
{
    return (anna_node_t *)anna_node_call_create(
	&node->location,
	(anna_node_t *)anna_node_identifier_create(
	    &node->function->location,
	    L"List"), 
	node->child_count,
	node->child);
}

