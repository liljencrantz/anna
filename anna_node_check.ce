#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_node_check.h"

anna_type_t *extract_type(anna_node_t *node, anna_stack_template_t *stack)
{
    if(node->node_type != ANNA_NODE_IDENTIFIER)
    {
	anna_error(node, L"Expected type identifier");
	return 0;
    }
    anna_node_identifier_t *id = (anna_node_identifier_t *)node;
    anna_object_t *wrapper = anna_stack_get_str(stack, id->name);
    if(!wrapper)
    {
	anna_error(node, L"Unknown type: %ls", id->name);
	return 0;	
    }
    anna_type_t *type = anna_type_unwrap(wrapper);
    if(!type)
    {
	anna_error(node, L"Identifier is not a type: %ls", id->name);
    }
    return type;
    
}

int check_node_identifier_name(anna_node_t *node,
			       wchar_t *name)
{
    if(node->node_type != ANNA_NODE_IDENTIFIER)
    {
	anna_error((anna_node_t *)node,
		   L"Unexpected argument type, expected an identifier");
	return 0;
    }
    anna_node_identifier_t *l = (anna_node_identifier_t *)node;
    if(wcscmp(l->name, name)!=0)
    {
	anna_error((anna_node_t *)node,
		   L"Unexpected identifier value, expected \"%ls\"", name);
	return 0;	
    }
    return 1;

}


int check_node_block(anna_node_t *n)
{
    if(n->node_type != ANNA_NODE_CALL)
    {
	anna_error((anna_node_t *)n,
		   L"Unexpected argument type. Expected a block definition.");
	return 0;	    
    }
    {
	anna_node_call_t *call = (anna_node_call_t *)n;
	if(call->function->node_type != ANNA_NODE_IDENTIFIER)
	{
	    anna_error((anna_node_t *)call->function,
		       L"Unexpected argument type. Expected a block definition.");
	    return 0;	    
	}
	anna_node_identifier_t *id = (anna_node_identifier_t *)call->function;
	if(wcscmp(id->name, L"__block__") != 0)
	{
	    anna_error((anna_node_t *)call->function,
		       L"Unexpected argument type. Expected a block definition.");
	    return 0;	    
	}
    }
    return 1;
}

