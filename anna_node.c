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
#include "anna_mid.h"
#include "anna_use.h"
#include "anna_stack.h"

#include "anna_node_specialize.c"
#include "anna_node_macro_expand.c"
#include "anna_node_prepare.c"
#include "anna_node_validate.c"
#include "anna_node_each.c"

anna_node_t *anna_node_type_lookup_get_payload(anna_node_t *node)
{
    return ((anna_node_wrapper_t *)node)->payload;
}

anna_type_t *anna_node_resolve_to_type(anna_node_t *node, anna_stack_template_t *stack)
{
    if((node->node_type == ANNA_NODE_TYPE_OF) || 
       (node->node_type == ANNA_NODE_RETURN_TYPE_OF) || 
       (node->node_type == ANNA_NODE_INPUT_TYPE_OF) )
    {
	anna_node_calculate_type(node);
	return node->return_type;
    }    
    
    anna_entry_t *eres = anna_node_static_invoke_try(
	node, stack);
    
    if(eres)
    {
	anna_object_t *res = anna_as_obj(eres);
	return (res->type == type_type) ? anna_type_unwrap(res) : res->type;
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
	anna_error(node, L"Expected a call node, got node of type %d", node->node_type);
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

static anna_entry_t *anna_node_assign_invoke(anna_node_assign_t *this, anna_stack_template_t *stack)
{
    anna_entry_t *result = anna_node_static_invoke(this->value, stack);
    anna_stack_set(stack, this->name, result);
    return result;
}

anna_type_t *anna_node_get_return_type(anna_node_t *this, anna_stack_template_t *stack)
{
    return 0;
}

anna_entry_t *anna_node_static_invoke_try(
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
	    return anna_from_obj(node->payload);
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *node = (anna_node_closure_t *)this;	    
	    anna_node_calculate_type(this);
	    if(node->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
		return anna_from_obj(node->payload->wrapper);
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    return anna_from_obj(anna_type_wrap(f));
	}

	case ANNA_NODE_INT_LITERAL:
	    return anna_from_obj(anna_int_create_mp(((anna_node_int_literal_t *)this)->payload));
	    
	case ANNA_NODE_FLOAT_LITERAL:
	    return anna_from_obj(anna_float_create(((anna_node_float_literal_t *)this)->payload));
	    
	case ANNA_NODE_STRING_LITERAL:
	    return anna_from_obj(anna_string_create(
		((anna_node_string_literal_t *)this)->payload_size, 
		((anna_node_string_literal_t *)this)->payload));
	    
	case ANNA_NODE_CHAR_LITERAL:
	    return anna_from_obj(anna_char_create(((anna_node_char_literal_t *)this)->payload));

	case ANNA_NODE_NULL:
	    return null_entry;

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
	    //fwprintf(stderr, L"Weee identifier %ls found. Frame? %ls\n", this2->name, frame?L"yes": L"no");
	    if(frame)
	    {
		//fwprintf(stderr, L"Frame is namespace. Readonly? %ls\n", (anna_stack_get_flag(frame, this2->name) & ANNA_STACK_READONLY)?L"yes":L"no");

		anna_node_declare_t *decl = anna_stack_get_declaration(frame, this2->name);
		if(decl)
		{
		    anna_node_calculate_type((anna_node_t *)decl);
		}
		
		if(anna_stack_get_flag(frame, this2->name) & ANNA_STACK_READONLY)
		{
		    //fwprintf(stderr, L"Identifier %ls is a constant\n", this2->name);
		    anna_object_t *res = anna_as_obj(anna_stack_get(frame, this2->name));
		    return anna_function_unwrap(res)?0:anna_from_obj(res);
		}
	    }
	    //fwprintf(stderr, L"Identifier lookup failed\n");
	    break;
	}
	
	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_access_t *this2 = (anna_node_member_access_t *)this;
	    //wprintf(L"Weee member get. Member is named %ls\n", anna_mid_get_reverse(this2->mid));
	    anna_object_t *obj = anna_as_obj(
		anna_node_static_invoke_try(
		    this2->object,
		    stack));
	    if(obj)
	    {
		anna_member_t *memb = anna_member_get(obj->type, this2->mid);
		if(!memb)
		{
		    return 0;
		}
		
		if(memb->is_property)
		{
		    return 0;
		}
		
		//wprintf(L"Weee member found object\n");
		return *anna_entry_get_addr(obj, this2->mid);
	    }
	    else
	    {
		//wprintf(L"Member not constant\n");		
	    }
	}
    }
    return 0;
}

anna_entry_t *anna_node_static_invoke(
    anna_node_t *this, 
    anna_stack_template_t *stack)
{
    anna_entry_t *res = anna_node_static_invoke_try(this, stack);
    if(!res)
    {
	anna_error(
	    this,L"Code could not be invoked at compile time\n");
	return null_entry;
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
	case ANNA_NODE_SPECIALIZE:
	case ANNA_NODE_CAST:
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
	case ANNA_NODE_MEMBER_SET:
	    return sizeof(anna_node_member_access_t);
	    
	case ANNA_NODE_RETURN:
	case ANNA_NODE_BREAK:
	case ANNA_NODE_CONTINUE:
	case ANNA_NODE_TYPE_OF:
	case ANNA_NODE_INPUT_TYPE_OF:
	case ANNA_NODE_RETURN_TYPE_OF:
	case ANNA_NODE_USE:
	    return sizeof(anna_node_wrapper_t);
	    
	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	    return sizeof(anna_node_declare_t);

	case ANNA_NODE_OR:
	case ANNA_NODE_AND:
	case ANNA_NODE_WHILE:
	case ANNA_NODE_MAPPING:
	    return sizeof(anna_node_cond_t);

	case ANNA_NODE_IF:
	    return sizeof(anna_node_if_t);

	case ANNA_NODE_TYPE:
	    return sizeof(anna_node_type_t);

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
    
    if( (n->node_type == ANNA_NODE_CALL) || 
	(n->node_type == ANNA_NODE_CONSTRUCT) || 
	(n->node_type == ANNA_NODE_MEMBER_CALL))
    {
	anna_node_call_t *r2=(anna_node_call_t *)r;
	anna_node_call_t *n2=(anna_node_call_t *)n;
	anna_node_call_dealias(r2, n2);
    }
    
    anna_alloc_gc_unblock();
    return r;
}

static anna_node_t *anna_node_clone_deep_each(
    anna_node_t *this, void *aux)
{
    return anna_node_clone_shallow(this);    
}

anna_node_t *anna_node_clone_deep(anna_node_t *n)
{
    return anna_node_each_replace(
	n, anna_node_clone_deep_each, 0);
}

static anna_node_t *anna_node_replace_each(
    anna_node_t *this, void *aux)
{
    anna_node_t **aux2 = (anna_node_t **)aux;
    
    if(this->node_type == ANNA_NODE_INTERNAL_IDENTIFIER)
    {
	anna_node_identifier_t *from = aux2[0];
	anna_node_t *to = aux2[1];
	anna_node_identifier_t *this2 = (anna_node_identifier_t *)this;
	if(wcscmp(this2->name,from->name)==0)
	{
	    anna_node_t *res = anna_node_clone_deep(to);
	    return res;
	}
    }
    
    return this;
}

anna_node_t *anna_node_replace(anna_node_t *tree, anna_node_identifier_t *from, anna_node_t *to)
{
    anna_node_t **aux = malloc(sizeof(anna_node_t *)*2);
    aux[0] = from;
    aux[1] = to;
    
    anna_node_t *res = anna_node_each_replace(
	tree, anna_node_replace_each, aux);
    free(aux);
    return res;    
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

	case ANNA_NODE_USE:
	{
	    anna_node_wrapper_t *n1 = (anna_node_use_t *)node1;
	    anna_node_wrapper_t *n2 = (anna_node_use_t *)node2;
	    return anna_node_compare(n1->payload, n2->payload);
	}
	
	default:
	    wprintf(L"OOPS! Unknown node type when comparing: %d\n", node1->node_type);
	    CRASH;
    }
}

