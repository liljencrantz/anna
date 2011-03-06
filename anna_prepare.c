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
#include "anna_node_create.h"
#include "anna_list.h"

#if 0
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


/**
   Sniff out all the return statements of the specified function and put them into the list.

*/
static void sniff(
    array_list_t *lst, 
    anna_function_t *f, 
    int level)
{
/*
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
*/
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
    //wprintf(L"Sniff return type of function %ls\n", f->name);
    

    array_list_t types;
    al_init(&types);
    sniff(&types, f, 0);
    int i;
        
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
	anna_node_call_add_child(f->body, anna_node_create_null(&f->body->location));
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
}

void anna_prepare()
{
}

void anna_prepare_stack_functions(anna_stack_template_t *stack, wchar_t *name, anna_node_t *context)
{
}

#endif
