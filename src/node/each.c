
typedef struct
{
    int node_type;
    array_list_t *al;
}
    anna_node_find_each_t;

static anna_node_t *anna_node_each_fun(
    anna_node_t *this, void *aux)
{
    void **aux2 = (void **)aux;
    anna_node_function_t fun = aux2[1];
    fun(this, aux2[0]);
    return this;
}

void anna_node_each(
    anna_node_t *this, anna_node_function_t fun, void *aux)
{
    void **aux2 = malloc(sizeof(void *)*2);
    aux2[0] = aux;
    aux2[1] = fun;
    anna_node_each_replace(this, &anna_node_each_fun, aux2);
    free(aux2);
}

anna_node_t *anna_node_each_replace(
    anna_node_t *this, anna_node_replace_function_t fun, void *aux)
{
    if(!this)
    {
	CRASH;
    }
    this = fun(this, aux);
    switch(this->node_type)
    {

	case ANNA_NODE_CALL:
	case ANNA_NODE_SPECIALIZE:
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CAST:
	case ANNA_NODE_MEMBER_CALL:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    n->function = anna_node_each_replace(n->function, fun, aux);
	    int i;
	    for(i=0; i<n->child_count; i++)
	    {
		if(!n->child[i])
		{
		    anna_error(n, L"Invalid child %d of %d\n", i+1, n->child_count);
		    CRASH;
		}
		
		n->child[i] = anna_node_each_replace(n->child[i], fun, aux);
	    }
	    
	    break;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *n = (anna_node_assign_t *)this;
	    n->value = anna_node_each_replace(n->value, fun, aux);
	    break;   
	}

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_BIND:
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *n = (anna_node_member_access_t *)this;
	    n->object = anna_node_each_replace(n->object, fun, aux);
	    if(this->node_type == ANNA_NODE_MEMBER_SET)
	    {
		n->value = anna_node_each_replace(n->value, fun, aux);
	    }
	    break;   
	}

	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	{
	    anna_node_declare_t *n = (anna_node_declare_t *)this;
	    n->type = anna_node_each_replace(n->type, fun, aux);
	    n->value = anna_node_each_replace(n->value, fun, aux);
	    break;   
	}

	case ANNA_NODE_WHILE:
	case ANNA_NODE_OR:
	case ANNA_NODE_AND:
	case ANNA_NODE_MAPPING:
	{
	    anna_node_cond_t *n = (anna_node_cond_t *)this;
	    n->arg1 = anna_node_each_replace(n->arg1, fun, aux);
	    n->arg2 = anna_node_each_replace(n->arg2, fun, aux);
	    break;   
	}
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *c = (anna_node_if_t *)this;
	    c->cond = anna_node_each_replace(c->cond, fun, aux);
	    c->block1 = (anna_node_call_t *)anna_node_each_replace(
		(anna_node_t *)c->block1, fun, aux);
	    c->block2 = (anna_node_call_t *)anna_node_each_replace(
		(anna_node_t *)c->block2, fun, aux);
	    break;
	}	
	
	case ANNA_NODE_RETURN:
	case ANNA_NODE_BREAK:
	case ANNA_NODE_CONTINUE:
	case ANNA_NODE_RETURN_TYPE_OF:
	case ANNA_NODE_TYPE_OF:
	case ANNA_NODE_INPUT_TYPE_OF:
	case ANNA_NODE_USE:
	{
	    anna_node_wrapper_t *n = (anna_node_wrapper_t *)this;
	    n->payload = anna_node_each_replace(n->payload, fun, aux);
	    break;
	}

	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_INTERNAL_IDENTIFIER:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_CLOSURE:
	case ANNA_NODE_TYPE:
	{
	    break;   
	}
	
	default:
	    wprintf(
		L"OOPS! Unknown node type when iterating over AST: %d\n", 
		this->node_type);
	    CRASH;
    }
    return this;
}

static void anna_node_find_each(anna_node_t *node, void *aux)
{
    anna_node_find_each_t *data = (anna_node_find_each_t *)aux;
    if(node->node_type == data->node_type)
    {
	al_push(data->al, node);
    }
}

void anna_node_find(anna_node_t *this, int node_type, array_list_t *al)
{
    anna_node_find_each_t data = 
	{
	    node_type, al
	}
    ;
    anna_node_each(this, &anna_node_find_each, &data);
}

