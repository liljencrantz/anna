//ROOT: src/node/node.c

int check_node_identifier_name(anna_node_t *n,
			       wchar_t *name)
{
    if(anna_node_is_named(n, name))
    {
	return 1;
    }    
    anna_error(
	n,
	L"Unexpected argument. Expected an identifier named \"%ls\".", name);
    return 0;
}

int check_node_block(anna_node_t *n)
{
    if(anna_node_is_call_to(n, L"__block__"))
    {
	return 1;
    }    
    anna_error(
	n,
	L"Unexpected argument type. Expected a block definition, got a node of type %d.", 
	n->node_type);
    return 0;
}
