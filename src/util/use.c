//ROOT: src/util/util.c

anna_use_t *anna_use_create_stack(struct anna_stack_template *stack)
{
    anna_use_t *res = malloc(sizeof(anna_use_t));
    res->type = anna_stack_wrap(stack)->type;
    res->node = (anna_node_t *)anna_node_create_dummy(0, anna_stack_wrap(stack));
    return res;
}

anna_use_t *anna_use_create_node(anna_node_t *node, struct anna_type *type)
{
    anna_use_t *res = malloc(sizeof(anna_use_t));
    res->type = type;
    res->node = node;
    return res;
}


