#ifndef ANNA_NODE_CHECK_H
#define ANNA_NODE_CHECK_H

#include "anna/base.h"
#include "anna/node.h"
#include "anna/stack.h"


/**
   If the condition cond is true, print an error message using
   anna_error(), and return a null AST node. The first argument is the
   condition, the second is the node, the third is the message, any
   following arguments are message parameters that will be formated
   into the message by anna_error().
*/
#define CHECK(cond, n, ...)if(!(cond))					\
    {									\
        anna_error((anna_node_t *)n, __VA_ARGS__);			\
        return (anna_node_t *)anna_node_create_null(&((n)->location));	\
    }  

/**
   Print an error message using anna_error(), and return a null AST
   node. The first argument is the node, the second one is the
   message, any following arguments are message parameters that will
   be formated into the message by anna_error().
*/
#define FAIL(n, ...)							\
    anna_error((anna_node_t *)n, __VA_ARGS__);				\
    return (anna_node_t *)anna_node_create_null(&((n)->location));

/**
   Check that the specified call node has the specified number of
   child nodes.
*/
#define CHECK_CHILD_COUNT(n, name, count)				\
    if(n->child_count != count)						\
    {									\
	anna_error(							\
	    (anna_node_t *)(n),						\
	    L"Wrong number of arguments to %ls: Got %d, expected %d",	\
	    name, n->child_count, count);				\
	return (anna_node_t *)anna_node_create_null(&n->location);	\
    }

/**
   Check that the specified node is of the specified node type
*/
#define CHECK_NODE_TYPE(n, type)					\
    if((n)->node_type != type)						\
    {									\
	anna_error(							\
	    (anna_node_t *)n,						\
	    L"Unexpected argument type, expected a parameter of type %s on line %d of %s", \
	    #type, __LINE__, __FILE__);					\
	return (anna_node_t *)anna_node_create_null(&(n)->location);	\
    }

/**
   Check that the specified node is a block.
*/
#define CHECK_NODE_BLOCK(n)						\
    if(!check_node_block(n))						\
        return (anna_node_t *)anna_node_create_null(&(n)->location)
/**
   Check that the specified node is an identifier with the specicified
   value.
*/
#define CHECK_NODE_IDENTIFIER_NAME(n, name)				\
    if(!check_node_identifier_name(n, name))				\
	return (anna_node_t *)anna_node_create_null(&node->location)

int check_node_identifier_name(anna_node_t *node,
			       wchar_t *name);

int check_node_block(anna_node_t *n);

#endif
