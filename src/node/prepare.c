//ROOT: src/node/node.c
#include "anna/object.h"

static anna_node_t *resolve_identifiers_each(
    anna_node_t *this, void *aux);

static anna_type_t *anna_method_curry(anna_function_type_t *fun)
{
    anna_function_t *res = anna_native_create(
	anna_util_identifier_generate(L"!anonymousFunction", 0),
	fun->flags,
	(anna_native_t)anna_vm_null_function,
	fun->return_type,
	fun->input_count-1,
	fun->input_type+1,
	fun->input_name+1,
	fun->input_default+1,
	0);
    
    return res->wrapper->type;
}


static void anna_node_set_stack_fun(anna_node_t *node, void *stack_ptr)
{
//    anna_message(L"Set stack %d for node %d of type %d\n", stack_ptr, node, node->node_type);
    node->stack = (anna_stack_template_t *)stack_ptr;
    switch(node->node_type)
    {
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)node;
//	    anna_message(L"\nSet stack %d for closure %ls\n", stack_ptr, c->payload->name);
	    anna_function_set_stack(
		c->payload,
		(anna_stack_template_t *)stack_ptr);
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)node;
//	    anna_message(L"\nSet stack %d for type %ls\n", stack_ptr, c->payload->name);
	    anna_type_set_stack(
		c->payload,
		(anna_stack_template_t *)stack_ptr);	    
	    break;
	}
    }
}

void anna_node_set_stack(
    anna_node_t *this,
    anna_stack_template_t *stack)
{
    anna_node_each(
	this, anna_node_set_stack_fun, stack);
}

void anna_node_register_declarations(
    anna_node_t *this,
    anna_stack_template_t *stack)
{
    array_list_t decls = AL_STATIC;

    anna_node_find(this, ANNA_NODE_DECLARE, &decls);
    anna_node_find(this, ANNA_NODE_CONST, &decls);
    size_t sz = al_get_count(&decls);
    int i;
    
    for(i=0; i<sz; i++)
    {
	anna_node_declare_t *decl = al_get(&decls, i);
	anna_stack_declare2(
	    stack,
	    decl);
    }
    al_destroy(&decls);
}

void anna_method_search(
    anna_type_t *type, 
    wchar_t *alias,
    array_list_t *use_memb,
    int reverse)
{
    int i;
    
    array_list_t *memb_list = &type->member_list;
    for(i=0; i<al_get_count(memb_list); i++)
    {
	anna_member_t *memb = al_get_fast(memb_list, i);
	int is_candidate = !reverse && (wcscmp(memb->name, alias) == 0);

	if(!is_candidate && memb->attribute)
	{
	    if(reverse)
	    {
		is_candidate = anna_attribute_has_alias_reverse(
		    memb->attribute,
		    alias);		
	    }
	    else
	    {
		is_candidate = anna_attribute_has_alias(
		    memb->attribute,
		    alias);
	    }
	}

	if(is_candidate)
	{
	    int callable = 0;
	    
	    anna_type_prepare_member(type, anna_mid_get(memb->name));

	    if(memb->type == type_type)
	    {
		callable = 1;
	    }
	    else
	    {
		if(!memb->type || memb->type == ANNA_NODE_TYPE_IN_TRANSIT)
		{
		    anna_error(0, L"Circular type checking dependency on member %ls::%ls\n", type->name, memb->name);
		    continue;
		}
		
		if(anna_function_type_unwrap(memb->type))
		{
		    callable = 1;
		}
	    }
	    
	    if(callable)
	    {
		al_push(use_memb, memb);
	    }
	    
	}
    }
}


static void anna_function_search_internal(
    anna_stack_template_t *stack, wchar_t *alias, array_list_t *stack_decl, array_list_t *use_memb)
{
    if(!stack)
    {
	return;
    }
    
    int i, j;
    for(i=0; i<stack->count; i++)
    {
	anna_node_declare_t *decl = stack->member_declare_node[i];
	if(
	    decl && 
	    (
		(
		    decl->attribute && 
		    anna_attribute_has_alias(
			decl->attribute,
			alias)) ||
		(
		    wcscmp(decl->name, alias) == 0)))
	{
	    stack->member_declare_node[i] = decl = 
		(anna_node_declare_t *)anna_node_calculate_type(
		    (anna_node_t *)decl);
	    if(decl->return_type && (decl->return_type != ANNA_NODE_TYPE_IN_TRANSIT))
	    {
		if(anna_function_type_unwrap(decl->return_type))
		{
		    al_push(stack_decl, decl);
		}
	    }
	}
    }

    anna_function_search_internal(
	stack->parent, alias, stack_decl, use_memb);
    
    array_list_t tmp = AL_STATIC;
    for(j=0; j<al_get_count(&stack->import); j++)
    {

	anna_use_t *use = al_get(&stack->import, j);
	anna_method_search(
	    use->type, alias, &tmp, 0);
	for(i=0; i<al_get_count(&tmp); i++)
	{
	    al_push(use_memb, use);
	    al_push(use_memb, al_get(&tmp, i));	    
	}
	al_truncate(&tmp, 0);
    }
    al_destroy(&tmp);
}

static wchar_t *anna_function_search(
    anna_stack_template_t *stack, wchar_t *alias, anna_node_call_t *call)
{
    wchar_t *res = 0;
    array_list_t stack_decl = AL_STATIC;
    array_list_t use_memb = AL_STATIC;
    anna_function_search_internal(
	stack, alias, &stack_decl, &use_memb);
    int i;
    size_t count=0;
    
    if(!anna_node_calculate_type_direct_children(call, call->stack))
    {
	return alias;
    }    

    if(al_get_count(&stack_decl) || al_get_count(&use_memb))
    {
	anna_function_type_t **ft = 
	    malloc(sizeof(anna_function_type_t *)*(al_get_count(&stack_decl) + al_get_count(&use_memb)));
	wchar_t **name = 
	    malloc(sizeof(wchar_t *)*(al_get_count(&stack_decl)+al_get_count(&use_memb)));
	
	for(i=0; i<al_get_count(&stack_decl); i++)
	{
	    anna_node_declare_t *decl = (anna_node_declare_t *)al_get(&stack_decl, i);
	    if(anna_stack_get_declaration(stack, decl->name) == decl)
	    {
		name[count] = decl->name;
		ft[count++] = anna_function_type_unwrap(decl->return_type);
	    }
	}
	while(al_get_count(&use_memb))
	{
	    anna_member_t *memb = (anna_member_t *)al_pop(&use_memb);
	    anna_use_t *use = (anna_use_t *)al_pop(&use_memb);
	    
	    if(anna_stack_search_use(stack, memb->name) == use)
	    {
		name[count] = memb->name;
		ft[count++] = anna_member_bound_function_type(memb);
	    }
	}
	
	
	if(count)
	{
	    int idx = anna_abides_search(
		call, ft, count);
	    if(idx != -1)
	    {
		res = name[idx];
	    }
	}
	
	free(ft);
	free(name);	
    }
    
    al_destroy(&stack_decl);
    al_destroy(&use_memb);
    return res;
}

static void anna_node_prepare_body(
    anna_node_t *this, void *aux)
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

static anna_node_t *anna_node_calculate_type_fun(
    anna_node_t *this,
    void *aux)
{
    return anna_node_calculate_type(this);
}

void anna_node_calculate_type_children(anna_node_t *node)
{
    node = anna_node_each_replace(node, &anna_node_calculate_type_fun, 0);
    anna_node_each(
	node, 
	&anna_node_prepare_body, 0);
}

static anna_node_t *resolve_identifiers_each(
    anna_node_t *this, void *aux)
{
    if(this->node_type != ANNA_NODE_IDENTIFIER)
    {
	return this;
    }
    anna_node_identifier_t *id = (anna_node_identifier_t *)this;
    
    anna_use_t *use = anna_stack_search_use(
	id->stack,
	id->name);
    
    if(use)
    {
	anna_node_t *src_node = anna_node_clone_deep(use->node);
	anna_node_set_stack(src_node, this->stack);
	src_node = anna_node_calculate_type(src_node);
	anna_node_t *res = (anna_node_t *)anna_node_create_member_get(
	    &id->location,
	    ANNA_NODE_MEMBER_GET,
	    src_node,
	    anna_mid_get(id->name));
	anna_node_set_stack(res, this->stack);
	return res;
    }
    return this;    
}

void anna_node_resolve_identifiers(
    anna_node_t *this)
{
    anna_node_each_replace(
	this, resolve_identifiers_each, 0);
}
