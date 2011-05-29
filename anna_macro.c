#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "common.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_node_wrapper.h"
#include "anna_macro.h"
#include "anna_type.h"
#include "anna_util.h"
#include "anna_function.h"
#include "anna_node_check.h"
#include "anna_node_create.h"
#include "anna_vm.h"

static inline anna_node_t *anna_macro_macro_i(anna_node_call_t *node)
{

    CHECK_CHILD_COUNT(node,L"macro definition", 3);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[2]);

    anna_node_identifier_t *name_identifier = (anna_node_identifier_t *)node->child[0];
    anna_node_identifier_t *param = (anna_node_identifier_t *)node->child[1];
    
    anna_function_t *result;
    result = anna_macro_create(
	name_identifier->name,
	node,
	param->name);
    return (anna_node_t *)
	anna_node_create_declare(
	    &node->location,
	    name_identifier->name,
	    anna_node_create_null(&node->location),
	    (anna_node_t *)anna_node_create_closure(
		&node->location,
		result
		),
	    anna_node_create_block2(&node->location),
	    1);
}
ANNA_VM_MACRO(anna_macro_macro)

static inline anna_node_t *anna_macro_type_i(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"type macro", 3);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_BLOCK(node->child[2]);    
    
    wchar_t *name = ((anna_node_identifier_t *)node->child[0])->name;
    anna_type_t *type = anna_type_create(name, node);
    
    return (anna_node_t *)anna_node_create_type(
	&node->location,
	type);
}
ANNA_VM_MACRO(anna_macro_type)

static inline anna_node_t *anna_macro_return_i(
    anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"return", 1);
    return (anna_node_t *)anna_node_create_return(&node->location, node->child[0], ANNA_NODE_RETURN);
}
ANNA_VM_MACRO(anna_macro_return)

static inline anna_node_t *anna_macro_def_i(
    anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node, L"__def__", 5);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);

    anna_function_t *fun = 
	anna_function_create_from_definition(
	    node);
    assert(fun);
    
    return (anna_node_t *)anna_node_create_closure(
	&node->location,
	fun);    
}
ANNA_VM_MACRO(anna_macro_def)

static inline anna_node_t *anna_macro_block_i(
    anna_node_call_t *node)
{
    //debug(D_SPAM,L"Create new block with %d elements at %d\n", node->child_count, node);
     
    anna_function_t *fun = 
	anna_function_create_from_block(
	    node);

    return (anna_node_t *)anna_node_create_closure(
	&node->location,
	fun);    
}
ANNA_VM_MACRO(anna_macro_block)

static inline anna_node_t *anna_macro_var_i(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"variable declaration", 4);
    CHECK_NODE_TYPE(node->child[3], ANNA_NODE_CALL);
    
    anna_node_identifier_t *name = node_cast_identifier(node->child[0]);
    
    return (anna_node_t *)
	anna_node_create_declare(
	    &node->location,
	    name->name,
	    node->child[1],
	    node->child[2],
	    (anna_node_call_t *)node->child[3],
	    anna_node_is_named(node->function, L"__const__"));
}
ANNA_VM_MACRO(anna_macro_var)

static inline anna_node_t *anna_macro_specialize_i(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"type specialization", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    anna_node_call_t *res = (anna_node_call_t *)node->child[1];
    res->function = node->child[0];
    res->node_type = ANNA_NODE_SPECIALIZE;
    res->location = node->location;   
    return node->child[1];
}
ANNA_VM_MACRO(anna_macro_specialize)


static inline anna_node_t *anna_macro_return_type_of_i(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"type of return", 1);
    anna_node_wrapper_t *res = anna_node_create_return_type_of(
	&node->location,
	node->child[0]);
    return (anna_node_t *)res;    
}
ANNA_VM_MACRO(anna_macro_return_type_of)

static inline anna_node_t *anna_macro_input_type_of_i(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"type of return", 2);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_INT_LITERAL);
    anna_node_int_literal_t *idx = (anna_node_int_literal_t *)node->child[1];
    int i_idx = mpz_get_si(idx->payload);
    anna_node_wrapper_t *res = anna_node_create_input_type_of(
	&node->location,
	node->child[0],
	i_idx);
    return (anna_node_t *)res;    
}
ANNA_VM_MACRO(anna_macro_input_type_of)

static anna_node_t *anna_macro_cast_i(
    anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"cast", 2);
//    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    node->function = anna_node_create_null(&node->location);
    
    node->node_type = ANNA_NODE_CAST;
    return (anna_node_t *)node;
}

ANNA_VM_MACRO(anna_macro_cast)


static inline anna_node_t *anna_macro_break_i(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"break", 0);
    return (anna_node_t *)anna_node_create_return(
	&node->location,
	0,
	ANNA_NODE_BREAK);
}
ANNA_VM_MACRO(anna_macro_break)

static inline anna_node_t *anna_macro_continue_i(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"continue", 0);
    return (anna_node_t *)anna_node_create_return(
	&node->location,
	0,
	ANNA_NODE_CONTINUE);
}
ANNA_VM_MACRO(anna_macro_continue)



#include "anna_macro_attribute.c"
#include "anna_macro_conditional.c"
#include "anna_macro_operator.c"

static void anna_macro_add(
    anna_stack_template_t *stack, 
    wchar_t *name,
    anna_native_t call)
{    
    anna_function_t *f = anna_native_create(
	name,
	ANNA_FUNCTION_MACRO,
	call,
	node_call_wrapper_type,
	0,
	0,
	0,
	stack);
    
    anna_function_set_stack(f, stack);
    anna_function_setup_interface(f);
    anna_function_setup_body(f);
    anna_stack_declare(
	stack,
	name,
	f->wrapper->type,
	f->wrapper,
	0);
}

void anna_macro_init(anna_stack_template_t *stack)
{
    anna_macro_add(stack, L"__def__", &anna_macro_def);
    anna_macro_add(stack, L"__block__", &anna_macro_block);
    anna_macro_add(stack, L"__memberGet__", &anna_macro_member_get);
    anna_macro_add(stack, L"__memberSet__", &anna_macro_member_set);
    anna_macro_add(stack, L"__var__", &anna_macro_var);
    anna_macro_add(stack, L"__const__", &anna_macro_var);
    anna_macro_add(stack, L"__or__", &anna_macro_or);
    anna_macro_add(stack, L"__and__", &anna_macro_and);
    anna_macro_add(stack, L"__if__", &anna_macro_if);
    anna_macro_add(stack, L"while", &anna_macro_while);
    anna_macro_add(stack, L"__assign__", &anna_macro_assign);
    anna_macro_add(stack, L"__macro__", &anna_macro_macro);
    anna_macro_add(stack, L"__specialize__", &anna_macro_specialize);
    anna_macro_add(stack, L"__type__", &anna_macro_type);
    anna_macro_add(stack, L"return", &anna_macro_return);
    anna_macro_add(stack, L"__staticReturnTypeOf__", &anna_macro_return_type_of);
    anna_macro_add(stack, L"__staticInputTypeOf__", &anna_macro_input_type_of);
    anna_macro_add(stack, L"cast", &anna_macro_cast);
    anna_macro_add(stack, L"__break__", &anna_macro_break);
    anna_macro_add(stack, L"__continue__", &anna_macro_continue);
}
