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

struct prepare_prev
{
    struct prepare_prev *prev;
    anna_function_t *function;
    anna_type_t *type;
}
    ;

typedef struct prepare_prev prepare_prev_t;

static void anna_prepare_function_internal(
    anna_function_t *function, 
    prepare_prev_t *prev);

static anna_node_t *anna_prepare_type_interface_internal(
    anna_type_t *type, 
    prepare_prev_t *dep);

static void anna_sniff_return_type(anna_function_t *f);

static void sniff(
    array_list_t *lst, 
    anna_function_t *f, int level)
{
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


static void anna_sniff_return_type(anna_function_t *f)
{
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



static anna_node_t *anna_prepare_function_interface(
    anna_function_t *function)
{
    int is_variadic=0;
    wchar_t *name=0;
    int is_anonymous=1;
    anna_node_call_t *node = function->definition;
    anna_type_t *type = function->type;
    
    wprintf(L"\n\nCreate function interface from node:\n");
    anna_node_print((anna_node_t *)node);

    CHECK_CHILD_COUNT(node,L"function definition", 5);
    
    if (node->child[0]->node_type == ANNA_NODE_IDENTIFIER) 
    {
	anna_node_identifier_t *name_identifier = (anna_node_identifier_t *)node->child[0];
	name = name_identifier->name;
	is_anonymous=0;
    }
    else {
	CHECK_NODE_TYPE(node->child[0], ANNA_NODE_NULL);
	name = L"!anonymous";
    }
    
    anna_node_t *body = node->child[4];

    anna_node_list_t list = 
	{
	    (anna_node_t *)body, 0, 0
	}
    ;
    
    
    if(body->node_type != ANNA_NODE_NULL && 
       body->node_type != ANNA_NODE_CALL &&
	body->node_type != ANNA_NODE_DUMMY)
    {
        FAIL(body, L"Invalid function body");
    }
    
    anna_type_t *out_type=0;
    anna_node_t *out_type_wrapper = node->child[1];
    
    if(out_type_wrapper->node_type == ANNA_NODE_NULL) 
    {	
	CHECK(body->node_type == ANNA_NODE_CALL, body, L"Function declarations must have a return type");
    }
    else
    {
        anna_node_prepare_child(node, 1, function, &list);
	out_type_wrapper = node->child[1];
	anna_node_identifier_t *type_identifier;
	type_identifier = node_cast_identifier(out_type_wrapper);
	anna_object_t *type_wrapper = anna_stack_get_str(
	    function->stack_template, 
	    type_identifier->name);

	if(!type_wrapper)
	{
	    FAIL(type_identifier, L"Unknown type: %ls", type_identifier->name);
	}
	out_type = anna_type_unwrap(type_wrapper);
    }
    
    size_t argc=0;
    anna_type_t **argv=0;
    wchar_t **argn=0;
    
    CHECK_NODE_TYPE(node->child[2], ANNA_NODE_CALL);
    anna_node_call_t *declarations = node_cast_call(node->child[2]);
    int i;
    if(declarations->child_count > 0 || type)
    {
	argc = declarations->child_count;
	if(type)
	{
	    argc++;
	}
	
	argv = malloc(sizeof(anna_type_t *)*argc);
	argn = malloc(sizeof(wchar_t *)*argc);
	
	if(type)
	{
	    argv[0]=type;
	    argn[0]=L"this";
	}
	
	for(i=0; i<declarations->child_count; i++)
	{
	    //declarations->child[i] = anna_node_prepare(declarations->child[i], function, parent);
	    CHECK_NODE_TYPE(declarations->child[i], ANNA_NODE_CALL);
	    anna_node_call_t *decl = node_cast_call(declarations->child[i]);
	    
	    CHECK_NODE_TYPE(decl->function, ANNA_NODE_IDENTIFIER);
	    anna_node_identifier_t *fun = node_cast_identifier(decl->function);
	    if(wcscmp(fun->name, L"__declare__") == 0)
	    {
		//CHECK_CHILD_COUNT(decl, L"variable declaration", 3);
		CHECK_NODE_TYPE(decl->child[0], ANNA_NODE_IDENTIFIER);
		anna_node_prepare_child(
		    decl,
		    1,
		    function,
		    &list);
		anna_node_identifier_t *name = 
		    node_cast_identifier(
			decl->child[0]);
		
		anna_node_identifier_t *type_name =
		    node_cast_identifier(decl->child[1]);
		
		anna_object_t **type_wrapper =
		    anna_stack_addr_get_str(function->stack_template, type_name->name);
		CHECK(
		    type_wrapper, 
		    (anna_node_t *)type_name,
		    L"Unknown type: %ls",
		    type_name->name);

		argv[i+!!type] = 
		    anna_type_unwrap(*type_wrapper);
		argn[i+!!type] = name->name;		
		
		if(decl->child_count ==3 &&
		   decl->child[2]->node_type == ANNA_NODE_IDENTIFIER) 
		{
		    anna_node_identifier_t *def =
			node_cast_identifier(decl->child[2]);
		    if(wcscmp(def->name,L"__variadic__") == 0)
		    {
			is_variadic = 1;
			if(i != (declarations->child_count-1))
			{
			    FAIL(def, L"Only the last argument to a function can be variadic");
			}
/*			else
			{
			    wprintf(L"Variadic function\n");
			}
*/
		    }
		}
	    }
	    else if(wcscmp(fun->name, L"__function__") == 0)
	    {
		anna_node_t *fun_node = 
		    anna_node_prepare((anna_node_t *)decl, function, &list);
		CHECK_NODE_TYPE(fun_node, ANNA_NODE_DUMMY);
		anna_node_dummy_t *fun_dummy = (anna_node_dummy_t *)fun_node;
		anna_function_t *fun = anna_function_unwrap(
		    fun_dummy->payload);
                CHECK(fun, decl, L"Could not parse function declaration");              
                argv[i+!!type] = anna_function_wrap(fun)->type;
                argn[i+!!type] = fun->name;

/*
                anna_node_t *fun_wrap = anna_prepare_function_interface(
                    0, decl, function, 
                    parent, 0);

                CHECK_NODE_TYPE(fun_wrap, ANNA_NODE_DUMMY);
                anna_node_dummy_t *fun_dummy = (anna_node_dummy_t *)fun_wrap;
                anna_function_t *fun = anna_function_unwrap(fun_dummy->payload);
                CHECK(fun, decl, L"Could not parse function declaration");              
                argv[i+!!type] = anna_function_wrap(fun)->type;
                argn[i+!!type] = fun->name;

*/
	    }
	    else 
	    {
		FAIL(decl, L"Unknown declaration type");
	    }
	    
	}
    }

    if(body->node_type == ANNA_NODE_CALL) {
	function->body = (anna_node_call_t *)body;
    }
    else if(body->node_type == ANNA_NODE_DUMMY)
    {
	anna_node_dummy_t *body_dummy = (anna_node_dummy_t *)body;
	function->native = (anna_native_t)(anna_native_function_t)body_dummy->payload;	
    }
    else
    {
	function->native = (anna_native_t)anna_i_null_function;	
    }
    
    function->flags |= (is_variadic?ANNA_FUNCTION_VARIADIC:0);
    function->flags |= ANNA_FUNCTION_PREPARED_INTERFACE;
    function->name = wcsdup(name);
    function->return_type = out_type;
    function->input_count = argc;
    function->input_name = argn;
    function->input_type = argv;
    
    if(!is_anonymous)
    {
	anna_stack_declare(
	    function->stack_template->parent, 
	    name, 
	    anna_type_for_function(
		function->return_type,
		function->input_count,
		function->input_type,
		function->input_name,
		is_variadic),
	    anna_function_wrap(function));
    }
    return (anna_node_t *)anna_node_dummy_create(&node->location, anna_function_wrap(function),0);
}


void anna_prepare_function(anna_function_t *function)
{
    anna_prepare_function_internal(function, 0);
}

static int anna_function_check_dependencies(
    anna_function_t *function,
    prepare_prev_t *node)
{
    if(!node)
    {
	return 0;
    }
    if(node->function == function)
    {
	anna_error(
	    (anna_node_t *)function->body, 
	    L"Circular dependency for function %ls", function->name);
	return 1;
    }
    return anna_function_check_dependencies(function, node->prev);
    
}

static void anna_prepare_function_internal(
    anna_function_t *function,
    prepare_prev_t *dep)
{
    int i;
    anna_node_list_t list = 
	{
	    (anna_node_t *)function->body, 0, 0
	}
    ;
/*
    prepare_prev_t current =
	{
	    dep,
	    function,
	    0
	}
    ;
*/
    if(anna_function_prepared(function))
	return;
    
    if(anna_function_check_dependencies(function, dep))
	return;

    //twprintf(L"WOOWEE Prepare function %ls\n", function->name);
        
    function->flags |= ANNA_FUNCTION_PREPARED_IMPLEMENTATION;
        
    for(i=0; i<function->body->child_count; i++) 
    {
	list.idx=i;
	function->body->child[i] = anna_node_prepare(function->body->child[i], function, &list);
    }
    /*
    wprintf(L"Body of function %ls after preparation:\n", function->name);
    anna_node_print(function->body);
    */
    for(i=0; i<function->body->child_count; i++) 
    {
	anna_node_validate(function->body->child[i], function->stack_template);
    }
    
    for(i=0; i<al_get_count(&function->child_function); i++) 
    {
	//anna_function_t *func = (anna_function_t *)al_get(&function->child_function, i);
/*
	wprintf(L"Prepare subfunction %d of %d in function %ls: %ls\n", 
		i, al_get_count(&function->child_function),
		function->name, func->name);
*/	
//	if(func->flags & ANNA_FUNCTION_MACRO)
//	    anna_prepare_function_internal(func, &current);
    }
    
    for(i=0; i<al_get_count(&function->child_type); i++) 
    {
	//anna_type_t *type = (anna_type_t *)al_get(&function->child_type, i);
/*	wprintf(L"Prepare subfunction %d of %d in function %ls: %ls\n", 
		i, al_get_count(&function->child_function),
		function->name, func->name);
*/
//	anna_prepare_type_interface_internal(type, 
//					     &current);
    }
    
    for(i=0; i<al_get_count(&function->child_function); i++) 
    {
	//anna_function_t *func = (anna_function_t *)al_get(&function->child_function, i);
/*
	wprintf(L"Prepare subfunction %d of %d in function %ls: %ls\n", 
		i, al_get_count(&function->child_function),
		function->name, func->name);
*/	
//	if(!(func->flags & ANNA_FUNCTION_MACRO))
//	    anna_prepare_function_internal(func, &current);
    }
    
    if(!function->return_type)
	anna_sniff_return_type(function);
    
    if(!function->flags) {
	assert(function->return_type);
    }
 
    /*
      wprintf(L"Function after preparations:\n");
      anna_node_print(function->body);
      wprintf(L"\n");
    */
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
   
    if(node->node_type == ANNA_NODE_DUMMY) {
	anna_node_dummy_t *dummy = (anna_node_dummy_t *)node;
	//anna_object_print(dummy->payload);
	return dummy->payload->type;
//      return anna_type_unwrap(dummy->payload);      
    }
    if(
	node->node_type != ANNA_NODE_IDENTIFIER &&
	node->node_type != ANNA_NODE_IDENTIFIER_TRAMPOLINE) 
    {
	anna_error(node,L"Could not determine type of node of type %d", node->node_type);
	anna_node_print(node);
	return 0;
    }
    
    anna_node_identifier_t *id = (anna_node_identifier_t *)node;
    anna_object_t *wrapper = anna_stack_get_str(function->stack_template, id->name);
    return anna_type_unwrap(wrapper);
}

static anna_node_t *anna_type_member(anna_type_t *type,
				     struct anna_node_call *node, 
				     struct anna_function *function,
				     struct anna_node_list *parent)
{
    CHECK_CHILD_COUNT(node,L"variable declaration", 3);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    anna_node_prepare_children(node, function, parent);
    anna_node_identifier_t *name_identifier = node_cast_identifier(node->child[0]);
    anna_type_t *var_type;
    switch(node->child[1]->node_type) 
    {
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *type_identifier;
	    type_identifier = node_cast_identifier(node->child[1]);
	    anna_object_t *type_wrapper = anna_stack_get_str(function->stack_template, type_identifier->name);
	    assert(type_wrapper);
	    var_type = anna_type_unwrap(type_wrapper);
	    break;
	}
	
	case ANNA_NODE_NULL:	
	    var_type = anna_node_get_return_type(node->child[2], function->stack_template);
	    //wprintf(L"Implicit var dec type: %ls\n", type->name);
	    break;

	default:
	    FAIL(node->child[1], L"Wrong type on second argument to declare - expected an identifier or a null node");
    }
    
    assert(var_type);

    anna_member_create(type, -1, name_identifier->name, 0, var_type);
    return anna_node_null_create(0);
    
    //anna_stack_declare(function->stack_template, name_identifier->name, type, null_object);
    
    /*
      anna_node_t *a_param[2]=
      {
      node->child[0],
      node->child[2]
      }
      ;
    
      return (anna_node_t *)
      anna_node_call_create(&node->location,
      (anna_node_t *)anna_node_identifier_create(&node->location,
      L"__assign__"),
      2,
      a_param);
    */
}

anna_node_t *anna_prepare_type_interface(
    anna_type_t *type)
{
    return anna_prepare_type_interface_internal(type, 0);
}

static anna_node_t *anna_prepare_type_interface_internal(
    anna_type_t *type, 
    prepare_prev_t *dep)
{
    
    if(anna_type_prepared(type))
	return 0;
    
    type->flags |= ANNA_TYPE_PREPARED_INTERFACE;

    anna_function_t *function;
    
    function = anna_native_create(
	L"!typePrepareFunction",
	ANNA_FUNCTION_MACRO,
	(anna_native_t)(anna_native_function_t)0,
	0,
	0,
	0, 
	0);

    function->stack_template=type->stack;
    
    //wprintf(L"Prepare type %ls\n", type->name);
    //anna_node_print(type->definition);
//	wprintf(L"\n");

    anna_node_call_t *node = (anna_node_call_t *)anna_node_clone_deep((anna_node_t *)type->definition);

    CHECK_CHILD_COUNT(node,L"type macro", 4);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[3]);
    CHECK_NODE_BLOCK(node->child[2]);
    
    anna_node_call_t *attribute_list = 
        (anna_node_call_t *)node->child[2];
    int i;

    array_list_t property_list;
    al_init(&property_list);
    
    for(i=0; i<attribute_list->child_count;i++)
    {
	CHECK_NODE_TYPE(attribute_list->child[i], ANNA_NODE_CALL);
	anna_node_call_t *attribute = 
	    (anna_node_call_t *)attribute_list->child[i];
	CHECK_NODE_TYPE(attribute->function, ANNA_NODE_IDENTIFIER);
	
	anna_node_identifier_t *id = 
	    (anna_node_identifier_t *)attribute->function;
	
	string_buffer_t sb;
	sb_init(&sb);
	sb_append(&sb, L"__");
	sb_append(&sb, id->name);
	sb_append(&sb, L"Attribute__");
	wchar_t *name = sb_content(&sb);
	
	anna_node_call_t *attribute_call_node =
	    anna_node_call_create(&attribute->location,
				  (anna_node_t *)anna_node_identifier_create(&attribute->location,
									     name),
				  0,
				  0);
	
	anna_node_call_add_child(
	    attribute_call_node,
	    (anna_node_t *)attribute);
	anna_node_call_add_child(
	    attribute_call_node, 
	    (anna_node_t *)node);
	anna_function_t *macro_definition = anna_node_macro_get(
	    attribute_call_node, 
	    function->stack_template);
	CHECK(macro_definition, id, L"No such attribute macro found: %ls", name);
	
	anna_node_t *tmp = anna_macro_invoke(
	    macro_definition,
	    attribute_call_node,
	    function,
	    0);
	CHECK_NODE_TYPE(tmp, ANNA_NODE_CALL);
	node = (anna_node_call_t *)tmp;
	/*
(anna_node_call_t *)macro_definition->native.macro(
	    attribute_call_node, 
	    function,
	    parent);
	*/
	CHECK(node->node_type == ANNA_NODE_CALL, attribute_list, L"Attribute call %ls did not return a valid type definition", id->name);
	sb_destroy(&sb);	
    }
    
    int error_count=0;
    
    anna_node_call_t *body = (anna_node_call_t *)node->child[3];
    
    for(i=0; i<body->child_count; i++)
    {
	anna_node_t *item = body->child[i];
	
	if(item->node_type != ANNA_NODE_CALL) 
	{
	    anna_error(
		item,
		L"Only function declarations and variable declarations allowed directly inside a class body" );
	    error_count++;
	    continue;
	}
	
	anna_node_call_t *call = (anna_node_call_t *)item;
	if(call->function->node_type != ANNA_NODE_IDENTIFIER)
	{
	    anna_error(
		call->function,
		L"Only function declarations and variable declarations allowed directly inside a class body" ); 
	    error_count++;
	    continue;
	}
	
	anna_node_identifier_t *declaration = 
	    (anna_node_identifier_t *)call->function;
	
	if(wcscmp(declaration->name, L"__function__")==0)
	{
	    anna_macro_function_internal(type, call, function, 0, 0);
	}
	else if(wcscmp(declaration->name, L"__declare__")==0)
	{
	    anna_type_member(type, call, function, 0);
	}
	else if(wcscmp(declaration->name, L"__functionNative__")==0)
	{
	    int i;
	    int argc;
	    wchar_t **argn;
	    anna_type_t **argv;
	    
	    anna_node_identifier_t *name = 
		(anna_node_identifier_t *)call->child[0];

	    anna_type_t *return_type = call->child[1]->node_type == ANNA_NODE_NULL?0:
		anna_prepare_type_from_identifier(
		    call->child[1], 	
		    function,
		    0);

	    anna_node_call_t *param_list = 
		(anna_node_call_t *)call->child[2];

	    argc = param_list->child_count;
	    argv = malloc(sizeof(anna_type_t *)*argc);
	    argn = malloc(sizeof(wchar_t *)*argc); 
	    for(i=0; i<argc; i++)
	    {
		anna_node_call_t *param =
		    (anna_node_call_t *)param_list->child[i];
		anna_node_identifier_t *param_name = 
		    (anna_node_identifier_t *)param->child[0];
		argv[i] = anna_prepare_type_from_identifier(
		    param->child[1],
		    function,
		    0);
		argn[i] = param_name->name;
	    }
	    
	    anna_node_int_literal_t *mid = 
		(anna_node_int_literal_t *)call->child[3];

	    anna_node_int_literal_t *flags = 
		(anna_node_int_literal_t *)call->child[4];

	    anna_node_dummy_t *func = 
		(anna_node_dummy_t *)call->child[5];

	    anna_native_method_create(
		type,
		(size_t)mid->payload,
		name->name,
		flags->payload,
		(anna_native_t)(anna_native_function_t)func->payload,
		return_type,
		argc,
		argv,
		argn);
	    free(argv);
	}
	else if(wcscmp(declaration->name, L"__declareNative__")==0)
	{

	    anna_node_identifier_t *name = 
		(anna_node_identifier_t *)call->child[0];

	    anna_type_t *return_type = 
		anna_prepare_type_from_identifier(
		    call->child[1], 	
		    function,
		    0);

	    anna_node_int_literal_t *mid = 
		(anna_node_int_literal_t *)call->child[2];

	    anna_node_int_literal_t *is_static = 
		(anna_node_int_literal_t *)call->child[3];

	    
	    anna_member_create(
		type,
		mid->payload,
		name->name,
		is_static->payload,
		return_type);
	}
	else if(wcscmp(declaration->name, L"__property__")==0)
	{
	    al_push(&property_list, item);
	}
    	else
	{
	    anna_error(call->function,
		       L"Only function declarations and variable declarations allowed directly inside a class body" );
	    error_count++;	    
	}
    }
    for(i=0; i<al_get_count(&property_list); i++)
    {
	anna_node_call_t *prop = al_get(&property_list, i);
	CHECK_CHILD_COUNT(prop,L"property", 3);
	//anna_node_prepare_child(prop, 0, function, 0);
	anna_node_prepare_child(prop, 1, function, 0);
	
	CHECK_NODE_TYPE(prop->child[0], ANNA_NODE_IDENTIFIER);
	CHECK_NODE_TYPE(prop->child[1], ANNA_NODE_IDENTIFIER);
	CHECK_NODE_TYPE(prop->child[2], ANNA_NODE_CALL);
	
	anna_node_identifier_t *name = 
	    (anna_node_identifier_t *)prop->child[0];
	
	anna_type_t *p_type = 
	    anna_prepare_type_from_identifier(
		prop->child[1], 	
		function,
		0);
	
	//wprintf(L"Wee, declare %ls\n", name->name);
	
	anna_member_create(type,
			   -1,
			   name->name,
			   0,
			   p_type);
	type->member_count--;
	type->property_count++;
	anna_member_t *memb = anna_type_member_info_get(type, name->name);
	memb->is_property = 1;
	
	anna_node_call_t *attribute_list = (anna_node_call_t *)prop->child[2];
	int j;
	CHECK_NODE_IDENTIFIER_NAME(attribute_list->function, L"__block__");
	
	for(j=0; j<attribute_list->child_count; j++)
	{
//	   anna_node_print(attribute_list->child[j]);
	   CHECK_NODE_TYPE(attribute_list->child[j], ANNA_NODE_CALL);
	   anna_node_call_t *attribute = (anna_node_call_t *)attribute_list->child[j];
	   CHECK_CHILD_COUNT(attribute, L"setter or getter", 1);
	   CHECK_NODE_TYPE(attribute->function, ANNA_NODE_IDENTIFIER);
	   CHECK_NODE_TYPE(attribute->child[0], ANNA_NODE_IDENTIFIER);
	   
	    anna_node_identifier_t *a_type = 
		(anna_node_identifier_t *)attribute->function;
	    
	    anna_node_identifier_t *m_name = 
		(anna_node_identifier_t *)attribute->child[0];
	    
	    anna_member_t *method = anna_type_member_info_get(type, m_name->name);
	    CHECK(method, m_name, L"Unknown method \"%ls\" in class \"%ls\"", m_name->name, type->name);

	    /*
	      Fixme: Check that [gs]etter has correct signature
	    */

	    if(wcscmp(L"getter", a_type->name) == 0)
	    {
	       memb->getter_offset = method->offset;
	    }
	    else if(wcscmp(L"setter", a_type->name) == 0)
	    {
	       memb->setter_offset = method->offset;
	    }
	    else
	    {
	       FAIL(a_type, L"Unknown attribute");
	    }
	
	}
    }
    
    if(error_count)
	return (anna_node_t *)anna_node_null_create(&node->location);
    
    anna_object_t **constructor_ptr = 
	anna_static_member_addr_get_mid(type, ANNA_MID_INIT_PAYLOAD);
    
    if(constructor_ptr)
    {
	anna_function_t *constructor = 
	    anna_function_unwrap(*constructor_ptr);
    
	anna_type_t **argv= malloc(sizeof(anna_type_t *)*(constructor->input_count));
	wchar_t **argn= malloc(sizeof(wchar_t *)*(constructor->input_count));
	argv[0]=type_type;
	argn[0]=L"this";
	
	for(i=1; i<constructor->input_count; i++)
	{
	    argv[i] = constructor->input_type[i];
	    argn[i] = constructor->input_name[i];
	}
    }
    
    memcpy(
	&type->child_function, 
	&function->child_function,
	sizeof(array_list_t));
    
    memcpy(
	&type->child_type, 
	&function->child_type,
	sizeof(array_list_t));
    
/*
  wprintf(L"Base type after transformations\n");
  anna_node_print(type->definition);
  wprintf(L"\n");
*/  
    return (anna_node_t *)anna_node_dummy_create(&node->location,
						 anna_type_wrap(type),
						 0);
/*    
      wprintf(L"Create __call__ for non-native type %ls\n", type->name);
*/  
}

void anna_prepare_type_implementation(anna_type_t *type)
{
    int i;
    type->flags |= ANNA_TYPE_PREPARED_IMPLEMENTATION;
    for(i=0; i<al_get_count(&type->child_function); i++)
    {
	anna_function_t *fun = 
	    (anna_function_t *)al_get(
		&type->child_function, i);
	anna_prepare_function(fun);
    }
    
}

void anna_prepare_function_recursive(anna_function_t *block)
{
    anna_prepare_function(block);
    int i;
    
    for(i=0; i<al_get_count(&block->child_function); i++) 
    {
	anna_function_t *func = (anna_function_t *)al_get(&block->child_function, i);
	anna_prepare_function_recursive(func);
    }
}

void anna_prepare_internal()
{
    int i;
    int again=0;
    
    int function_count = al_get_count(&anna_function_list); 
    int type_count = al_get_count(&anna_type_list); 

    /*
      Prepare all macros and their subblocks. 
     */
    for(i=0; i<function_count; i++)
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
    for(i=0; i<type_count; i++)
    {
	anna_type_t *type = (anna_type_t *)al_get(&anna_type_list, i);
	
	if(!(type->flags & ANNA_TYPE_REGISTERED))
	{
	    type->flags |= ANNA_TYPE_REGISTERED;
	    //wprintf(L"Register type %ls\n", type->name);
	    anna_stack_declare(type->stack,
			       type->name,
			       type_type,
			       anna_type_wrap(type));
	}
    }
    
    
    
    /*
      Prepare interfaces of all known types
     */
    for(i=0; i<type_count; i++)
    {
	anna_type_t *type = (anna_type_t *)al_get(&anna_type_list, i);
	
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
    for(i=0; i<function_count; i++)
    {
	anna_function_t *func = (anna_function_t *)al_get(&anna_function_list, i);
/*
	wprintf(L"Prepare subfunction %d of %d in function %ls: %ls\n", 
		i, al_get_count(&function->child_function),
		function->name, func->name);
*/	
	if((!(func->flags & ANNA_FUNCTION_MACRO)) 
	   && (func->body)
	   && !(func->flags &ANNA_FUNCTION_PREPARED_INTERFACE))
	{
	    again=1;
	    anna_prepare_function_interface(func);
	}
    }
    
    for(i=0; i<function_count; i++)
    {
	anna_function_t *func = (anna_function_t *)al_get(&anna_function_list, i);
/*
	wprintf(L"Prepare subfunction %d of %d in function %ls: %ls\n", 
		i, al_get_count(&function->child_function),
		function->name, func->name);
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
    for(i=0; i<type_count; i++)
    {
	anna_type_t *type = (anna_type_t *)al_get(&anna_type_list, i);
	if(!(type->flags & ANNA_TYPE_PREPARED_IMPLEMENTATION))
	{
	    again=1;
	    anna_prepare_type_implementation(type);
	}
    }
    
    if(again)
    {
	wprintf(L"Again\n");
	
	anna_prepare_internal();
    }
}


void anna_prepare()
{
    anna_member_create(
	type_type,
	ANNA_MID_TYPE_WRAPPER_PAYLOAD,
	L"!typeWrapperPayload",
	0,
	null_type);
    anna_prepare_internal();

}
