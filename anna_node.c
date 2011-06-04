#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "common.h"
#include "wutil.h"
#include "anna_node.h"
#include "anna_int.h"
#include "anna_float.h"
#include "anna_string.h"
#include "anna_char.h"
#include "anna_function.h"
#include "anna_type.h"
#include "anna_node_create.h"
#include "anna_member.h"
#include "anna_function_type.h"
#include "anna_list.h"
#include "anna_hash.h"
#include "anna_alloc.h"
#include "anna_pair.h"
#include "anna_util.h"
#include "anna_vm.h"

typedef struct
{
    int node_type;
    array_list_t *al;
}
    anna_node_find_each_t;



#include "anna_node_specialize.c"
#include "anna_node_macro_expand.c"
#include "anna_node_prepare.c"

anna_node_t *anna_node_type_lookup_get_payload(anna_node_t *node)
{
    anna_node_wrapper_t *d = (anna_node_wrapper_t *)node;
    return d->payload;
}

anna_type_t *anna_node_resolve_to_type(anna_node_t *node, anna_stack_template_t *stack)
{
    debug(D_SPAM,L"Figure out type from:\n");    
    anna_node_print(D_SPAM, node);
    
    if((node->node_type == ANNA_NODE_TYPE_OF) || 
       (node->node_type == ANNA_NODE_RETURN_TYPE_OF) || 
       (node->node_type == ANNA_NODE_INPUT_TYPE_OF) )
    {
	anna_node_calculate_type(node);
	return node->return_type;
    }
    
    anna_object_t *res = anna_node_static_invoke_try(
	node, stack);
    
    if(!res)
    {
	return 0;
    }
    if(res->type == type_type)
    {
	return anna_type_unwrap(res);
    }
    else
    {
	return res->type;
    }
    return 0;
}

int anna_node_is_call_to(anna_node_t *this, wchar_t *name){
    if( this->node_type == ANNA_NODE_CALL)
    {
	anna_node_call_t *this2 = (anna_node_call_t *)this;
	return anna_node_is_named(this2->function, name);
    }
    return 0;
}

int anna_node_is_named(anna_node_t *this, wchar_t *name){
    if( ( this->node_type == ANNA_NODE_IDENTIFIER) ||
	( this->node_type == ANNA_NODE_INTERNAL_IDENTIFIER))
    {
	anna_node_identifier_t *fun = (anna_node_identifier_t *)this;
	return wcscmp(fun->name, name)==0;
    }
    return 0;
}

void anna_node_set_location(anna_node_t *node, anna_location_t *l)
{
    if(l)    
    {
	memcpy(&node->location, l, sizeof(anna_location_t));
    }
    else
    {
	memset(&node->location, 0, sizeof(anna_location_t));	
    }
}

anna_node_call_t *node_cast_call(anna_node_t *node) 
{
    if(node->node_type!=ANNA_NODE_CALL)
    {
	anna_error(node, L"Expected a call node");
	CRASH;
    }
    
    return (anna_node_call_t *)node;
}

anna_node_identifier_t *node_cast_identifier(anna_node_t *node) 
{
    if(node->node_type!=ANNA_NODE_IDENTIFIER)
    {
	anna_error(node, L"Expected an identifier node, got node of type %d", node->node_type);
	CRASH;
    }
    return (anna_node_identifier_t *)node;
}

anna_node_int_literal_t *node_cast_int_literal(anna_node_t *node) 
{
    assert(node->node_type==ANNA_NODE_INT_LITERAL);
    return (anna_node_int_literal_t *)node;
}

anna_node_string_literal_t *node_cast_string_literal(anna_node_t *node) 
{
    assert(node->node_type==ANNA_NODE_STRING_LITERAL);
    return (anna_node_string_literal_t *)node;
}

void anna_node_call_add_child(anna_node_call_t *call, anna_node_t *child)
{
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 8 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(anna_node_t *));
	if(!call->child) 
	{
	    wprintf(L"Out of memory\n");
	    CRASH;
	}	
	call->child_capacity = new_capacity;
    }
//    wprintf(L"LALALA %d %d\n", call->child_capacity, call->child_count);
    
    call->child[call->child_count++] = child;
}

void anna_node_call_prepend_child(anna_node_call_t *call, anna_node_t *child)
{
    assert(call->node_type == ANNA_NODE_CALL);
    
    if(call->child_count==0) 
    {
	anna_node_call_add_child(call, child);
	return;
    }
    
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 8 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(anna_node_t *));
	if(!call->child) 
	{
	    wprintf(L"Out of memory\n");
	    CRASH;
	}	
	call->child_capacity = new_capacity;
    }
    memmove(&call->child[1], &call->child[0], sizeof(anna_node_t *)*call->child_count);
    
    call->child[0] = child;
    call->child_count++;
}

void anna_node_call_set_function(anna_node_call_t *call, anna_node_t *function)
{
    call->function = function;
}

static anna_object_t *anna_node_assign_invoke(anna_node_assign_t *this, anna_stack_template_t *stack)
{
    anna_object_t *result = anna_node_static_invoke(this->value, stack);
    anna_stack_set(stack, this->name, result);
    return result;
}

anna_type_t *anna_node_get_return_type(anna_node_t *this, anna_stack_template_t *stack)
{
    return 0;
}

anna_object_t *anna_node_static_invoke_try(
    anna_node_t *this, 
    anna_stack_template_t *stack)
{
    if(!this)
    {
	wprintf(L"Critical: Invoke null node\n");
	CRASH;
    }
    
    switch(this->node_type)
    {
	case ANNA_NODE_DUMMY:
	{
	    anna_node_dummy_t *node = (anna_node_dummy_t *)this;
	    return node->payload;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *node = (anna_node_closure_t *)this;	    
	    anna_node_calculate_type(this);
	    if(node->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
		return node->payload->wrapper;
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    return anna_type_wrap(f);
	}

	case ANNA_NODE_INT_LITERAL:
	    return anna_int_create_mp(((anna_node_int_literal_t *)this)->payload);
	    
	case ANNA_NODE_FLOAT_LITERAL:
	    return anna_float_create(((anna_node_float_literal_t *)this)->payload);
	    
	case ANNA_NODE_STRING_LITERAL:
	    return anna_string_create(
		((anna_node_string_literal_t *)this)->payload_size, 
		((anna_node_string_literal_t *)this)->payload);
	    
	case ANNA_NODE_CHAR_LITERAL:
	    return anna_char_create(((anna_node_char_literal_t *)this)->payload);

	case ANNA_NODE_NULL:
	    return null_object;

	case ANNA_NODE_CONST:
	case ANNA_NODE_DECLARE:
	    return anna_node_assign_invoke((anna_node_assign_t *)this, stack);

	case ANNA_NODE_SPECIALIZE:
	{
	    anna_node_specialize((anna_node_call_t *)this, stack);
	    if(this->node_type == ANNA_NODE_DUMMY)
	    {
		return anna_node_static_invoke_try(this, stack);
	    }
	    return 0;
	}
	
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *this2 = (anna_node_identifier_t *)this;
	    anna_stack_template_t *frame = anna_stack_template_search(stack, this2->name);
//	    fwprintf(stderr, L"Weee identifier %ls found. Frame? %ls\n", this2->name, frame?L"yes": L"no");
	    if(frame)
	    {
//		fwprintf(stderr, L"Frame is namespace. Readonly? %ls\n", (anna_stack_get_flag(frame, this2->name) & ANNA_STACK_READONLY)?L"yes":L"no");

		anna_node_declare_t *decl = anna_stack_get_declaration(frame, this2->name);
		if(decl)
		{
		    anna_node_calculate_type((anna_node_t *)decl);
		}
		
		if(anna_stack_get_flag(frame, this2->name) & ANNA_STACK_READONLY)
		{
//		    fwprintf(stderr, L"Identifier %ls is a constant\n", this2->name);
		    anna_object_t *res = anna_stack_get(frame, this2->name);
		    return anna_function_unwrap(res)?0:res;
		}
	    }
//	    fwprintf(stderr, L"Identifier lookup failed\n");
	    break;
	}
	
	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_access_t *this2 = (anna_node_member_access_t *)this;
	    //wprintf(L"Weee member get. Member is named %ls\n", anna_mid_get_reverse(this2->mid));
	    anna_object_t *obj = anna_node_static_invoke_try(
		this2->object,
		stack);
	    if(obj)
	    {
		anna_member_t *memb = anna_member_get(obj->type, this2->mid);
		if(memb->is_property)
		{
		    return 0;
		}
		
		//wprintf(L"Weee member found object\n");
		return anna_as_obj(*anna_entry_get_addr(obj, this2->mid));
	    }
	    else
	    {
		//wprintf(L"Member not constant\n");		
	    }
	    
	}
    }
    return 0;
}

anna_object_t *anna_node_static_invoke(
    anna_node_t *this, 
    anna_stack_template_t *stack)
{
    anna_object_t *res = anna_node_static_invoke_try(this, stack);
    if(!res)
    {
	anna_error(
	    this,L"Code could not be invoked at compile time\n");
	return null_object;
    }
    return res;
}


static size_t anna_node_size(anna_node_t *n)
{
    switch(n->node_type)
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_MEMBER_CALL:
	case ANNA_NODE_CONSTRUCT:
	    return sizeof(anna_node_call_t);
	    
	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_INTERNAL_IDENTIFIER:
	    return sizeof(anna_node_identifier_t);

	case ANNA_NODE_INT_LITERAL:
	    return sizeof(anna_node_int_literal_t);
	case ANNA_NODE_STRING_LITERAL:
	    return sizeof(anna_node_string_literal_t);
	case ANNA_NODE_CHAR_LITERAL:
	    return sizeof(anna_node_char_literal_t);
	case ANNA_NODE_FLOAT_LITERAL:
	    return sizeof(anna_node_float_literal_t);
	case ANNA_NODE_NULL:
	    return sizeof(anna_node_t);
	case ANNA_NODE_DUMMY:
	    return sizeof(anna_node_dummy_t);
	case ANNA_NODE_CLOSURE:
	    return sizeof(anna_node_closure_t);
	case ANNA_NODE_ASSIGN:
	    return sizeof(anna_node_assign_t);
	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_BIND:
	    return sizeof(anna_node_member_access_t);
	case ANNA_NODE_MEMBER_SET:
	    return sizeof(anna_node_member_access_t);
	case ANNA_NODE_RETURN:
	case ANNA_NODE_TYPE_OF:
	case ANNA_NODE_INPUT_TYPE_OF:
	case ANNA_NODE_RETURN_TYPE_OF:
	    return sizeof(anna_node_wrapper_t);
	default:
	    anna_error(n, L"Unknown node type %d encoundered while determining size of node\n", n->node_type);
	    CRASH;
    }    
}

static void anna_node_call_dealias(anna_node_call_t *dest, anna_node_call_t *src)
{
    dest->child = malloc(sizeof(anna_node_t *)*src->child_capacity);
    memcpy(dest->child, src->child, sizeof(anna_node_t *)*dest->child_count);
}

anna_node_t *anna_node_clone_shallow(anna_node_t *n)
{
    size_t sz = anna_node_size(n);
    anna_alloc_gc_block();
    anna_node_t *r = anna_alloc_node(sz);
    memcpy(r,n,sz);
    r->wrapper=0;
    ANNA_UNPREPARED(r);

    switch(n->node_type)
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_call_t *r2=(anna_node_call_t *)r;
	    anna_node_call_t *n2=(anna_node_call_t *)n;
	    anna_node_call_dealias(r2, n2);
	    break;
	}
    }
    
    anna_alloc_gc_unblock();
    return r;
}

anna_node_t *anna_node_clone_deep(anna_node_t *n)
{
    
    switch(n->node_type)
    {
	/*
	  Clone a call node. This invlolves a shallow clone of the
	  node as well as calling the deep clone of every child and
	  the function node.
	*/
	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_call_t *r = (anna_node_call_t *)anna_node_clone_shallow(n);
	    int i;
	    
	    if(!r->function)
	    {
		anna_error(n, L"Call node has invalid function");
		CRASH;
	    }

	    r->function = anna_node_clone_deep(r->function);

	    for(i=0;i<r->child_count; i++)
	    {
		r->child[i] = anna_node_clone_deep(r->child[i]);
	    }
	    return (anna_node_t *)r;
	}
	
	/*
	  These nodes are not mutable and they have no child nodes, so
	  we can return them as is.
	*/
	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_INTERNAL_IDENTIFIER:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_CLOSURE:
	    return anna_node_clone_shallow(n);
	    
	    /*
	      These nodes are not yet handled, but that should be
	      perfectly ok for now, since they are only ever created
	      by the prepare system, and cloning a prepared AST is
	      never supported or possible.
	    */
	case ANNA_NODE_ASSIGN:
	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_BIND:
	case ANNA_NODE_MEMBER_SET:
	case ANNA_NODE_RETURN:
	default:
	    anna_error(n, L"Unsupported node type %d for deep copy!\n", n->node_type);
	    CRASH;
	
    }
}

anna_node_t *anna_node_replace(anna_node_t *tree, anna_node_identifier_t *from, anna_node_t *to)
{
    switch(tree->node_type)
    {
	case ANNA_NODE_INTERNAL_IDENTIFIER:
	{
	    anna_node_identifier_t *tree2 = (anna_node_identifier_t *)tree;
	    if(wcscmp(tree2->name,from->name)==0)
	    {
		anna_node_t *res = anna_node_clone_deep(to);
		return res;
	    }
	    return tree;
	}

	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_CLOSURE:
	case ANNA_NODE_RETURN_TYPE_OF:
	case ANNA_NODE_TYPE_OF:
	case ANNA_NODE_INPUT_TYPE_OF:
	{
	    return tree;
	}

	case ANNA_NODE_RETURN:
	{
	    anna_node_wrapper_t *this2 =(anna_node_wrapper_t *)anna_node_clone_shallow(tree);
	    this2->payload = anna_node_replace(this2->payload,
					       from, to);
	    return (anna_node_t *)this2;
	}

	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	{
	    int i;
	    anna_node_call_t *this2 =(anna_node_call_t *)anna_node_clone_shallow(tree);	    
	    this2->function = anna_node_replace(this2->function,
						from, to);
	    for(i=0;i<this2->child_count;i++)
	    {
		this2->child[i] = anna_node_replace(this2->child[i],
						    from, to);
	    }
	    return (anna_node_t *)this2;	    
	}
	
	default:
	    wprintf(L"OOPS! Unknown node type when replacing: %d\n", tree->node_type);
	    CRASH;
    }
    
}

int anna_node_compare(anna_node_t *node1, anna_node_t *node2)
{
    if(node1->node_type != node2->node_type)
    {
        return node1->node_type - node2->node_type;
    }
   
    switch(node1->node_type)
    {

	case ANNA_NODE_RETURN:
	{
	    anna_node_wrapper_t *this1 =(anna_node_wrapper_t *)node1;
	    anna_node_wrapper_t *this2 =(anna_node_wrapper_t *)node2;

	    return anna_node_compare(this1->payload, this2->payload);
	}

	case ANNA_NODE_INPUT_TYPE_OF:
	{
	    anna_node_wrapper_t *this1 =(anna_node_wrapper_t *)node1;
	    anna_node_wrapper_t *this2 =(anna_node_wrapper_t *)node2;
	    
	    anna_node_t *c1 = anna_node_type_lookup_get_payload(node1);
	    anna_node_t *c2 = anna_node_type_lookup_get_payload(node2);
	    if(this1->steps != this2->steps)
	    {
		return this1->steps - this2->steps;
	    }
	    return anna_node_compare(c1,c2);
	}

	case ANNA_NODE_TYPE_OF:
	case ANNA_NODE_RETURN_TYPE_OF:
	{
	    anna_node_t *c1 = anna_node_type_lookup_get_payload(node1);
	    anna_node_t *c2 = anna_node_type_lookup_get_payload(node2);
	    return anna_node_compare(c1,c2);
	}
	
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *id1 = (anna_node_identifier_t *)node1;
	    anna_node_identifier_t *id2 = (anna_node_identifier_t *)node2;
	    return wcscmp(id1->name, id2->name);
	}
	
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *n1 = (anna_node_call_t *)node1;
	    anna_node_call_t *n2 = (anna_node_call_t *)node2;
	    if(n1->child_count != n2->child_count)
		return n1->child_count - n2->child_count;
	    int ff = anna_node_compare(n1->function, n2->function);
	    
	    if(ff)
		return ff;
	    
	    int i;
	    for(i=0; i<n1->child_count;i++)
	    {
		int cf = anna_node_compare(n1->child[i], n2->child[i]);
		if(cf)
		    return cf;
	    }
	    return 0;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *n1 = (anna_node_closure_t *)node1;
	    anna_node_closure_t *n2 = (anna_node_closure_t *)node2;
	    return n1->payload - n2->payload;
	}
	
	case ANNA_NODE_INT_LITERAL:
	{
	    anna_node_int_literal_t *n1 = (anna_node_int_literal_t *)node1;
	    anna_node_int_literal_t *n2 = (anna_node_int_literal_t *)node2;
	    return mpz_cmp( n1->payload, n2->payload);
	}
	
	case ANNA_NODE_CHAR_LITERAL:
	{
	    anna_node_char_literal_t *n1 = (anna_node_char_literal_t *)node1;
	    anna_node_char_literal_t *n2 = (anna_node_char_literal_t *)node2;
	    return n1->payload - n2->payload;
	}
	
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_node_float_literal_t *n1 = (anna_node_float_literal_t *)node1;
	    anna_node_float_literal_t *n2 = (anna_node_float_literal_t *)node2;
	    if(n1->payload > n2->payload)
		return 1;
	    else if(n1->payload < n2->payload)
		return -1;
	    else
		return 0;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *n1 = (anna_node_string_literal_t *)node1;
	    anna_node_string_literal_t *n2 = (anna_node_string_literal_t *)node2;
	    if(n1->payload_size != n2->payload_size)
		return n1->payload_size - n2->payload_size;
	    return wcsncmp(n1->payload, n2->payload, n1->payload_size);   
	}
	
	case ANNA_NODE_OR:
	case ANNA_NODE_AND:
	case ANNA_NODE_MAPPING:
	{
	    anna_node_cond_t *n1 = (anna_node_cond_t *)node1;
	    anna_node_cond_t *n2 = (anna_node_cond_t *)node2;
	    int d1 = anna_node_compare(n1->arg1, n2->arg1);
	    return d1?d1:anna_node_compare(n1->arg2, n2->arg2);
	}
	
	default:
	    wprintf(L"OOPS! Unknown node type when comparing: %d\n", node1->node_type);
	    CRASH;
    }
}

void anna_node_each(anna_node_t *this, anna_node_function_t fun, void *aux)
{
    fun(this, aux);
    switch(this->node_type)
    {

	case ANNA_NODE_CALL:
	case ANNA_NODE_SPECIALIZE:
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CAST:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    anna_node_each(n->function, fun, aux);
	    int i;
	    for(i=0; i<n->child_count; i++)
		anna_node_each(n->child[i], fun, aux);
	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    anna_node_each(n->object, fun, aux);
	    int i;
	    for(i=0; i<n->child_count; i++)
		anna_node_each(n->child[i], fun, aux);
	    break;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *n = (anna_node_assign_t *)this;
	    anna_node_each(n->value, fun, aux);
	    break;   
	}

	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_access_t *n = (anna_node_member_access_t *)this;
	    anna_node_each(n->object, fun, aux);
	    break;   
	}

	case ANNA_NODE_MEMBER_BIND:
	{
	    anna_node_member_access_t *n = (anna_node_member_access_t *)this;
	    anna_node_each(n->object, fun, aux);
	    break;   
	}

	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	{
	    anna_node_declare_t *n = (anna_node_declare_t *)this;
	    anna_node_each(n->type, fun, aux);
	    anna_node_each(n->value, fun, aux);
	    break;   
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *n = (anna_node_member_access_t *)this;
	    anna_node_each(n->object, fun, aux);
	    anna_node_each(n->value, fun, aux);
	    break;   
	}

	case ANNA_NODE_WHILE:
	case ANNA_NODE_OR:
	case ANNA_NODE_AND:
	case ANNA_NODE_MAPPING:
	{
	    anna_node_cond_t *n = (anna_node_cond_t *)this;
	    anna_node_each(n->arg1, fun, aux);
	    anna_node_each(n->arg2, fun, aux);
	    break;   
	}
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *c = (anna_node_if_t *)this;
	    anna_node_each(c->cond, fun, aux);
	    anna_node_each((anna_node_t *)c->block1, fun, aux);
	    anna_node_each((anna_node_t *)c->block2, fun, aux);
	    break;
	}	
	
	case ANNA_NODE_RETURN:
	case ANNA_NODE_BREAK:
	case ANNA_NODE_CONTINUE:
	case ANNA_NODE_RETURN_TYPE_OF:
	case ANNA_NODE_TYPE_OF:
	case ANNA_NODE_INPUT_TYPE_OF:
	{
	    anna_node_each(((anna_node_wrapper_t *)this)->payload, fun, aux);
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
	    wprintf(L"OOPS! Unknown node type when iterating over AST: %d\n", this->node_type);
	    CRASH;
    }    
    
}

static int anna_node_f_get_index(anna_function_type_t *f, int is_method, wchar_t *name)
{
    int i;
    for(i=(!!is_method); i<f->input_count; i++)
    {
	if(wcscmp(name, f->input_name[i]) == 0)
	{
	    return i - !!is_method;
	}
    }
    return -1;
}

int anna_node_call_validate(
    anna_node_call_t *call, 
    anna_function_type_t *target, 
    int is_method, 
    int print_error)
{
    anna_type_t **param = target->input_type;
    wchar_t **param_name = target->input_name;
    int param_count = target->input_count;    
    int res=0;

    if(ANNA_IS_VARIADIC(target))
	return 1;
    
    if(is_method)
    {
	param++;
	param_name++;
	param_count--;
    }

    int i;
    int *set = calloc(sizeof(int), param_count);
    int has_named=0;

    if(param_count != call->child_count)
    {
	if(print_error)
	{
	    anna_error((anna_node_t *)call, L"Wrong number of parameters to function call. Got %d, expected %d.", call->child_count, param_count);
	    CRASH;
	}
	
	goto END;
    }
    for(i=0; i<param_count; i++)
    {
	int is_named = call->child[i]->node_type == ANNA_NODE_MAPPING;
	if(has_named && !is_named)
	{
	    if(print_error)
	    {
		anna_error(call->child[i], L"An anonymous parameter value can not follow after a named parameter value");
	    }
	    goto END;
	}
	int idx = i;
	if(is_named)
	{
	    anna_node_cond_t *p = (anna_node_cond_t *)call->child[i];
	    if(p->arg1->node_type != ANNA_NODE_INTERNAL_IDENTIFIER)
	    {
		if(print_error)
		{
		    anna_error(call->child[i], L"Invalid named parameter %d", p->arg1->node_type);
		}
		goto END;
	    }
	    anna_node_identifier_t *name = (anna_node_identifier_t *)p->arg1;	    
	    idx = anna_node_f_get_index(target, is_method, name->name);
	    if(idx < 0)
	    {
		if(print_error)
		{
		    anna_error(call->child[i], L"Invalid named parameter");
		}
		goto END;
	    }
	}
	set[idx]++;
    }

    for(i=0; i<param_count; i++)
    {
	if(set[i] > 1)
	{
	    if(print_error)
	    {
		anna_error((anna_node_t *)call, L"More than one value was provided for argument %d, %ls, in function call ", i+1, param_name[i]);
	    }
	    goto END;
	}
	else if(set[i] < 1)
	{
	    if(print_error)
	    {
		anna_error((anna_node_t *)call, L"No value was provided for argument %d, %ls, in function call ", i+1, param_name[i]);
	    }
	    goto END;	    
	}
	
    }
    res = 1;

  END:
    free(set);
    return res;
}


void anna_node_call_map(
    anna_node_call_t *call, 
    anna_function_type_t *target, 
    int is_method)
{
    if(ANNA_IS_VARIADIC(target))
	return;

    anna_type_t **param = target->input_type;
    int param_count = target->input_count;    
    
    if(is_method)
    {
	param++;
	param_count--;
    }

    int i;
    size_t order_sz = sizeof(anna_node_t *)* param_count;
    anna_node_t **order = calloc(1, order_sz);

    for(i=0; i<param_count; i++)
    {
	int is_named = call->child[i]->node_type == ANNA_NODE_MAPPING;
	if(is_named)
	{
	    anna_node_cond_t *p = (anna_node_cond_t *)call->child[i];
	    anna_node_identifier_t *name = (anna_node_identifier_t *)p->arg1;	    
	    int idx = anna_node_f_get_index(target, is_method, name->name);
	    order[idx] = p->arg2;
	}
	else
	{
	    order[i] = call->child[i];
	}
    }
    memcpy(call->child, order, order_sz);
    
    free(order);
    return;
    
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
