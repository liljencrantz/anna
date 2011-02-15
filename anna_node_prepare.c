
#define ANNA_NODE_TYPE_IN_TRANSIT ((anna_type_t *)1)

typedef struct
{
    anna_stack_frame_t *src;
    anna_stack_frame_t *dst;
}
anna_node_import_data;
/*
static void anna_node_import_item(
    void *key_ptr,
    void *val_ptr,
    void *aux_ptr)
{
    wchar_t *name = (wchar_t *)key_ptr;
    size_t *offset=(size_t *)val_ptr;
    anna_node_import_data *data = 
	(anna_node_import_data *)aux_ptr;
    
    if(data->src->member_flags[*offset])
    {
//	wprintf(L"Import: Skipping private member %ls\n", name);
	return;
    }
    //wprintf(L"Import: Importing public member %ls\n", name);
    
    anna_object_t *item =
	anna_stack_get_str(
	    data->src,
	    name);
    anna_stack_declare(
	data->dst,
	name,
	item->type,
	item,
	ANNA_STACK_PRIVATE);
}
*/

 /*
static anna_object_t *anna_node_constructor_template(
    anna_object_t *type_object,
    anna_node_call_t *node, 
    anna_function_t *function,
    anna_node_list_t *parent)
{
//    wprintf(L"Check function call for template\n");
//    anna_node_print(node);
    return type_object;    
}
 */
anna_node_t *anna_node_macro_expand(
    anna_node_t *this,
    anna_stack_frame_t *stack)
{
/*
    wprintf(L"EXPAND\n");
    anna_node_print(this);
*/  
    switch( this->node_type )
    {
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *this2 =(anna_node_call_t *)this;
	    
	    if(this2->function->node_type == ANNA_NODE_CALL)
	    {
		anna_node_call_t *c = (anna_node_call_t *)this2->function;
		if(c->function->node_type == ANNA_NODE_IDENTIFIER && c->child_count==2)
		{
		    anna_node_identifier_t *mgfun = (anna_node_identifier_t *)c->function;
		    if(wcscmp(mgfun->name, L"__memberGet__")==0 && c->child[1]->node_type == ANNA_NODE_IDENTIFIER)
		    {
			anna_node_identifier_t *fun = (anna_node_identifier_t *)c->child[1];
			anna_object_t **stack_object_ptr = anna_stack_addr_get_str(stack, fun->name);
			if(stack_object_ptr)
			{
			    anna_function_t *fun = anna_function_unwrap(*stack_object_ptr);
			    if( fun && (fun->flags & ANNA_FUNCTION_MACRO))
			    {	
				anna_node_t *res = anna_macro_invoke(fun, this2);
				res = anna_node_macro_expand(res, stack);
				return res;
			    }
			}
		    }
		}
	    }
	    
	    this2->function = anna_node_macro_expand(this2->function, stack);
	    
	    if(this2->function->node_type == ANNA_NODE_IDENTIFIER)
	    {
		anna_node_identifier_t *fun = (anna_node_identifier_t *)this2->function;
		anna_object_t **stack_object_ptr = anna_stack_addr_get_str(stack, fun->name);
//		anna_stack_print(stack);
		
		if(stack_object_ptr)
		{
		    anna_function_t *fun2 = anna_function_unwrap(*stack_object_ptr);
		    if( fun2 && (fun2->flags & ANNA_FUNCTION_MACRO))
		    {
			anna_node_t *res = anna_macro_invoke(fun2, this2);
			res = anna_node_macro_expand(res, stack);
			
			return res;
		    }
		}		
	    }

	    int i;
	    for(i=0;i<this2->child_count;i++)
	    {
		this2->child[i] = anna_node_macro_expand(this2->child[i], stack);
	    }
	    
	    if(this2->function->node_type == ANNA_NODE_MEMBER_GET)
	    {
		anna_node_member_get_t *mg = (anna_node_member_get_t *)this2->function;
		
		anna_node_t *result = (anna_node_t *)anna_node_create_member_call(
		    &this2->location,
		    mg->object,
		    mg->mid,
		    this2->child_count,
		    this2->child);
		return result;
		
	    }

	    return this;
	}
	
	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	{
	    return this;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;
	    anna_function_t *f = c->payload;
	    
	    if(f->body)
	    {
		int i;
		for(i=0;i<f->body->child_count; i++)
		    f->body->child[i] = anna_node_macro_expand(f->body->child[i], stack);
	    }
	    return this;
	}

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_get_t *g = (anna_node_member_get_t *)this;
	    g->object = anna_node_macro_expand(g->object, stack);
	    return this;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_set_t *g = (anna_node_member_set_t *)this;
	    g->object = anna_node_macro_expand(g->object, stack);
	    g->value = anna_node_macro_expand(g->value, stack);
	    return this;
	}

	case ANNA_NODE_DECLARE:
	{
	    anna_node_declare_t *d = (anna_node_declare_t *)this;
	    d->type = anna_node_macro_expand(d->type, stack);
	    d->value = anna_node_macro_expand(d->value, stack);
	    return this;
	}	
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *d = (anna_node_assign_t *)this;
	    d->value = anna_node_macro_expand(d->value, stack);
	    return this;
	}	
	
	case ANNA_NODE_AND:
	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *c = (anna_node_cond_t *)this;
	    c->arg1 = anna_node_macro_expand(c->arg1, stack);
	    c->arg2 = anna_node_macro_expand(c->arg2, stack);
	    return this;
	}	
	
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *c = (anna_node_if_t *)this;
	    c->cond = anna_node_macro_expand(c->cond, stack);
	    c->block1 = (anna_node_call_t *)anna_node_macro_expand((anna_node_t *)c->block1, stack);
	    c->block2 = (anna_node_call_t *)anna_node_macro_expand((anna_node_t *)c->block2, stack);
	    return this;
	}	
	
	default:
	{
	    anna_error(
		this,
		L"Invalid node of type %d during macro expansion", this->node_type);	    
	}
    }

    return this;
}

int anna_node_is_call_to(anna_node_t *this, wchar_t *name){
    if( this->node_type == ANNA_NODE_CALL)
    {
	anna_node_call_t *this2 = (anna_node_call_t *)this;
	if( this2->function->node_type == ANNA_NODE_IDENTIFIER) 
	{
	    anna_node_identifier_t *fun = (anna_node_identifier_t *)this2->function;
	    return wcscmp(fun->name, name)==0;
	}	
    }
    return 0;
}



void anna_node_calculate_type_param(
    size_t argc,
    anna_node_t **argv,
    int is_method,
    anna_function_type_key_t *funt)
{
    int i, j;
    for(i=0; i<argc; i++)
    {
	if(argv[i]->node_type == ANNA_NODE_CLOSURE)
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)argv[i];
	    anna_function_t *closure = c->payload;
	    wprintf(L"Closure as param %d. function type says argument is of type %ls\n", i, funt->argv[i+!!is_method]->name);
	    anna_function_type_key_t *template = anna_function_type_extract(funt->argv[i+!!is_method]);
	    assert(template);
	    wprintf(L"Closure template takes %d params\n", template->argc);
	    for(j=0; j<template->argc; j++)
	    {
		wprintf(L"Argument %d should be of type %ls\n", j, template->argv[j]->name);
		anna_function_argument_hint(
		    closure,
		    j,
		    template->argv[j]);
	    }
	}
	
    }
}

anna_stack_frame_t *anna_node_register_declarations(
    anna_node_t *this,
    size_t extra)
{
    array_list_t decls = 
	{
	    0,0,0
	}
    ;
    anna_node_find(this, ANNA_NODE_DECLARE, &decls);
    size_t sz = al_get_count(&decls);
    anna_stack_frame_t *stack = anna_stack_create(sz+extra, 0);    
    int i;
/*
    wprintf(L"WOO WEE WOO %d declarations in ast\n", sz);
    anna_node_print(this);
*/  
    for(i=0; i<sz; i++)
    {
	anna_node_declare_t *decl = al_get(&decls, i);
	
	anna_stack_declare2(
	    stack,
	    decl);
	anna_sid_t sid = anna_stack_sid_create(stack, decl->name);
	decl->sid = sid;
    }
    //stack_freeze(stack);
    return stack;
}

static void anna_node_calculate_type_internal(
    anna_node_t *this,
    anna_stack_frame_t *stack)
{
    switch(this->node_type)
    {
	
	case ANNA_NODE_INT_LITERAL:
	{
	    this->return_type = int_type;
	    break;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    this->return_type = string_type;
	    break;
	}
	
	case ANNA_NODE_CHAR_LITERAL:
	{
	    this->return_type = char_type;
	    break;
	}
	
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    this->return_type = float_type;
	    break;
	}

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *id = (anna_node_identifier_t *)this;
	    anna_type_t *t = anna_stack_get_type(stack, id->name);
	    
	    if(!t)
	    {
		anna_node_declare_t *decl = anna_stack_get_declaration(stack, id->name);
		if(decl)
		{
		    anna_node_calculate_type((anna_node_t *)decl,stack);
		    if(decl->return_type && decl->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
		    {
			anna_stack_set_type(stack, id->name, decl->return_type);
			t = decl->return_type;
		    }
		}
		
	    }
	    
	    if(!t || t == ANNA_NODE_TYPE_IN_TRANSIT){
		anna_error(this, L"Unknown identifier: %ls", id->name);
//		anna_stack_print(stack);
	    }
	    else
	    {
		this->return_type = t;
	    }
	    
	    break;
	}
	
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *call = (anna_node_call_t *)this;
	    anna_node_calculate_type(call->function, stack);
	    anna_type_t *fun_type = call->function->return_type;

	    if(fun_type == type_type)
	    {
		if(call->function->node_type == ANNA_NODE_IDENTIFIER)
		{
		    anna_node_identifier_t *id = (anna_node_identifier_t *)call->function;
		    anna_object_t *wrapper = anna_stack_get_str(stack, id->name);
		    if(wrapper != 0)
		    {
			anna_type_t *type = anna_type_unwrap(wrapper);
			if(type)
			{
			    this->node_type = ANNA_NODE_CONSTRUCT;
			    call->function = (anna_node_t *)anna_node_create_dummy(
				&call->function->location,
				wrapper,
				0);		
			    call->return_type = type;
			    break;
			}
		    }
		}
	    }

	    if(fun_type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }

	    anna_function_type_key_t *funt = anna_function_type_extract(fun_type);
	    if(!funt)
	    {
		anna_error(this, L"Value is not callable");
		anna_type_print(fun_type);		
		break;
	    }
	    
	    if(funt->flags & ANNA_FUNCTION_MACRO)
	    {
		anna_error(this, L"Unexpanded macro call");
		break;
		
	    }
	    
	    call->return_type = funt->result;
	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	{	    
	    anna_node_member_call_t *n = (anna_node_member_call_t *)this;
	    
	    anna_node_calculate_type(n->object, stack);
	    anna_type_t *type = n->object->return_type;
	    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }

	    anna_member_t *member = anna_member_get(type, n->mid);
	    
	    if(!member)
	    {
		anna_type_t **types = malloc(sizeof(anna_type_t *)*n->child_count);
		int i;
		int ok = 1;
		
		for(i=0; i<n->child_count; i++)
		{
		    anna_node_calculate_type(n->child[i], stack);
		    types[i] = n->child[i]->return_type;
		    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
		    {
			ok = 0;
			break;
		    }
		}
		if(ok)
		{
		    member = anna_member_method_search(
			type, n->mid, n->child_count, types);
		    
		    if(member)
		    {
			n->mid = anna_mid_get(member->name);
		    }
		}
		
	    }
	    
	    if(!member)
	    {
		anna_error(
		    this, 
		    L"No member named %ls in type %ls\n", 
		    anna_mid_get_reverse(n->mid),
		    type->name);
		break;
	    }

	    anna_function_type_key_t *funt = anna_function_type_extract(member->type);
	    n->return_type = funt->result;

	    anna_node_calculate_type_param(n->child_count, n->child, 1, funt);
	    
	    break;
	}
	
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;
//	    wprintf(L"AAA1 %ls\n", c->payload->name);
	    
	    anna_function_setup_interface(c->payload, stack);
//	    wprintf(L"AAA2 %ls\n", c->payload->name);
	    if(c->payload->wrapper)
	    {
		c->return_type = c->payload->wrapper->type;
	    }
//	    wprintf(L"AAA3 %ls\n", c->payload->name);
//	    anna_function_setup_body(c->payload, stack);
//	    wprintf(L"AAA4 %ls\n", c->payload->name);
	    
	    break;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *c = (anna_node_assign_t *)this;
	    anna_node_calculate_type(c->value, stack);
	    c->return_type = c->value->return_type;
	    break;
	}

	case ANNA_NODE_NULL:
	{
	    this->return_type = null_type;
	    break;
	}

	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_get_t *c = (anna_node_member_get_t *)this;
	    anna_node_calculate_type(c->object, stack);
	    anna_type_t *type = c->object->return_type;
	    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }
	    
	    anna_member_t *member = anna_member_get(type, c->mid);
	    if(!member)
	    {
		anna_error(
		    this, 
		    L"No member named %ls in type %ls\n", 
		    anna_mid_get_reverse(c->mid),
		    type->name);
		break;
		
	    }
	    
	    if(member->is_method)
	    {
		c->node_type = ANNA_NODE_MEMBER_GET_WRAP;
	    }
	    c->return_type = member->type;
	    break;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_set_t *g = (anna_node_member_set_t *)this;
	    anna_node_calculate_type(g->value, stack);
	    g->return_type = g->value->return_type;
	    break;
	}

	case ANNA_NODE_DECLARE:
	{
	    anna_node_declare_t *d = (anna_node_declare_t *)this;
	    if(d->type->node_type == ANNA_NODE_IDENTIFIER)
	    {
		anna_node_identifier_t *t = node_cast_identifier(d->type);
		anna_object_t *t2 = anna_stack_get_str(stack, t->name);	    
		d->return_type = anna_type_unwrap(t2);
	    }
	    else if(d->type->node_type == ANNA_NODE_NULL)
	    {
		if(d->value->node_type == ANNA_NODE_NULL)
		{
		    anna_error(this, L"No type specified for variable declaration");
		}
		anna_node_calculate_type(d->value, stack);
		d->return_type = d->value->return_type;
	    }
	    else
	    {
		anna_error(d->type, L"Invalid type for declaration");
	    }
	    
	    break;
	}
	
	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_node_calculate_type(d->arg1, stack);
	    anna_node_calculate_type(d->arg2, stack);
	    if((d->arg1->return_type == ANNA_NODE_TYPE_IN_TRANSIT) ||
	       (d->arg2->return_type == ANNA_NODE_TYPE_IN_TRANSIT))
	    {
		break;
	    }
	    
	    d->return_type = anna_type_intersect(
		d->arg1->return_type,
		d->arg2->return_type);
	    
	    break;   
	}

	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_node_calculate_type(d->arg2, stack);
	    d->return_type = d->arg2->return_type;	    
	    break;   
	}
	
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *d = (anna_node_if_t *)this;

	    anna_node_calculate_type((anna_node_t *)d->block1, stack);
	    anna_node_calculate_type((anna_node_t *)d->block2, stack);
	    if((d->block1->return_type == ANNA_NODE_TYPE_IN_TRANSIT) ||
	       (d->block2->return_type == ANNA_NODE_TYPE_IN_TRANSIT))
	    {
		break;
	    }
	    
	    d->return_type = anna_type_intersect(
		d->block1->return_type,
		d->block2->return_type);
	    
	    break;
	}	
	
	case ANNA_NODE_DUMMY:
	{
	    anna_node_dummy_t *d = (anna_node_dummy_t *)this;
	    d->return_type = d->payload->type;
	    break;   
	}
	
	default:
	{
	    anna_error(
		this,
		L"Don't know how to handle node of type %d during type calculation", this->node_type);	    
	    break;
	}
    }
}

void anna_node_prepare_body(
    anna_node_t *this)
{
    switch(this->node_type)
    {
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;    
	    anna_function_setup_body(c->payload);
	    break;
	}
    }
}


void anna_node_calculate_type(
    anna_node_t *this,
    anna_stack_frame_t *stack)
{
    if(this->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
    {
	anna_error(this, L"Circular type checking dependency");
	anna_node_print(this);
    }
    else if(this->return_type == 0)
    {
	this->return_type = ANNA_NODE_TYPE_IN_TRANSIT;
	anna_node_calculate_type_internal( this, stack );
    }
}



