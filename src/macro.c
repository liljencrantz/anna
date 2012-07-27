#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/fallback.h"
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
#include "anna/intern.h"

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
	    anna_message(L"FADFADS\n");
	    anna_node_print(5, nam);
	    anna_message(L"\n\n");
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
    CHECK_NODE_TYPE(node->child[2], ANNA_NODE_CALL);
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
    CHECK_NODE_TYPE(node->child[3], ANNA_NODE_CALL);
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
    CHECK_NODE_BLOCK(node->child[3]);
    if(anna_node_is_call_to(node->child[0], L"__collection__"))
    {
	node->function = (anna_node_t *)anna_node_create_identifier(
	    &node->child[0]->location, 
	    anna_node_is_named(node->function, L"__const__")?L"__constList__":L"__varList__");
	return (anna_node_t *)node;
    }
    
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);

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


ANNA_VM_MACRO(anna_macro_assign)
{
    CHECK_CHILD_COUNT(node,L"assignment operator", 2);

    switch(node->child[0]->node_type)
    {

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *name_identifier = 
		node_cast_identifier(node->child[0]);
	    return (anna_node_t *)
		anna_node_create_assign(&node->location,
					name_identifier->name,
					node->child[1]);
	}
       
	case ANNA_NODE_CALL:
	{

	    anna_node_call_t *call = node_cast_call(node->child[0]);
	    //    anna_node_print(0, call->function);

	    if(call->function->node_type == ANNA_NODE_IDENTIFIER)
	    {
		
/*
__assign__(__memberGet__( OBJ, KEY), VL  )
 => 
__memberSet__( OBJ, KEY, VAL)

 */
		anna_node_identifier_t *name_identifier = node_cast_identifier(call->function);
		int is_set = (wcscmp(name_identifier->name, L"__memberGet__")==0) || (wcscmp(name_identifier->name, L"__staticMemberGet__")==0);
		int is_static = wcscmp(name_identifier->name, L"__staticMemberGet__")==0;
		
		if(is_set)
		{
		    // foo.bar = baz
		    call->function = (anna_node_t *)
			anna_node_create_identifier(
			&name_identifier->location, 
			is_static ? L"__staticMemberSet__":L"__memberSet__");
		    anna_node_call_add_child(
			call, 
			node->child[1]);
		    return (anna_node_t *)call;
		}	    
		else if(anna_node_is_call_to(node->child[0], L"__collection__"))
		{
		    node->function = (anna_node_t *)anna_node_create_identifier(
			&node->child[0]->location, 
			L"__assignList__");
		    return (anna_node_t *)node;
		}
		
	    }
	    else if(call->function->node_type == ANNA_NODE_CALL)
	    {
		anna_node_call_t *call2 = node_cast_call(call->function);
		
		if(call2->function->node_type == ANNA_NODE_IDENTIFIER && call2->child_count == 2)
		{
		    
		    anna_node_identifier_t *name_identifier = node_cast_identifier(call2->function);
		    
		    if(wcscmp(name_identifier->name, L"__memberGet__")==0 && call2->child[1]->node_type == ANNA_NODE_IDENTIFIER)
		    {
			anna_node_identifier_t *name_identifier2 = node_cast_identifier(call2->child[1]);
			if(wcscmp(name_identifier2->name, L"__get__")==0)
			{
			    /*
			      __assign__(__memberGet__( OBJ, "__get__")(KEY), val  )
			      
			      __memberGet__( OBJ, "__set__")( KEY, VAL)
			      
			    */
			    call2->child[1] = (anna_node_t *)
				anna_node_create_identifier(
				    &name_identifier->location,
				    L"__set__");
			    anna_node_call_add_child(
				call, 
				node->child[1]);
			    return (anna_node_t *)call;
			}
		    }
		}		
	    }
	}
    }
    FAIL(node->child[0], L"Invalid left-hand value in assignment");
}

ANNA_VM_MACRO(anna_macro_member_get)
{
    CHECK_CHILD_COUNT(node,L". operator", 2);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
        
    anna_node_identifier_t *name_node = node_cast_identifier(node->child[1]);
    mid_t mid = anna_mid_get(name_node->name);

    return (anna_node_t *)anna_node_create_member_get(
	&node->location,
	ANNA_NODE_MEMBER_GET,
	node->child[0], 
	mid);
}

ANNA_VM_MACRO(anna_macro_static_member_get)
{
    CHECK_CHILD_COUNT(node,L". operator", 2);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
        
    anna_node_identifier_t *name_node = node_cast_identifier(node->child[1]);
    mid_t mid = anna_mid_get(name_node->name);

    return (anna_node_t *)anna_node_create_member_get(
	&node->location,
	ANNA_NODE_STATIC_MEMBER_GET,
	node->child[0], 
	mid);
}

 
ANNA_VM_MACRO(anna_macro_member_set)
{
    CHECK_CHILD_COUNT(node,L"member assignment", 3);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    
    anna_node_identifier_t *name_node = node_cast_identifier(node->child[1]);
    mid_t mid = anna_mid_get(name_node->name);

    return (anna_node_t *)anna_node_create_member_set(
	&node->location,
	ANNA_NODE_MEMBER_SET,
	node->child[0], 
	mid,
	node->child[2]);
}

ANNA_VM_MACRO(anna_macro_static_member_set)
{
    CHECK_CHILD_COUNT(node,L"static member assignment", 3);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    
    anna_node_identifier_t *name_node = node_cast_identifier(node->child[1]);
    mid_t mid = anna_mid_get(name_node->name);

    return (anna_node_t *)anna_node_create_member_set(
	&node->location,
	ANNA_NODE_STATIC_MEMBER_SET,
	node->child[0], 
	mid,
	node->child[2]);
}


ANNA_VM_MACRO(anna_macro_or)
{
    CHECK_CHILD_COUNT(node,L"or expression", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_OR,
	    node->child[0],
	    node->child[1]);
}

ANNA_VM_MACRO(anna_macro_and)
{
    CHECK_CHILD_COUNT(node,L"and expression", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_AND,
	    node->child[0],
	    node->child[1]);
}

ANNA_VM_MACRO(anna_macro_if)
{
    if(node->child_count < 2 || node->child_count > 3)
    {
	anna_error((anna_node_t *)node, L"Invalid parameter count");
	return anna_node_create_null(&node->location);
    }
    CHECK_NODE_BLOCK(node->child[1]);
    if(node->child_count == 2)
    {
	return (anna_node_t *)
	    anna_node_create_if(
		&node->location, 
		node->child[0],
		(anna_node_call_t *)node->child[1],
		anna_node_create_block2(&node->location));
    }
    else
    {
	CHECK_NODE_BLOCK(node->child[2]);
	anna_node_if_t *res = anna_node_create_if(
		&node->location, 
		node->child[0],
		(anna_node_call_t *)node->child[1],
		(anna_node_call_t *)node->child[2]);
	res->has_else = 1;
	return (anna_node_t *)res;
    }
}

ANNA_VM_MACRO(anna_macro_else)
{
    CHECK_CHILD_COUNT(node,L"else clause", 1);
    CHECK_NODE_BLOCK(node->child[0]);
    node->flags |= ANNA_NODE_MERGE;
    node->flags |= ANNA_NODE_DONT_EXPAND;
    return (anna_node_t *)node;
}

ANNA_VM_MACRO(anna_macro_while)
{
    CHECK_CHILD_COUNT(node,L"while loop expression", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    anna_node_call_t *block = (anna_node_call_t *) node->child[1];
    block->function = (anna_node_t *)anna_node_create_identifier(&block->function->location, L"__loopBlock__");
    
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_WHILE,
	    node->child[0],
	    node->child[1]);
}

ANNA_VM_MACRO(anna_macro_nothing)
{
    node->node_type = ANNA_NODE_NOTHING;
    node->object = 0;
    return (anna_node_t *)node;
}

static void anna_macro_add(
    anna_stack_template_t *stack, 
    wchar_t *name,
    anna_native_t call,
    wchar_t *doc)
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
    anna_function_document(f,anna_intern_static(doc));
    
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
    anna_macro_add(stack, L"__def__", &anna_macro_def, L"Create a function with the specified definition.");
    anna_macro_add(stack, L"__defInternal__", &anna_macro_def_internal, L"Internal utility function used by __def__. Don't call directly.");
    anna_macro_add(stack, L"__block__", &anna_macro_block, L"Create a block function.");
    anna_macro_add(stack, L"__loopBlock__", &anna_macro_block, L"Create a block for a while loop.");
    anna_macro_add(stack, L"__staticMemberGet__", &anna_macro_static_member_get, L"Return a static member of the specified type.");
    anna_macro_add(stack, L"__memberGet__", &anna_macro_member_get, L"Returns a member of the specified object.");
    anna_macro_add(stack, L"__memberSet__", &anna_macro_member_set, L"Assigns a new value to the specified member of the specified object.");
    anna_macro_add(stack, L"__staticMemberSet__", &anna_macro_static_member_set, L"Assigns a new value to the specified static member of the specified type.");
    anna_macro_add(stack, L"__var__", &anna_macro_var, L"Declare a new variable.");
    anna_macro_add(stack, L"__const__", &anna_macro_var, L"Declare a new constant.");
    anna_macro_add(stack, L"__varInternal__", &anna_macro_var_internal, L"Internal utility macro used by __var__. Don't call directly.");
    anna_macro_add(stack, L"__constInternal__", &anna_macro_var_internal, L"Internal utility macro used by __const__. Don't call directly.");
    anna_macro_add(stack, L"__or__", &anna_macro_or, L"");
    anna_macro_add(stack, L"__and__", &anna_macro_and, L"");
    anna_macro_add(stack, L"if", &anna_macro_if, L"Conditionally execute a block.");
    anna_macro_add(stack, L"else", &anna_macro_else, L"");
    anna_macro_add(stack, L"while", &anna_macro_while, L"Repeatedly execute a block until a condition fails.");
    anna_macro_add(stack, L"__assign__", &anna_macro_assign, L"Set a new value to a variable.");
    anna_macro_add(stack, L"__macro__", &anna_macro_macro, L"Create a new function which is a valid macro with the specified definition.");
    anna_macro_add(stack, L"__specialize__", &anna_macro_specialize, L"Specialize the specified type or function template.");
    anna_macro_add(stack, L"typeType", &anna_macro_type, L"Define a new type.");
    anna_macro_add(stack, L"__typeInternal__", &anna_macro_type_internal, L"Internal utility macro used by typeType. Don't call directly.");
    anna_macro_add(stack, L"return", &anna_macro_return, L"Stop execution of the current funtion and return the specified value.");
    anna_macro_add(stack, L"__staticTypeOf__", &anna_macro_type_of, L"Calculate the static return type of the specified expression.");
    anna_macro_add(stack, L"__staticReturnTypeOf__", &anna_macro_return_type_of, L"");
    anna_macro_add(stack, L"__staticInputTypeOf__", &anna_macro_input_type_of, L"");
    anna_macro_add(stack, L"cast", &anna_macro_cast, L"Cast thespecified value to the specified type.");
    anna_macro_add(stack, L"break", &anna_macro_break, L"Stop execution of the current loop and return the specified value.");
    anna_macro_add(stack, L"continue", &anna_macro_continue, L"Stop the execution of the current lap of the current loop and return the specified value.");
    anna_macro_add(stack, L"use", &anna_macro_use, L"");
    anna_macro_add(stack, L"nothing", &anna_macro_nothing, L"Execute all the specified expressions and return the last one.");
}
