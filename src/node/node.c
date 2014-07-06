#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "anna/fallback.h"
#include "anna/base.h"
#include "anna/util.h"
#include "anna/common.h"
#include "anna/wutil.h"
#include "anna/node.h"
#include "anna/function.h"
#include "anna/type.h"
#include "anna/node_create.h"
#include "anna/member.h"
#include "anna/alloc.h"
#include "anna/misc.h"
#include "anna/vm.h"
#include "anna/mid.h"
#include "anna/use.h"
#include "anna/stack.h"
#include "anna/intern.h"
#include "anna/node_hash.h"
#include "anna/node_check.h"
#include "anna/module.h"
#include "anna/attribute.h"
#include "anna/function_type.h"

#include "anna/lib/lang/int.h"
#include "anna/lib/lang/float.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/lang/char.h"
#include "anna/lib/lang/pair.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/hash.h"

static int anna_node_calculate_type_direct_children(
    anna_node_call_t *n, anna_stack_template_t *stack);

#include "specialize.c"
#include "macro_expand.c"
#include "prepare.c"
#include "calculate_type.c"
#include "validate.c"
#include "each.c"
#include "hash.c"
#include "create.c"
#include "print.c"
#include "check.c"

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
    
    anna_entry_t eres = anna_node_static_invoke_try(
	node, stack);
    
    if(!anna_entry_null_ptr(eres))
    {
	anna_object_t *res = anna_as_obj(eres);
	if(res->type == type_type)
	{
	    return anna_type_unwrap(res);
	}
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
    if((node->node_type!=ANNA_NODE_CALL) &&
       (node->node_type!=ANNA_NODE_NOTHING))
    {
	anna_error(node, L"Expected a call node, got node of type %d", node->node_type);
	CRASH;
    }
    
    return (anna_node_call_t *)node;
}

anna_node_cond_t *node_cast_mapping(anna_node_t *node)
{
    if(node->node_type!=ANNA_NODE_MAPPING)
    {
	anna_error(node, L"Expected a mapping node, got node of type %d", node->node_type);
	CRASH;
    }
    return (anna_node_cond_t *)node;
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

void anna_node_call_push(anna_node_call_t *call, anna_node_t *child)
{
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 4 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(anna_node_t *));
	if(!call->child) 
	{
	    anna_message(L"Out of memory\n");
	    CRASH;
	}	
	call->child_capacity = new_capacity;
    }
//    anna_message(L"LALALA %d %d\n", call->child_capacity, call->child_count);
    
    call->child[call->child_count++] = child;
}

void anna_node_call_prepend_child(anna_node_call_t *call, anna_node_t *child)
{
    assert(call->node_type == ANNA_NODE_CALL);
    
    if(call->child_count==0) 
    {
	anna_node_call_push(call, child);
	return;
    }
    
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 8 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(anna_node_t *));
	if(!call->child) 
	{
	    anna_message(L"Out of memory\n");
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

anna_entry_t anna_node_static_invoke_try(
    anna_node_t *this, 
    anna_stack_template_t *stack)
{
    if(!this)
    {
	anna_message(L"Critical: Invoke null node\n");
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
	    return anna_from_obj(0);
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    return anna_from_obj(anna_type_wrap(f));
	}

	case ANNA_NODE_NOTHING:
	{
	    int i;
	    anna_node_call_t *this2 = (anna_node_call_t *)this;
	    anna_entry_t res = anna_from_obj(0);
	    for(i=0; i<this2->child_count; i++)
	    {
		res = anna_node_static_invoke_try(this2->child[i], stack);
		if(anna_entry_null_ptr(res))
		{
		    break;
		}
	    }
	    return res;
	}

	case ANNA_NODE_INT_LITERAL:
	{
	    mpz_t *mp = &((anna_node_int_literal_t *)this)->payload;
	    
	    if(mpz_sizeinbase(*mp, 2) < ANNA_SMALL_MAX_BIT)
	    {
		return anna_from_int(mpz_get_si(*mp));	
	    }
	    else
	    {
		return anna_from_obj(anna_int_create_mp(((anna_node_int_literal_t *)this)->payload));
	    }
	}
	
	case ANNA_NODE_FLOAT_LITERAL:
	    return anna_from_float(((anna_node_float_literal_t *)this)->payload);
	    
	case ANNA_NODE_STRING_LITERAL:
	    return anna_from_obj(anna_string_create(
		((anna_node_string_literal_t *)this)->payload_size, 
		((anna_node_string_literal_t *)this)->payload));
	    
	case ANNA_NODE_CHAR_LITERAL:
	    return anna_from_obj(anna_char_create(((anna_node_char_literal_t *)this)->payload));
	    
	case ANNA_NODE_NULL:
	    return null_entry;

	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_access_t *this2 = (anna_node_member_access_t *)this;
	    anna_object_t *obj = anna_as_obj(
		anna_node_static_invoke_try(
		    this2->object,
		    stack));
	    if(obj)
	    {
		anna_member_t *memb = anna_member_get(obj->type, this2->mid);
		if(!memb)
		{
		    return anna_from_obj(0);
		}
		
		if(anna_member_is_property(memb))
		{
		    return anna_from_obj(0);
		}
		
		return *anna_entry_get_addr(obj, this2->mid);
	    }
	    return anna_from_obj(0);

	}

	case ANNA_NODE_SPECIALIZE:
	{
	    anna_node_t *this2 = anna_node_calculate_type(this);
	    if(this2->return_type == 0 || this2->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		return anna_from_obj(0);
	    }
	    if(this2->node_type != ANNA_NODE_SPECIALIZE)
	    {
		return anna_node_static_invoke_try(this2, stack);
	    }
	    return anna_from_obj(0);
	}

	case ANNA_NODE_CALL:
	{
	    return anna_from_obj(0);
	}
    }
    this = anna_node_calculate_type(this);
    if(this->return_type == 0 || this->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
    {
	return anna_from_obj(0);
    }
    
    switch(this->node_type)
    {
	case ANNA_NODE_CONST:
	case ANNA_NODE_DECLARE:
	{    
	    anna_node_declare_t *this2 = (anna_node_declare_t *)this;	    
	    anna_entry_t type_val = anna_node_static_invoke_try(
		this2->type, 
		stack);
	    anna_type_t *decl_type = 0;
	    anna_entry_t result = anna_node_static_invoke_try(this2->value, stack);
	    if(anna_entry_null_ptr(type_val) || anna_entry_null_ptr(result))
	    {
		return anna_from_obj(0);
	    }
	    
	    if(!anna_entry_null(type_val))
	    {
		decl_type = anna_type_unwrap(anna_as_obj(type_val));
	    }
	    else if(!anna_entry_null_ptr(result))
	    {
		decl_type = anna_as_obj(result)->type;
	    }

	    if(decl_type)
	    {
		anna_stack_set_type(stack, this2->name, decl_type);
		anna_stack_set(stack, this2->name, result);
	    }

	    return result;
	}
	
	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *this2 = (anna_node_call_t *)this;
	    anna_entry_t res = anna_node_static_invoke_try(
		this2->child[0], stack);
	    anna_entry_t type_entry = anna_node_static_invoke_try(
		this2->child[1], stack);
	    if(!anna_entry_null_ptr(res) && !anna_entry_null(type_entry))
	    {
		anna_type_t *type = anna_type_unwrap(anna_as_obj(type_entry));
		if(type)
		{
		    anna_object_t *res_obj = anna_as_obj(res);
		    return (anna_abides(res_obj->type, type)) ? res : null_entry;
		}
	    }
	    return anna_from_obj(0);
	}	

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *this2 = (anna_node_identifier_t *)this;
	    return anna_stack_get_try(
		stack,
		this2->name);
	}
	
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_call_t *this2 = (anna_node_call_t *)this;
	    anna_object_t *obj = anna_as_obj(
		anna_node_static_invoke_try(
		    this2->object,
		    stack));
	    if(obj)
	    {
		anna_member_t *memb = anna_member_get(obj->type, this2->mid);
		if((!memb) || (!anna_member_is_bound(memb)))
		{
		    break;
		}
		anna_function_t *meth = anna_function_unwrap(anna_as_obj(obj->type->static_member[memb->offset]));
		if(!(meth->flags & ANNA_FUNCTION_PURE))
		{
		    break;
		}

		anna_entry_t *argv = malloc(sizeof(anna_entry_t )* meth->input_count);
		int i;
		int ok=1;
		argv[0] = anna_from_obj(obj);
		for(i=0; i<this2->child_count; i++)
		{
		    anna_entry_t next = anna_node_static_invoke_try(
			this2->child[i],
			stack);
		    if(!anna_as_obj(next))
		    {
			ok = 0;
			break;
		    }
		    argv[i+1] = next;
		}
		anna_entry_t res = anna_from_obj(0);
		if(ok)
		{
		    res = anna_from_obj(
			anna_vm_run(
			    anna_function_wrap(meth),
			    meth->input_count,
			    argv));
		    res = anna_as_native(res);
		}
		free(argv);
		return res;
	    }
	    
	}
	
    }
    return anna_from_obj(0);
}

anna_entry_t anna_node_static_invoke(
    anna_node_t *this, 
    anna_stack_template_t *stack)
{
    anna_entry_t res = anna_node_static_invoke_try(this, stack);
    if(anna_entry_null_ptr(res))
    {
	if(anna_error_count == 0)
	{
	    anna_error(
		this,L"Code could not be invoked at compile time\n");
	}
	return null_entry;
    }
    return res;
}


static size_t anna_node_size(anna_node_t *n)
{
    switch(n->node_type)
    {
	case ANNA_NODE_NOTHING:
	case ANNA_NODE_CALL:
	case ANNA_NODE_MEMBER_CALL:
	case ANNA_NODE_STATIC_MEMBER_CALL:
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
	case ANNA_NODE_STATIC_MEMBER_GET:
	case ANNA_NODE_STATIC_MEMBER_SET:
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
    dest->child = src->child_count?malloc(sizeof(anna_node_t *)*src->child_count):0;
    dest->child_capacity = dest->child_count;
    memcpy(dest->child, src->child, sizeof(anna_node_t *)*dest->child_count);
}

anna_node_t *anna_node_clone_shallow(anna_node_t *n)
{
    size_t sz = anna_node_size(n);
    anna_alloc_gc_block();
    anna_node_t *r = anna_alloc_node(sz);
    memcpy(r,n,sz);
    r->return_type=0;
    r->stack=0;
    r->wrapper=0;
    r->transformed=0;
    
    if( (n->node_type == ANNA_NODE_CALL) || 
	(n->node_type == ANNA_NODE_NOTHING) || 
	(n->node_type == ANNA_NODE_CONSTRUCT) || 
	(n->node_type == ANNA_NODE_MEMBER_CALL) ||
	(n->node_type == ANNA_NODE_SPECIALIZE) ||
	(n->node_type == ANNA_NODE_CAST) ||
	(n->node_type == ANNA_NODE_STATIC_MEMBER_CALL))
    {
	anna_node_call_t *r2=(anna_node_call_t *)r;
	anna_node_call_t *n2=(anna_node_call_t *)n;
	anna_node_call_dealias(r2, n2);
    }
    else if(n->node_type == ANNA_NODE_INT_LITERAL)
    {
	anna_node_int_literal_t *r2 = (anna_node_int_literal_t *)r;
	anna_node_int_literal_t *n2 = (anna_node_int_literal_t *)n;
	mpz_init_set(r2->payload, n2->payload);
    }
    else if(n->node_type == ANNA_NODE_STRING_LITERAL)
    {
	anna_node_string_literal_t *r2 = (anna_node_string_literal_t *)r;
	anna_node_string_literal_t *n2 = (anna_node_string_literal_t *)n;
	if(n2->free)
	{
	  r2->payload = malloc(sizeof(wchar_t) * r2->payload_size);
	  memcpy(r2->payload, n2->payload, sizeof(wchar_t) * r2->payload_size);
	}
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
	anna_node_identifier_t *from = (anna_node_identifier_t *)aux2[0];
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
    aux[0] = (anna_node_t *)from;
    aux[1] = to;
    if(!to)
    {
	debug(D_CRITICAL, L"Replace node with nothing\n");
	CRASH;
    }
    
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

	case ANNA_NODE_NOTHING:
	case ANNA_NODE_SPECIALIZE:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *n1 = (anna_node_call_t *)node1;
	    anna_node_call_t *n2 = (anna_node_call_t *)node2;
	    if(n1->child_count != n2->child_count)
		return n1->child_count - n2->child_count;
	    if(node1->node_type == ANNA_NODE_CALL)
	    {
		int ff = anna_node_compare(n1->function, n2->function);
	    
		if(ff)
		    return ff;
	    }
	    
	    int i;
	    for(i=0; i<n1->child_count;i++)
	    {
		int cf = anna_node_compare(n1->child[i], n2->child[i]);
		if(cf)
		    return cf;
	    }
	    return 0;
	}
	
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *id1 = (anna_node_identifier_t *)node1;
	    anna_node_identifier_t *id2 = (anna_node_identifier_t *)node2;
	    return wcscmp(id1->name, id2->name);
	}
	
	case ANNA_NODE_INT_LITERAL:
	{
	    anna_node_int_literal_t *n1 = (anna_node_int_literal_t *)node1;
	    anna_node_int_literal_t *n2 = (anna_node_int_literal_t *)node2;
	    return mpz_cmp( n1->payload, n2->payload);
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *n1 = (anna_node_string_literal_t *)node1;
	    anna_node_string_literal_t *n2 = (anna_node_string_literal_t *)node2;
	    if(n1->payload_size != n2->payload_size)
		return n1->payload_size - n2->payload_size;
	    return wcsncmp(n1->payload, n2->payload, n1->payload_size);   
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
	
	case ANNA_NODE_NULL:
	{
	    return 0;
	}
	
	case ANNA_NODE_DUMMY:
	{
	    anna_node_dummy_t *n1 = (anna_node_dummy_t *)node1;
	    anna_node_dummy_t *n2 = (anna_node_dummy_t *)node2;
	    return n1->payload - n2->payload;
	}

	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *n1 = (anna_node_closure_t *)node1;
	    anna_node_closure_t *n2 = (anna_node_closure_t *)node2;
	    return n1->payload - n2->payload;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *n1 =(anna_node_assign_t *)node1;
	    anna_node_assign_t *n2 =(anna_node_assign_t *)node2;
	    int n = wcscmp(n1->name, n2->name);
	    if(n != 0)
		return n;
	    return anna_node_compare(n1->value, n2->value);
	}

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_BIND:
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *n1 =(anna_node_member_access_t *)node1;
	    anna_node_member_access_t *n2 =(anna_node_member_access_t *)node2;

	    if( n1->access_type != n2->access_type)
	    {
		return n1->access_type - n2->access_type;
	    }	    

	    int ff = anna_node_compare(n1->object, n2->object);
	    
	    if(ff)
		return ff;

	    if(node1->node_type == ANNA_NODE_MEMBER_SET)
	    {
		int val = anna_node_compare(n1->value, n2->value);
		if(val)
		    return val;
	    }

	    return n1->mid - n2->mid;
	}

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
	    anna_node_wrapper_t *n1 = (anna_node_wrapper_t *)node1;
	    anna_node_wrapper_t *n2 = (anna_node_wrapper_t *)node2;
	    return anna_node_compare(n1->payload, n2->payload);
	}
	
	default:
	    anna_message(L"OOPS! Unknown node type when comparing: %d\n", node1->node_type);
	    CRASH;
    }
}

anna_node_t *anna_node_definition_specialize(anna_node_t *code, array_list_t *spec)
{
    int i;
    for(i=0; i<al_get_count(spec); i++)
    {
	anna_node_t *node = (anna_node_t *)al_get(spec, i);
	if(node->node_type == ANNA_NODE_CALL)
	{
	    anna_node_call_t *call = (anna_node_call_t *)node;
	    CHECK_CHILD_COUNT(call, L"Template specialization", 2);
	    CHECK_NODE_TYPE(call->child[0], ANNA_NODE_INTERNAL_IDENTIFIER);
	    code = anna_node_replace(code, (anna_node_identifier_t *)call->child[0], call->child[1]);
	}
	else
	{
	    CHECK_NODE_TYPE(node, ANNA_NODE_MAPPING);
	    anna_node_cond_t *call = (anna_node_cond_t *)node;
	    CHECK_NODE_TYPE(call->arg1, ANNA_NODE_INTERNAL_IDENTIFIER);
	    code = anna_node_replace(code, (anna_node_identifier_t *)call->arg1, call->arg2);
	}
    }
    
    return code;    
}

void anna_node_compile(anna_node_t *this, void *aux)
{
    if(this->node_type == ANNA_NODE_CLOSURE)
    {
	anna_node_closure_t *this2 = (anna_node_closure_t *)this;
	if(!(this2->payload->flags & ANNA_FUNCTION_COMPILATION_STARTED))
	{
	    this2->payload->flags |= ANNA_FUNCTION_COMPILATION_STARTED;
	    if(this2->payload->body)
	    {
		anna_node_each((anna_node_t *)this2->payload->body, &anna_node_compile, 0);
	    }
	    
	    anna_vm_compile(this2->payload);
	}

    }
    if(this->node_type == ANNA_NODE_TYPE)
    {
	anna_node_type_t *this2 = (anna_node_type_t *)this;	
	if(this2->payload->body && !(this2->payload->flags & ANNA_TYPE_COMPILED))
	{
	    this2->payload->flags |= ANNA_TYPE_COMPILED;
	    anna_node_each((anna_node_t *)this2->payload->body, &anna_node_compile, 0);
	}
    }
}

