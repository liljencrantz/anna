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
		    anna_node_call_push(
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
			    anna_node_call_push(
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
    if((node->child_count == 3))
    {
	if (anna_node_is_call_to(node->child[2], L"else")) 
	{
	    anna_node_call_t *else_clause = (anna_node_call_t *)node->child[2];
	    if (else_clause->child_count != 1)
	    {
		anna_error(node->child[2], L"Invalid else clause");
		return anna_node_create_null(&node->location);
	    }
	    node->child[2] = else_clause->child[0];
	}
    }
    
    int i;
    for(i=1; i<node->child_count; i++)
    {
	if(!anna_node_is_call_to(node->child[i], L"__block__"))
	{
	    node->child[i] = (anna_node_t *)
		anna_node_create_block(
		    &node->location,
		    1,
		    &node->child[i]);
	}
	CHECK_NODE_BLOCK(node->child[i]);
    }
    return (anna_node_t *)anna_node_create_if(
	&node->location, 
	node->child[0],
	(anna_node_call_t *)node->child[1],
	node->child_count == 2 ? 0 : (anna_node_call_t *)node->child[2]);
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

static void anna_macro_add_internal(
    anna_stack_template_t *stack, 
    wchar_t *name,
    anna_native_t call,
    ...)
{
    wchar_t *argn[] = {L"node"};
    
    anna_function_t *f = anna_native_create(
	name,
	ANNA_FUNCTION_MACRO,
	call,
	node_type,
	1,
	&node_call_type,
	argn,
	0,
	stack);
    
    va_list va;
    va_start( va, call );
    wchar_t *arg;
    while( (arg=va_arg(va, wchar_t *) )!= 0 ) 
    {
	anna_function_document(f, anna_intern_static(arg));
    }
    va_end( va );

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

#define anna_macro_add( ... ) anna_macro_add_internal( __VA_ARGS__, (void *)0 )

void anna_macro_init(anna_stack_template_t *stack)
{
    anna_stack_document(
	stack,
	L"The builtinMacros module contains all the macros that are "
	"enabled by default in Anna.");

    anna_stack_document(
	stack,
	L"These macros are a fundamental part of the Anna language, and "
	"implement things like function definition, type definition, "
	"variable declaration and assignment, conditional execution and "
	"template specialization. There are also a large number of utility "
	"macros that make using Anna significantly easier. These include "
	"macros for functional programming, list assignment, simplified "
	"type definition and error handling.");

    anna_stack_document(
	stack,
	L"Many of these macros are almost exclusively used indirectly through "
	"various syntactic sugar, like := or the def syntax for defining "
	"functions.");
    
    anna_macro_add(
	stack,
	L"__def__", 
	&anna_macro_def,
	L"Create a function with the specified definition.",
	L"This macro is usually called indirectly through the def-syntactic sugar. It expects exactly five parameters,",
	L"<ol><li>the name of the function, which must be an identifier</li><li>the return type of the function,</li><li>the input parameter list of the function,</li><li>the attribute list of the function and</li><li>the function body.</li></ol>",
	L"It is possible to call the __def__ function directly, but the def sugar is significantly easier to use. Compare"
	anna_example(L"next :== __def__(next, ?, {var Int in}, {}, {in+1});\n"),
	L"with",
	anna_example(L"def next(int in) {in+1}\n"));

    anna_macro_add(stack, L"__defInternal__", &anna_macro_def_internal, L"Internal utility function used by __def__. Don't call directly.");
    anna_macro_add(
	stack,
	L"__block__",
	&anna_macro_block,
	L"Create a block function.",
	L"This macro is a shortcut for creating functions that take no arguments and where you don't care about the return type. It is usually used through the {} syntactic sugar like this:",
	anna_example(L"someBlock := {print(\"Hello\")}\n")
	);

    anna_macro_add(
	stack,
	L"__loopBlock__",
	&anna_macro_block,
	L"Create a block for a while loop.");

    anna_macro_add(
	stack,
	L"__memberGet__",
	&anna_macro_member_get,
	L"Returns a member of the specified object.",
	L"Usually used through the '.' operator.",
	anna_example(L"// These two lines are equivalent\n__memberGet__(foo, bar);\nfoo.bar;\n"));

    anna_macro_add(
	stack,
	L"__memberSet__",
	&anna_macro_member_set,
	L"Assigns a new value to the specified member of the specified object.",
	L"Usually used through the '.' operator.",
	anna_example(L"// These two lines are equivalent\n__memberSet__(foo, bar, 5);\nfoo.bar = 5;\n")
	);

    anna_macro_add(
	stack,
	L"__staticMemberGet__",
	&anna_macro_static_member_get,
	L"Return a static member of the specified type.",
	L"Usually used through the '::' operator.",
	anna_example(L"// These two lines are equivalent\n__staticMemberGet__(Foo, bar);\nfoo::bar;\n")
	);

    anna_macro_add(
	stack,
	L"__staticMemberSet__",
	&anna_macro_static_member_set,
	L"Assigns a new value to the specified static member of the specified type.",
	L"Usually used through the '::' operator.",
	anna_example(L"// These two lines are equivalent\n__staticMemberSet__(Foo, bar, 5);\nfoo::bar = 5;\n")
	);

    anna_macro_add(
	stack,
	L"__var__",
	&anna_macro_var,
	L"Declare a new variable value.",
	L"A variable is a name that can be associated with a particular value.",
	L"The __var__ macro is usually used through the := syntactic sugar, like this:",
	anna_example(
	    L"myVariable :== 5;\nprint(myVariable); // Prints '5'\nmyVariable = 7; // This reassigns myVariable to hold the value 7.\n"),
	L"__var__ expects exactly four parameters,",
	L"<ol><li>the name of the variable,</li><li>the return type of the variable,</li><li>the value of the variable and</li><li>the attribute list of the variable.</li></ol>");
    anna_macro_add(
	stack,
	L"__const__",
	&anna_macro_var,
	L"Declare a new constant value.",
	L"A constant is like a variable, except that it always references the same object.",
	(
	    L"Note that a contant can point to a mutable object (Such as a "
	    L"MutableList or a MutableString) in which case the state of the "
	    L"object can be freely mutated, but the constant will always point "
	    L"to the <em>same</em> object instance, even as the actual state of "
	    L"the object is changing. The concepts of constness (variables that "
	    L"can't be reassigned) and imutability (objects that can not change "
	    L"their state), while related, must not be confused. It <em>is</em> very "
	    L"easy to confuse them though, especially if one comes from a C++ "
	    L"background, where the const keyword is used to denote both, and "
	    L"the exact placement of the keyword in a declaration decides which "
	    L"of the two is meant."),
	L"The __const__ macro is usually used through the :== syntactic sugar, like this:",
	anna_example(
	    L"myConstant :== 5;\n"
	    L"print(myConstant); // Prints '5'\n"
	    L"myConstant = 7; // This is a syntax error, myConstant can't be reassigned.\n"),
	L"__const__ expects exactly four parameters,",
	L"<ol><li>the name of the constant,</li><li>the return type of the constant,</li><li>the value of the constant and</li><li>the attribute list of the constant.</li></ol>"
	);

    anna_macro_add(
	stack,
	L"__varInternal__",
	&anna_macro_var_internal,
	L"Internal utility macro used by __var__. Don't call directly.");

    anna_macro_add(
	stack,
	L"__constInternal__",
	&anna_macro_var_internal,
	L"Internal utility macro used by __const__. Don't call directly.");

    anna_macro_add(
	stack,
	L"__or__",
	&anna_macro_or,
	L"Execute one expression, and if it returns null, also execute a second expression. Return value is the first expression if it is non-null, otherwise the second expression.",
	L"This macro is often used for grouping of logical operations, but is also very useful for providing a fallback value in case of failure.",
	anna_example(
	    L"// Logical or check.\n"
	    L"if(cond1() or cond2()) {...}\n"
	    L"// Provide a default value of 10\n"
	    L"numberOfLaps :== Int::convert(system.argument[1]) or 10;"),
	L"Used together, the <code>and</code> and <code>or</code> operators can be used to replace the C ternary operator."
	anna_example(
	    L"// Equivalent to cond ? val1 : val2 in C\n"
	    L"cond and val1 or val2;\n")
	);

    anna_macro_add(
	stack, 
	L"__and__",
	&anna_macro_and,
	L"Execute one expression, and if it returns non-null, also execute a second expression. Return value is null if the first expression fails, otherwise the second expression.",
	L"This macro is usually used through the <code>and</code> syntactic suger, and is mainly used for grouping of logical operations.",
	anna_example(
	    L"// Logical and check.\n"
	    L"if(cond1() and cond2()) {...}\n"),
	L"Used together, the <code>and</code> and <code>or</code> operators can be used to replace the C ternary operator."
	anna_example(
	    L"// Equivalent to cond ? val1 : val2 in C\n"
	    L"cond and val1 or val2;\n")
	);

    anna_macro_add(
	stack,
	L"if",
	&anna_macro_if,
	L"Conditionally execute a block of code.",
	anna_example(L"if(weather == cloudy)\n{\n   print(\"Don\'t forget your umbrella!\";\n}\n"),
	L"The if macro is often used together with the <a member='else'>else-macro</a> to provide a second block to execute if the condition is not met.",
	L"Usage example:",
	anna_example(L"if(weather == cloudy)\n{\n   print(\"Don\'t forget your umbrella!\";\n}\nelse\n{\n   print(\"Better put on some sun lotion!\";\n}\n"),
	L"The if macro can also be used as a replacement for the C-style ternary operator:",
	anna_example(L"myMoodDescription := if(happieness >=5, \"happy\", \"sad\");\n")
	);

    anna_macro_add(
	stack,
	L"while",
	&anna_macro_while,
	L"Repeatedly execute a block until a condition fails.",
	L"while expects exactly two parameters,",
	L"<ol><li>the conditional expression and</li><li>the code block to execute.</li></ol>",
	L"An example usage of the while macro:",
	anna_example(L"print(\"How old are you?\");\nwhile(!(age := Int::convert(readLine.readLine)))\n{\n    print(\"Invalid number, please try again!\");\n}")
	);

    anna_macro_add(
	stack,
	L"__assign__",
	&anna_macro_assign,
	L"Assign a new value to a variable.",
	L"__assign__ takes two parameters, the first must be an identifier, the second can be any expression. If the identifier does not denote a declared variable, or if the expression can't mask as the type of that variable, a compilation error will result.",
	L"Usually used through the '=' syntactic sugar,",
	anna_example(
	    L"// These two lines are equivalent.\nmyVariable = 7;\n__assign__(myVariable, 7);\n"
	    )
	);

    anna_macro_add(
	stack, L"__macro__", &anna_macro_macro,
	L"Create a new function which is a valid macro with the specified definition.",
	L"A macro is any function that takes an ast call node as its only input and returns any AST node. Any and all module functions that has the correct function signature is a macro. Whether or not a given function with the correct signature is considered for use as a macro during the macro expansion phase of compilation depends on whether the module that it belongs to has been specified to be used for macro expansion using the expand() expression. The <code>__macro__</code>-macro is simply a shorthand for the corresponding def-based function definition. Since all macros deal with AST nodes, the parser-module is also implicitly use:d by any function defined using the <code>__macro__</code>-macro. Specifically, the two following definitions are equivalent:",
	anna_example(
	    L"def parser.Node next(parser.Call node)\n"
	    L"{\n"
	    L"    use(parser);\n"
	    L"    return ast(1+%node) % [\"node\": node];\n"
	    L"}\n"
	    L"\n"
	    L"macro next(node)}n"
	    L"{\n"
	    L"    return ast(1+%node) % [\"node\": node];\n"
	    L"}\n"));

    anna_macro_add(
	stack, L"__specialize__", &anna_macro_specialize,
	L"Specialize the specified type or function template.");

    anna_macro_add(
	stack,
	L"typeType",
	&anna_macro_type,
	L"Define a raw new type.",
	L"This macro is rarely used directly, but rather through an intermediary macro like classType, structType or enumType, that make it significantly more convenient to create and manipulate types. And even then, the type definition syntactic sugar is almost always used to make it easier to define the type.", 
	L"typeType expects exactly 3 parameters,",
	L"<ol><li>the name of the type,</li><li>the attribute list of the type and</li><li>a block containing all member declarations of the type.</li></ol>",
	L"A new type can be created using code like",
	anna_example(L"myManualType :== typeType(myManualType, {}, {def myMethod() (bound) {print(\"Hi\")}});\n")
	L"which is equivalent to",
	anna_example(L"type myManualType {def myMethod() (bound) {print(\"Hi\")}};\n")
	);

    anna_macro_add(
	stack, L"__typeInternal__", &anna_macro_type_internal,
	L"Internal utility macro used by typeType. Don't call directly.");

    anna_macro_add(
	stack,
	L"return",
	&anna_macro_return,
	L"Stop execution of the current funtion and return the specified value.",
	anna_example("def double(Int in){ return in+in }\n")
	);

    anna_macro_add(
	stack,
	L"staticType",
	&anna_macro_type_of,
	L"Calculate the static type of the specified expression.",
	L"This function is similar to the <a path='lang' member='type'>type</a>-function, but where type returns the true type of an expression, staticType returns the type calculated by the compiler, which may be different. An important property of the staticType macro is that it can be used in places where a type is required during compilation, e.g. as the type in a function declaration. For example,",
	anna_example(L"myNumber := 7;\nmyFunction :== __def__(myFunction, ?, {__var__(in, staticType(myNumber), ?, {})}, {}, {in+1});\nmyFunction(4);\n"));

    anna_macro_add(
	stack, L"__staticReturnTypeOf__", &anna_macro_return_type_of,
	L"Calculate the static type of the return value of the specified function.");

    anna_macro_add(
	stack, L"__staticInputTypeOf__", &anna_macro_input_type_of,
	L"Calculate the static type of the input parameter of the specified function with the specified index.");

    anna_macro_add(
	stack,
	L"__cast__",
	&anna_macro_cast,
	L"Cast thespecified value to a different type.",
	L"Usually used through the as-operator",
	anna_example(L"print((myAstNode as parser.Call)[0]);\n"),
	L"If the cast is illegal, a null value is returned instead.");

    anna_macro_add(
	stack, L"break", &anna_macro_break,
	L"Stop execution of the current loop and return the specified value.");

    anna_macro_add(
	stack, L"continue", &anna_macro_continue,
	L"Stop the execution of the current lap of the current loop and return the specified value.");

    anna_macro_add(
	stack, L"use", &anna_macro_use,
	L"This macro imports the specified expression, so that any members in it will automatically be considered as variables during variable name lookup.",
	anna_example(
	    L"// Prints the constant math.pi\n"
	    L"print(math.pi);\n"
	    L"// Imports the module math into the list of implicit namespaces\n"
	    L"use(math);\n"
	    L"// Because of the above use-clause, this will locate the pi constant\n"
	    L"// in the math module, so it is equivalent to the first line above.\n"
	    L"print(pi);\n"
	    ),
	L"Note that the use macro can be used on any object, not just modules.",
	anna_example(
	    L"myList := [];\n"
	    L"use(myList);\n"
	    L"// This will call myList.push and myList.pop\n"
	    L"push(3);\n"
	    L"print(pop());\n"
	    )
	);

    anna_macro_add(
	stack,
	L"nothing",
	&anna_macro_nothing, 
	L"Execute all the specified expressions and return the last one.",
	L"This macro is often very convinient inside of other macros, as it allows you to replace a single expression with multiple new expressions.",
	L"This example macro will print the file location of an expression every time it is executed:",
	anna_example(
	    L"macro printAndDo(node)\n{\n"
	    "    ast(nothing(print(\"File: %, line: %\" % [%file, %line]), %node)) %\n"
	    "        [\"node\": node, \"file\": StringLiteral(node, node.file), \"line\": node.line];\n"
	    "}"));
}
