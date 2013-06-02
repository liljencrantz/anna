//ROOT: src/node/node.c

static void anna_node_hash_func_step(
    anna_node_t *this, void *aux)
{
    int *res = (int *)aux;
    *res ^= this->node_type;
    int contrib = 0;
    
    switch(this->node_type)
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_NOTHING:
	case ANNA_NODE_SPECIALIZE:
	{
	    anna_node_call_t *i = (anna_node_call_t *)this;
	    contrib = anna_hash((int *)&i->child_count, 1);
	    break;
	}
	
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *i = (anna_node_identifier_t *)this;
	    contrib = anna_hash((int *)i->name, wcslen(i->name)*sizeof(wchar_t)/sizeof(int));
	    break;
	}

	case ANNA_NODE_INT_LITERAL:
	{
	    anna_node_int_literal_t *i = (anna_node_int_literal_t *)this;
	    int payload = (int)mpz_get_si(i->payload);
	    contrib = anna_hash(&payload, 1);
	    break;
	}

	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *i = (anna_node_string_literal_t *)this;
	    contrib = (int)i->payload_size;
	    break;
	}
	
	case ANNA_NODE_CHAR_LITERAL:
	{
	    anna_node_char_literal_t *i = (anna_node_char_literal_t *)this;
	    contrib = anna_hash((int *)&i->payload, sizeof(wchar_t)/sizeof(int));
	    break;
	}
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_node_float_literal_t *i = (anna_node_float_literal_t *)this;
	    union int_double
	    {
		int i;
		double d;
	    }
	    ;
	    contrib = anna_hash((int *)&i->payload, sizeof(double)/sizeof(int));
	    break;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *i = (anna_node_closure_t *)this;
	    contrib = anna_hash(i->payload->name, wcslen(i->payload->name)*sizeof(wchar_t)/sizeof(int)) + i->payload->input_count;
	    break;
	}
	
	case ANNA_NODE_WHILE:
	case ANNA_NODE_IF:
	case ANNA_NODE_AND:
	case ANNA_NODE_OR:
	case ANNA_NODE_MAPPING:
	{
	    break;
	}
	/*
	default:
	{
	    anna_error(this, L"Can't calculate hash code for specified node type %d", this->node_type);
	}
	*/
    }
    
    *res ^= contrib ^ (contrib << 5);
    *res = (*res << 3) | (*res >> 29);
    
}

int anna_node_hash_func( void *data )
{
    int res = 0xDEADBEEF;
    anna_node_each(
	(anna_node_t *)data,
	anna_node_hash_func_step, &res);
    return res;
}

int anna_node_hash_cmp( 
    void *a,
    void *b )
{
    anna_node_t *na = (anna_node_t *)a;
    anna_node_t *nb = (anna_node_t *)b;
    return anna_node_compare(na, nb) == 0;
}
