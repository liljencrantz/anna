#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_stack.h"
#include "anna_type.h"
#include "anna_node.h"
#include "anna_node_check.h"
#include "anna_prepare.h"
#include "anna_macro.h"
#include "anna_function.h"
#include "anna_node_wrapper.h"

struct prepare_item
{
    anna_function_t *function;
    anna_type_t *type;
    wchar_t *action;
}
    ;

typedef struct prepare_item prepare_item_t;

static void anna_prepare_function_internal(
    anna_function_t *function);

static anna_node_t *anna_prepare_type_interface_internal(
    anna_type_t *type);

static void anna_sniff_return_type(anna_function_t *f);

static array_list_t *preparation_list=0;

void anna_prepare_describe(prepare_item_t *item)
{
    if(item->type)
	wprintf(L"%ls of type %ls\n", item->action, item->type->name);
    else
    {
	if(item->function->flags & ANNA_FUNCTION_MODULE)
	{
	    wprintf(L"%ls of module %ls\n", item->action, item->function->name);
	}
	else if(item->function->member_of)
	{
	    wprintf(L"%ls of method %ls\n", item->action, item->function->name);
	}
	else
	{
	    wprintf(L"%ls of function %ls\n", item->action, item->function->name);
	}
    }
}

static int anna_prepare_index(prepare_item_t *item)
{
    int i;
    if(!preparation_list)
    {
	preparation_list = malloc(sizeof(array_list_t));
	al_init(preparation_list);
    }

    for(i=0; i<al_get_count(preparation_list); i++)
    {
	prepare_item_t *it = (prepare_item_t *)al_get(preparation_list, i);
	
	if(it->function == item->function &&
	   it->type == item->type &&
	   wcscmp(it->action, item->action)==0)
	{
	    return i;
	}
    }
    return -1;
}

int anna_prepare_check(prepare_item_t *item)
{
    int i = anna_prepare_index(item);
    if(i != -1)
    {
	wprintf(L"Critical: Encountered a cyclic dependency chain in code:\n");
	for(;i<al_get_count(preparation_list); i++)
	{
	    anna_prepare_describe((prepare_item_t *)al_get(preparation_list, i));
	}
	anna_prepare_describe(item);
	CRASH;
	return 1;
    }
    al_push(preparation_list, item);
    return 0;
}


void anna_prepare_pop()
{
    al_pop(preparation_list);
}



/**
   Sniff out all the return statements of the specified function and put them into the list.

*/
static void sniff(
    array_list_t *lst, 
    anna_function_t *f, 
    int level)
{
    if(level != 0)
	anna_prepare_function_interface(f);
    //wprintf(L"Sniff return type for %ls\n", f->name);
    
    if(f->return_pop_count == level)
    {
	int i;
	
	for(i=0;i<f->body->child_count;i++)
	{
	    if(f->body->child[i]->node_type == ANNA_NODE_RETURN)
	    {
		anna_node_return_t *r = (anna_node_return_t *)f->body->child[i];
		al_push(lst, anna_node_get_return_type(r->payload, f->stack_template));
	    }
	}
	for(i=0;i<al_get_count(&f->child_function);i++)
	{
	    sniff(lst, al_get(&f->child_function, i), level+1);
	}
    }
}

/**
  Try to figure out the return type of the specified function. 

  If there are return calls in the function itself or it's subblocks,
  the return type is determined by sniffing the types of those return
  statements. Otherwise, the return type is the type of the last
  expressin in the body, or null if the body is empty.
 */
static void anna_sniff_return_type(anna_function_t *f)
{
    array_list_t types;
    al_init(&types);
    sniff(&types, f, 0);
    int i;
    
    anna_prepare_function(f);
        
    if(al_get_count(&types) >0)
    {
	//wprintf(L"Got the following %d return types for function %ls, create intersection:\n", al_get_count(&types), f->name);
	anna_type_t *res = al_get(&types, 0);
	for(i=1;i<al_get_count(&types); i++)
	{
	    anna_type_t *t = al_get(&types, i);
	    res = anna_type_intersect(res,t);
	}
	f->return_type = res;
	anna_node_call_add_child(f->body, anna_node_null_create(&f->body->location));
    }
    else
    {
	if(f->body->child_count)
	    f->return_type = anna_node_get_return_type(f->body->child[f->body->child_count-1], f->stack_template);
	else
	    f->return_type = null_type;
	//wprintf(L"Implicit return type is %ls\n", f->return_type->name);
    }

}



/**
   Given an AST node, this will return a type object that the specified node represents.

   E.g. an identifier node with the payload "Object" will return the object type.
*/
static anna_type_t *anna_prepare_type_from_identifier(
    anna_node_t *node,
    anna_function_t *function, 
    anna_node_list_t *parent)
{
    node = anna_node_prepare(node, function, parent);
   
    if(node->node_type == ANNA_NODE_DUMMY ||
       node->node_type == ANNA_NODE_TRAMPOLINE ) {
	anna_node_dummy_t *dummy = (anna_node_dummy_t *)node;
	//anna_object_print(dummy->payload);
	if(anna_type_is_fake(dummy->payload->type))
	{
	    anna_function_t *f = anna_function_unwrap(dummy->payload);
	    if(f)
	    {
		anna_prepare_function_interface(anna_function_unwrap(dummy->payload));
	    }


	    if(anna_type_is_fake(dummy->payload->type))
	    {
		wprintf(
		    L"Critical: Preparation resulted in fake function type\n");
		CRASH;
	    }
	    
	}

	return dummy->payload->type;
//      return anna_type_unwrap(dummy->payload);      
    }
    else if(
	node->node_type == ANNA_NODE_IDENTIFIER ||
	node->node_type == ANNA_NODE_IDENTIFIER_TRAMPOLINE) 
    {
	anna_node_identifier_t *id = (anna_node_identifier_t *)node;
	anna_object_t *wrapper = anna_stack_get_str(function->stack_template, id->name);
	//wprintf(L"LALA\n");
	//anna_object_print(wrapper);
	return anna_type_unwrap(wrapper);
    }
    else if(node->node_type == ANNA_NODE_MEMBER_GET) 
    {
	anna_node_member_get_t *mg= (anna_node_member_get_t *)node;
	/*
	  FIXME: Add support for namespaces within namespaces!

	  FIXME: Check that type is actually a namespace and not some random object...
	*/
	if(mg->object->node_type == ANNA_NODE_IDENTIFIER)
	{
	    anna_node_identifier_t *id = (anna_node_identifier_t *)mg->object;
	    anna_object_t *wrapper = anna_stack_get_str(function->stack_template, id->name);
	    anna_stack_frame_t *stack = anna_stack_unwrap(wrapper);    
	    return anna_type_unwrap(anna_stack_get_str(stack, anna_mid_get_reverse(mg->mid)));
	}
    }
    
    anna_error(node,L"Could not determine type of node of type %d", node->node_type);
    anna_node_print(node);
    CRASH;
    return 0;
}


#include "anna_prepare_function.c"
#include "anna_prepare_type.c"


void anna_prepare_internal()
{
    int i=0, j=0, k=0, m=0, n=0, p=0;
    int again;
    
    do
    {
	again=0;
	
	int function_count = al_get_count(&anna_function_list); 
	int type_count = al_get_count(&anna_type_list); 
	
	/*
	  Prepare all macros and their subblocks. 
	*/
	for(; i<function_count; i++)
	{
	    anna_function_t *func = (anna_function_t *)al_get(&anna_function_list, i);
	    
	    if((func->flags & ANNA_FUNCTION_MACRO) 
	       && (func->body)
	       && !(func->flags &ANNA_FUNCTION_PREPARED_IMPLEMENTATION))
	    {
		//wprintf(L"Prepare macro %ls\n", func->name);
		again=1;
		anna_prepare_function_recursive(func);
	    }
	}
	
	/*
	  Register all known types
	*/

	for(; j<type_count; j++)
	{
	    anna_type_t *type = (anna_type_t *)al_get(&anna_type_list, j);
	    
	    if(!(type->flags & ANNA_TYPE_REGISTERED))
	    {
		type->flags |= ANNA_TYPE_REGISTERED;
		//wprintf(L"Register type %ls\n", type->name);
		again=1;
		anna_stack_declare(type->stack,
				   type->name,
				   type_type,
				   anna_type_wrap(type));
	    }
	}
	
	/*
	  Prepare interfaces of all known types
	*/
	for(; k<type_count; k++)
	{
	    anna_type_t *type = (anna_type_t *)al_get(&anna_type_list, k);
	    if(!(type->flags & ANNA_TYPE_PREPARED_INTERFACE))
	    {
		//wprintf(L"Prepare interface for type %ls\n", type->name);
		anna_prepare_type_interface(type);
		again=1;
	    }
/*	anna_stack_set_str(type->stack,
	type->name,
	anna_type_wrap(type));
*/
	    
	}
    //anna_stack_print(stack_global);
    
    
    /*
      Prepare all non-macro functions and their subblocks
     */
	for(; m<function_count; m++)
	{
	    anna_function_t *func = (anna_function_t *)al_get(&anna_function_list, m);
/*
  wprintf(L"Prepare subfunction %d of %d in function %ls: %ls\n", 
  i, al_get_count(&function->child_function),
  function->name, func->name);
*/	
	    if((!(func->flags & ANNA_FUNCTION_MACRO)) 
	       && !(func->flags &ANNA_FUNCTION_PREPARED_INTERFACE))
	    {
		again=1;
		anna_prepare_function_interface(func);
	    }
	}
	
	for(; n<function_count; n++)
	{
	    anna_function_t *func = (anna_function_t *)al_get(&anna_function_list, n);

/*
	    wprintf(L"Prepare function %ls\n", 
		    func->name);
*/	    


	    if((!(func->flags & ANNA_FUNCTION_MACRO)) 
	       && (func->body)
	       && !(func->flags &ANNA_FUNCTION_PREPARED_IMPLEMENTATION))
	    {
		again=1;
		anna_prepare_function(func);
	    }
	}
    
    /*
      Prepare implementations of all known types
     */
	for(; p<type_count; p++)
	{
	    anna_type_t *type = (anna_type_t *)al_get(&anna_type_list, p);
	    if(!(type->flags & ANNA_TYPE_PREPARED_IMPLEMENTATION))
	    {
		again=1;
		anna_prepare_type_implementation(type);
	    }
	}

    }
    while(again);
}


void anna_prepare()
{
    wprintf(L"Prepare all types and functions\n");
    anna_member_create(
	type_type,
	ANNA_MID_TYPE_WRAPPER_PAYLOAD,
	L"!typeWrapperPayload",
	0,
	null_type);
    anna_prepare_internal();
    wprintf(L"Preparations complete\n");

}

void anna_prepare_stack_functions(anna_stack_frame_t *stack, wchar_t *name, anna_node_t *context)
{
    //wprintf(L"Searching stack for %ls\n", name);
    
    if(!stack)
    {
	anna_error(context, L"Failed variable search: %ls.", name);
	CRASH;
    }
    
    if(anna_stack_frame_get_str(stack, name))
	return;
    
    if(stack->function)
    {
	prepare_item_t it = 
	    {
		stack->function, 0, L"Body preparation"
	    }
	;
	if(anna_prepare_index(&it) == -1)
	{
//	    wprintf(L"Prepare function %ls\n", stack->function->name);
	    anna_prepare_function(stack->function);
	    if(anna_stack_frame_get_str(stack, name))
		return;
	}
	else
	{
//	    wprintf(L"Ignore function %ls, already being prepared\n", stack->function->name);
	}
	
    }
    
    anna_prepare_stack_functions(stack->parent, name, context);
}

