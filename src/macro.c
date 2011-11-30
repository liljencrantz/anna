#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/util.h"
#include "anna/common.h"
#include "anna/base.h"
#include "anna/node.h"
#include "anna/lib/parser.h"
#include "anna/macro.h"
#include "anna/type.h"
#include "anna/util.h"
#include "anna/function.h"
#include "anna/node_check.h"
#include "anna/node_create.h"
#include "anna/vm.h"
#include "anna/mid.h"

static anna_node_t *anna_macro_attribute_expand(anna_node_call_t *node, anna_node_call_t *attr)
{
    int i;
    
    for(i = attr->child_count-1; i >= 0; i--)
    {
	if(attr->child[i]->node_type == ANNA_NODE_IDENTIFIER)
	{
	    anna_node_identifier_t *nam = (anna_node_identifier_t *)attr->child[i];
	    string_buffer_t sb;
	    sb_init(&sb);
	    sb_printf(&sb, L"%lsAttribute", nam->name);
	    
	    node = anna_node_create_call2(
		&node->location,
		anna_node_create_identifier(
		    &nam->location,
		    sb_content(&sb)),
		node);
	    sb_destroy(&sb);
/*
	    wprintf(L"FADFADS\n");
	    anna_node_print(5, nam);
	    wprintf(L"\n\n");
*/
	}
	
    }
    return (anna_node_t *)node;
}


ANNA_VM_MACRO(anna_macro_macro)
{
    CHECK_CHILD_COUNT(node,L"macro definition", 4);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[2]);
    CHECK_NODE_BLOCK(node->child[3]);

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

ANNA_VM_MACRO(anna_macro_type)
{
    CHECK_CHILD_COUNT(node,L"type macro", 3);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_BLOCK(node->child[2]);

    anna_node_call_t *attr = (anna_node_call_t *)node->child[1];
    
    node->function = (anna_node_t *)anna_node_create_identifier(
	&node->function->location,
	L"__typeInternal__");
    
    return anna_macro_attribute_expand(node, attr);
}

ANNA_VM_MACRO(anna_macro_type_internal)
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

ANNA_VM_MACRO(anna_macro_return)
{
    CHECK_CHILD_COUNT(node,L"return", 1);
    return (anna_node_t *)anna_node_create_return(&node->location, node->child[0], ANNA_NODE_RETURN);
}

ANNA_VM_MACRO(anna_macro_def)
{
    CHECK_CHILD_COUNT(node, L"__def__", 5);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[2]);
    CHECK_NODE_BLOCK(node->child[3]);
    CHECK_NODE_BLOCK(node->child[4]);

    anna_node_call_t *attr = (anna_node_call_t *)node->child[3];
    
    node->function = (anna_node_t *)anna_node_create_identifier(
	&node->function->location,
	L"__defInternal__");
    
    return anna_macro_attribute_expand(node, attr);
}

ANNA_VM_MACRO(anna_macro_def_internal)
{
    CHECK_CHILD_COUNT(node, L"__defInternal__", 5);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_TYPE(node->child[3], ANNA_NODE_CALL);

    anna_function_t *fun = 
	anna_function_create_from_definition(
	    node);
    assert(fun);
    
    return (anna_node_t *)anna_node_create_closure(
	&node->location,
	fun);    
}

ANNA_VM_MACRO(anna_macro_block)
{
    //debug(D_SPAM,L"Create new block with %d elements at %d\n", node->child_count, node);
    anna_function_t *fun = 
	anna_function_create_from_block(
	    node);
    
    if(!fun)
    {
	return (anna_node_t *)anna_node_create_null(&node->location);
    }
    
    return (anna_node_t *)anna_node_create_closure(
	&node->location,
	fun);    
}

ANNA_VM_MACRO(anna_macro_var)
{
    CHECK_CHILD_COUNT(node, L"variable declaration", 4);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[3]);

    anna_node_call_t *attr = (anna_node_call_t *)node->child[3];
    
    node->function = (anna_node_t *)anna_node_create_identifier(
	&node->function->location,
	anna_node_is_named(node->function, L"__const__")?L"__constInternal__": L"__varInternal__");

    return anna_macro_attribute_expand(node, attr);
}


ANNA_VM_MACRO(anna_macro_var_internal)
{
    CHECK_CHILD_COUNT(node,L"variable declaration", 4);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_TYPE(node->child[3], ANNA_NODE_CALL);
    
    anna_node_identifier_t *name = node_cast_identifier(node->child[0]);
    
    return (anna_node_t *)
	anna_node_create_declare(
	    &node->location,
	    name->name,
	    node->child[1],
	    node->child[2],
	    (anna_node_call_t *)node->child[3],
	    anna_node_is_named(node->function, L"__constInternal__"));
}

ANNA_VM_MACRO(anna_macro_specialize)
{
    CHECK_CHILD_COUNT(node,L"type specialization", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    anna_node_call_t *res = (anna_node_call_t *)node->child[1];
    res->function = node->child[0];
    res->node_type = ANNA_NODE_SPECIALIZE;
    res->location = node->location;   
    return node->child[1];
}

ANNA_VM_MACRO(anna_macro_type_of)
{
    CHECK_CHILD_COUNT(node,L"type of", 1);
    anna_node_wrapper_t *res = anna_node_create_type_of(
	&node->location,
	node->child[0]);
    return (anna_node_t *)res;    
}

ANNA_VM_MACRO(anna_macro_return_type_of)
{
    CHECK_CHILD_COUNT(node,L"type of return", 1);
    anna_node_wrapper_t *res = anna_node_create_return_type_of(
	&node->location,
	node->child[0]);
    return (anna_node_t *)res;    
}

ANNA_VM_MACRO(anna_macro_input_type_of)
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

ANNA_VM_MACRO(anna_macro_cast)
{
    CHECK_CHILD_COUNT(node,L"cast", 2);
//    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    node->function = anna_node_create_null(&node->location);
    
    node->node_type = ANNA_NODE_CAST;
    return (anna_node_t *)node;
}

ANNA_VM_MACRO(anna_macro_break)
{
    CHECK_CHILD_COUNT(node,L"break", 1);
    return (anna_node_t *)anna_node_create_return(
	&node->location,
	node->child[0],
	ANNA_NODE_BREAK);
}

ANNA_VM_MACRO(anna_macro_continue)
{
    CHECK_CHILD_COUNT(node,L"continue", 1);
    return (anna_node_t *)anna_node_create_return(
	&node->location,
	node->child[0],
	ANNA_NODE_CONTINUE);
}

ANNA_VM_MACRO(anna_macro_use)
{
    CHECK_CHILD_COUNT(node,L"use", 1);
    return (anna_node_t *)anna_node_create_return(
	&node->location,
	node->child[0],
	ANNA_NODE_USE);
}

#include "src/macro_conditional.c"
#include "src/macro_operator.c"

static void anna_macro_add(
    anna_stack_template_t *stack, 
    wchar_t *name,
    anna_native_t call)
{    
    anna_function_t *f = anna_native_create(
	name,
	ANNA_FUNCTION_MACRO,
	call,
	node_call_type,
	0,
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
	anna_from_obj(f->wrapper),
	0);
}

void anna_macro_init(anna_stack_template_t *stack)
{
    anna_macro_add(stack, L"__def__", &anna_macro_def);
    anna_macro_add(stack, L"__defInternal__", &anna_macro_def_internal);
    anna_macro_add(stack, L"__block__", &anna_macro_block);
    anna_macro_add(stack, L"__loopBlock__", &anna_macro_block);
    anna_macro_add(stack, L"__staticMemberGet__", &anna_macro_static_member_get);
    anna_macro_add(stack, L"__memberGet__", &anna_macro_member_get);
    anna_macro_add(stack, L"__memberSet__", &anna_macro_member_set);
    anna_macro_add(stack, L"__staticMemberSet__", &anna_macro_static_member_set);
    anna_macro_add(stack, L"__var__", &anna_macro_var);
    anna_macro_add(stack, L"__const__", &anna_macro_var);
    anna_macro_add(stack, L"__varInternal__", &anna_macro_var_internal);
    anna_macro_add(stack, L"__constInternal__", &anna_macro_var_internal);
    anna_macro_add(stack, L"__or__", &anna_macro_or);
    anna_macro_add(stack, L"__and__", &anna_macro_and);
    anna_macro_add(stack, L"if", &anna_macro_if);
    anna_macro_add(stack, L"else", &anna_macro_else);
    anna_macro_add(stack, L"while", &anna_macro_while);
    anna_macro_add(stack, L"__assign__", &anna_macro_assign);
    anna_macro_add(stack, L"__macro__", &anna_macro_macro);
    anna_macro_add(stack, L"__specialize__", &anna_macro_specialize);
    anna_macro_add(stack, L"type", &anna_macro_type);
    anna_macro_add(stack, L"__typeInternal__", &anna_macro_type_internal);
    anna_macro_add(stack, L"return", &anna_macro_return);
    anna_macro_add(stack, L"__staticTypeOf__", &anna_macro_type_of);
    anna_macro_add(stack, L"__staticReturnTypeOf__", &anna_macro_return_type_of);
    anna_macro_add(stack, L"__staticInputTypeOf__", &anna_macro_input_type_of);
    anna_macro_add(stack, L"cast", &anna_macro_cast);
    anna_macro_add(stack, L"break", &anna_macro_break);
    anna_macro_add(stack, L"continue", &anna_macro_continue);
    anna_macro_add(stack, L"use", &anna_macro_use);
}
