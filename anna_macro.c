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

static anna_node_t *anna_macro_macro_i(anna_node_call_t *node)
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

static anna_node_t *anna_macro_iter_declare(anna_node_t *id)
{
    return (anna_node_t *)anna_node_create_call2(
	&id->location,
	(anna_node_t *)anna_node_create_identifier(&id->location,L"__var__"),
	id,
	anna_node_create_null(&id->location),
	anna_node_create_null(&id->location));
}

anna_node_t *anna_macro_iter_i(anna_node_call_t *node)			    
{
    anna_node_t *n[]={0,0};
    anna_node_t *body;
    if(node->child_count < 2)
    {
	anna_error((anna_node_t *)node,
		   L"Invalid arguments for iteration");
    }
    if(node->child_count > 3)
    {
	anna_error((anna_node_t *)node,
		   L"Invalid arguments for iteration");
    }
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[node->child_count-1]);
    body = node->child[node->child_count-1];
    if(node->child_count == 2)
    {
	n[0] = anna_macro_iter_declare((anna_node_t *)anna_node_create_identifier(
	    &body->location,
	    L"!unused"));
	n[1] = anna_macro_iter_declare(node->child[0]);
    }
    else
    {
	CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
	n[0] = anna_macro_iter_declare(node->child[0]);
	n[1] = anna_macro_iter_declare(node->child[1]);
    }
    
    anna_node_call_t *attribute_list = anna_node_create_block2(&body->location);
    anna_node_call_t *declaration_list = anna_node_create_block(&body->location, 2, n);
    
    node->child[0] = (anna_node_t *)anna_node_create_call2(
	&body->location,
	(anna_node_t *)anna_node_create_identifier(&body->location,L"__def__"), 
	(anna_node_t *)anna_node_create_identifier(
	    &body->location,
	    L"!anonymous"),
	anna_node_create_null(&body->location),
	declaration_list, 
	attribute_list, 
	body);
    
    
    node->child_count = 1;
    anna_node_call_t *fun = (anna_node_call_t *)node->function;

    anna_node_identifier_t *orig_name = (anna_node_identifier_t *)fun->child[1];

    string_buffer_t sb;
    sb_init(&sb);
    sb_append(&sb, L"__");
    sb_append(&sb, orig_name->name);
    sb_append(&sb, L"__");
    

    fun->child[1] = (anna_node_t *)anna_node_create_identifier(
	&fun->child[1]->location,
	sb_content(&sb));
    sb_destroy(&sb);
    
    return (anna_node_t *)node;
    
}
ANNA_VM_MACRO(anna_macro_iter)


static anna_node_t *anna_macro_type_i(anna_node_call_t *node)
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

static anna_node_t *anna_macro_return_i(
    anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"return", 1);
    return (anna_node_t *)anna_node_create_return(&node->location, node->child[0]);
}
ANNA_VM_MACRO(anna_macro_return)

static anna_node_t *anna_macro_ast_i(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"ast", 1);
    return (anna_node_t *)anna_node_create_dummy(
	&node->location,
	anna_node_wrap(node->child[0]),
	0);
}
ANNA_VM_MACRO(anna_macro_ast)

static anna_node_t *anna_macro_def_i(
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

static anna_node_t *anna_macro_block_i(
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

static anna_node_t *anna_macro_var_i(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"variable declaration", 4);
    CHECK_NODE_TYPE(node->child[3], ANNA_NODE_CALL);
    
    anna_node_identifier_t *name = node_cast_identifier(node->child[0]);
    
    debug(D_SPAM,L"Declare a stack variable %ls with initial value\n", name->name);
    anna_node_print(0, node->child[2]);
    
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

static anna_node_t *anna_macro_specialize_i(anna_node_call_t *node)
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

static anna_node_t *anna_macro_collection_i(anna_node_call_t *node)
{
    if(node->child_count == 0)
    {
	anna_error((anna_node_t *)node,
		   L"At least one argument required");
	return (anna_node_t *)anna_node_create_null(&node->location);
    }

    if(anna_node_is_call_to(node->child[0], L"__mapping__"))
    {
	anna_node_call_t *p0 = (anna_node_call_t *)node->child[0];
	CHECK_CHILD_COUNT(p0, L"Map", 2);
	
	anna_node_t *param[] = 
	    {
		(anna_node_t *)anna_node_create_type_lookup(
		    &node->function->location,
		    p0->child[0]),
		(anna_node_t *)anna_node_create_type_lookup(
		    &node->function->location,
		    p0->child[1])
	    }
	;
	
	node->function = (anna_node_t *)anna_node_create_specialize(
	    &node->function->location,
	    (anna_node_t *)anna_node_create_dummy(
		&node->function->location,
		anna_type_wrap(hash_type), 0),
	    2,
	    param);

	int i;
	for(i=0; i<node->child_count; i++)
	{
	    if(!anna_node_is_call_to(node->child[i], L"__mapping__"))
	    {
		anna_error(node->child[i], L"Not a key value pair");
		return (anna_node_t *)anna_node_create_null(&node->location);
	    }
	    anna_node_call_t *pp = (anna_node_call_t *)node->child[i];
	    CHECK_CHILD_COUNT(pp, L"Map", 2);
	    
	    anna_node_t *pp_param[] = 
		{
		    (anna_node_t *)anna_node_create_type_lookup(
			&pp->function->location,
			p0->child[0]),
		    (anna_node_t *)anna_node_create_type_lookup(
			&pp->function->location,
			p0->child[1])
		}
	    ;
	
	    pp->function = (anna_node_t *)anna_node_create_specialize(
		&pp->function->location,
		(anna_node_t *)anna_node_create_dummy(
		    &pp->function->location,
		    anna_type_wrap(pair_type), 0),
		2,
		pp_param);
	    
	}
	
	return (anna_node_t *)node;
    }
    else
    {
	
	anna_node_t *param[] = 
	    {
		(anna_node_t *)anna_node_create_type_lookup(
		    &node->function->location,
		    node->child[0])
	    }
	;
	
	node->function = (anna_node_t *)anna_node_create_specialize(
	    &node->function->location,
	    (anna_node_t *)anna_node_create_dummy(
		&node->function->location,
		anna_type_wrap(list_type), 0),
	    1,
	    param);
	return (anna_node_t *)node;
    }
    
}
ANNA_VM_MACRO(anna_macro_collection)

static anna_node_t *anna_macro_update_i(anna_node_call_t *node)
{
    if(node->child_count == 1)
    {
	/*
	  __next__(i) 
              => 
	  __assign(i, __memberGet(i,__nextAssign__)())

	 */
	string_buffer_t name;
	CHECK_NODE_TYPE(node->function, ANNA_NODE_IDENTIFIER);
	anna_node_identifier_t *name_id = (anna_node_identifier_t *)node->function;
	sb_init(&name);
	sb_append_substring(&name, name_id->name, wcslen(name_id->name)-2);
	sb_append(&name, L"Assign__");
	
	
	anna_node_t *param[] ={
	    node->child[0], 
	    (anna_node_t *)
	    anna_node_create_call2(
		&node->location,
		(anna_node_t *)anna_node_create_call2(
		    &node->location,
		    (anna_node_t *)anna_node_create_identifier(&node->location,L"__memberGet__"),
	anna_node_clone_deep((anna_node_t *)node->child[0]),
		    (anna_node_t *)anna_node_create_identifier(&node->location,sb_content(&name))))};
	sb_destroy(&name);

	anna_node_t *res = (anna_node_t *)
	    anna_node_create_call(
		&node->location,
		(anna_node_t *)anna_node_create_identifier(&node->location,L"__assign__"),
		2,
		param);
	return res;
    }
    if(node->child_count == 2)
    {
	/*
	  __append__(i,j)
              => 
	  __assign(i, __memberGet(i,__appendAssign__)(j))

	 */
	string_buffer_t name;
	CHECK_NODE_TYPE(node->function, ANNA_NODE_IDENTIFIER);
	anna_node_identifier_t *name_id = (anna_node_identifier_t *)node->function;
	sb_init(&name);
	sb_append_substring(&name, name_id->name, wcslen(name_id->name)-2);
	sb_append(&name, L"Assign__");
	
	anna_node_t *param0[] ={
	    anna_node_clone_deep((anna_node_t *)node->child[0]),
	    (anna_node_t *)anna_node_create_identifier(&node->location,sb_content(&name))
	};
	sb_destroy(&name);

	anna_node_t *param_call[] ={
	    node->child[1]
	};
	
	anna_node_t *param[] ={
	    node->child[0], 
	    (anna_node_t *)
	    anna_node_create_call(
		&node->location,
		(anna_node_t *)anna_node_create_call(
		    &node->location,
		    (anna_node_t *)anna_node_create_identifier(&node->location,L"__memberGet__"),
		    2,
		    param0),
		1,
		param_call)
	};
	anna_node_t *res = (anna_node_t *)
	    anna_node_create_call(
		&node->location,
		(anna_node_t *)anna_node_create_identifier(&node->location,L"__assign__"),
		2,
		param);
	return res;
    }
    anna_error((anna_node_t *)node, L"Invalid number of arguments");
    return anna_node_create_null(&node->location);
}
ANNA_VM_MACRO(anna_macro_update)

static anna_node_t *anna_macro_range_i(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"Range", 2);
    node->function = (anna_node_t *)anna_node_create_identifier(
	&node->function->location,
	L"Range");

    if(anna_node_is_call_to(node->child[1], L"Pair")){
	anna_node_call_t *pair = (anna_node_call_t *)node->child[1];
	CHECK_CHILD_COUNT(pair,L"Range", 2);
	node->child[1] = pair->child[0];
	anna_node_call_add_child(
	    node,
	    pair->child[1]);
    }
    else 
    {
	anna_node_call_add_child(
	    node,
	    (anna_node_t *)anna_node_create_null(0));
    }
    anna_node_call_add_child(
	node,
	(anna_node_t *)anna_node_create_null(0));
    
    return (anna_node_t *)node;
}
ANNA_VM_MACRO(anna_macro_range)

static anna_node_t *anna_macro_mapping_i(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"mapping", 2);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    node->child[0]->node_type = ANNA_NODE_MAPPING_IDENTIFIER;
    return (anna_node_t *)anna_node_create_mapping(
	&node->location,
	node->child[0],
	node->child[1]
	);
}
ANNA_VM_MACRO(anna_macro_mapping)

#include "anna_macro_attribute.c"
#include "anna_macro_conditional.c"
#include "anna_macro_operator.c"
#include "anna_macro_cast.c"

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
    
    anna_function_setup_interface(f, stack);
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
    anna_macro_add(stack, L"ast", &anna_macro_ast);
    anna_macro_add(stack, L"__assign__", &anna_macro_assign);
    anna_macro_add(stack, L"__macro__", &anna_macro_macro);
    anna_macro_add(stack, L"each", &anna_macro_iter);
    anna_macro_add(stack, L"map", &anna_macro_iter);
    anna_macro_add(stack, L"filter", &anna_macro_iter);
    anna_macro_add(stack, L"filterFirst", &anna_macro_iter);
    anna_macro_add(stack, L"__specialize__", &anna_macro_specialize);
    anna_macro_add(stack, L"__collection__", &anna_macro_collection);
    anna_macro_add(stack, L"type", &anna_macro_type);
    anna_macro_add(stack, L"__range__", &anna_macro_range);
    anna_macro_add(stack, L"__next__", &anna_macro_update);
    anna_macro_add(stack, L"__prev__", &anna_macro_update);
    anna_macro_add(stack, L"__increase__", &anna_macro_update);
    anna_macro_add(stack, L"__decrease__", &anna_macro_update);
    anna_macro_add(stack, L"__append__", &anna_macro_update);
    anna_macro_add(stack, L"__mapping__", &anna_macro_mapping);
    anna_macro_add(stack, L"cast", &anna_macro_cast);
    anna_macro_add(stack, L"return", &anna_macro_return);
    
}
