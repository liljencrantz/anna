static void free_val(void *key, void *value)
{
    free(value);
}

static void anna_alloc_free(void *obj)
{
    
    switch(*((int *)obj) & ANNA_ALLOC_MASK)
    {
	case ANNA_OBJECT:
	{
	    anna_object_t *o = (anna_object_t *)obj;
	    int i;
	    for(i=0; i<o->type->finalizer_count; i++)
	    {
		o->type->finalizer[i](o);
	    }
	    anna_alloc_tot -= o->type->object_size;
	    anna_slab_free(obj, o->type->object_size);

	    break;
	}
	case ANNA_TYPE:
	{
	    int i;
	    anna_type_t *o = (anna_type_t *)obj;
	    
//	    anna_message(L"Discarding unused type %ls %d\n", o->name, o);
	    
	    if(obj != null_type)
	    {
		for(i=0; i<anna_type_get_member_count(o); i++)
		{
		    anna_slab_free(
			anna_type_get_member_idx(o, i),
			sizeof(anna_member_t));
		}
	    }
	    
	    free(o->static_member);
	    free(o->mid_identifier);
	    
	    anna_alloc_tot -= sizeof(anna_type_t);
	    anna_slab_free(obj, sizeof(anna_type_t));
	    break;
	}
	case ANNA_ACTIVATION_FRAME:
	{
	    anna_activation_frame_t *o = (anna_activation_frame_t *)obj;
	    anna_alloc_tot -= o->function->frame_size;
//	    anna_message(L"FRAMED %ls\n", o->function->name);
	    
	    anna_slab_free(o, o->function->frame_size);
	    break;
	}
	case ANNA_FUNCTION:
	{
	    anna_function_t *o = (anna_function_t *)obj;
	    free(o->code);
	    free(o->input_type);
	    anna_alloc_tot -= sizeof(anna_function_t);
	    anna_slab_free(obj, sizeof(anna_function_t));
	    break;
	}

	case ANNA_NODE:
	{
	    anna_node_t *o = (anna_node_t *)obj;
//	    o->flags |= ANNA_NODE_FREED;
	    
	    switch(o->node_type)
	    {
		case ANNA_NODE_CALL:
		case ANNA_NODE_CONSTRUCT:
		case ANNA_NODE_MEMBER_CALL:
		case ANNA_NODE_STATIC_MEMBER_CALL:
	        case ANNA_NODE_SPECIALIZE:
		case ANNA_NODE_CAST:
		{
		    anna_node_call_t *n = (anna_node_call_t *)o;
		    free(n->child);
		    break;
		}
		case ANNA_NODE_INT_LITERAL:
		{
		    anna_node_int_literal_t *n = (anna_node_int_literal_t *)o;
		    mpz_clear(n->payload);
		    break;
		}
		case ANNA_NODE_STRING_LITERAL:
		{
		    anna_node_string_literal_t *n = (anna_node_string_literal_t *)o;
		    if(n->free)
		    {
			free(n->payload);
		    }
		    break;
		}
	    }
	    free(obj);
	    break;
	}

	case ANNA_STACK_TEMPLATE:
	{
	    int i;
	    anna_stack_template_t *o = (anna_stack_template_t *)obj;
	    free(o->member_declare_node);
	    free(o->member_flags);
	    for(i=0; i<al_get_count(&o->import); i++)
	    {
		free(al_get(&o->import, i));
	    }
	    al_destroy(&o->import);
	    for(i=0; i<al_get_count(&o->expand); i++)
	    {
		free(al_get(&o->expand, i));
	    }
	    al_destroy(&o->expand);
	    hash_foreach(&o->member_string_identifier, free_val);
	    hash_destroy(&o->member_string_identifier);
//	    anna_alloc_count -= sizeof(anna_stack_template_t);
	    anna_slab_free(obj, sizeof(anna_stack_template_t));
	    break;
	}
	case ANNA_BLOB:
	{
//	    anna_message(L"DA BLOB\n");
	    
	    int *blob = (int *)obj;
	    size_t sz = blob[1];
	    anna_alloc_tot -= sz;
	    anna_slab_free(obj, sz);
	    break;
	}
	default:
	{
	    break;
	}
    }
}

