#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna_function.h"
#include "anna_type.h"
#include "anna_macro.h"
#include "anna_node_wrapper.h"
#include "anna_node_create.h"
#include "anna_node_check.h"
#include "anna_util.h"

void anna_function_argument_hint(
    anna_function_t *f,
    int argument,
    anna_type_t *type)
{
    anna_node_call_t *declarations = node_cast_call(f->definition->child[2]);
    anna_node_call_t *declaration = node_cast_call(declarations->child[argument]);
    if(declaration->child[1]->node_type == ANNA_NODE_NULL)
    {
	declaration->child[1] = 
	    (anna_node_t *)anna_node_create_dummy(0, anna_type_wrap(type), 0);
    }
}

static anna_node_t *anna_function_setup_arguments(
    anna_function_t *f,
    anna_stack_frame_t *parent_stack)
{
//    wprintf(L"Setup function %ls\n", f->name);
    CHECK_NODE_TYPE(f->definition->child[2], ANNA_NODE_CALL);
    anna_node_call_t *declarations = node_cast_call(f->definition->child[2]);
    int i;
    f->input_count = declarations->child_count;
        
    int argc = declarations->child_count;
    
//    wprintf(
//	L"Adding input arguments to function\n");
    
    
    anna_type_t **argv = f->input_type = malloc(sizeof(anna_type_t *)*argc);
    wchar_t **argn = f->input_name = malloc(sizeof(wchar_t *)*argc);
        
    for(i=0; i<argc; i++)
    {
	//declarations->child[i] = anna_node_prepare(declarations->child[i], function, parent);
	CHECK_NODE_TYPE(declarations->child[i], ANNA_NODE_CALL);
	anna_node_call_t *decl = node_cast_call(declarations->child[i]);
	
	CHECK_NODE_TYPE(decl->function, ANNA_NODE_IDENTIFIER);
	anna_node_identifier_t *fun = node_cast_identifier(decl->function);
	if(wcscmp(fun->name, L"__var__") == 0 || wcscmp(fun->name, L"__const__") == 0)
	{
	    //CHECK_CHILD_COUNT(decl, L"variable declaration", 3);
	    CHECK_NODE_TYPE(decl->child[0], ANNA_NODE_IDENTIFIER);
	
	    anna_node_identifier_t *name = 
		node_cast_identifier(
		    decl->child[0]);

	    argn[i] = name->name;		

	    anna_node_t *type_node = anna_node_macro_expand(decl->child[1], parent_stack);
	    anna_node_t *val_node = anna_node_macro_expand(decl->child[2], parent_stack);
	    
	    if(type_node->node_type == ANNA_NODE_IDENTIFIER)
	    {
	    
		anna_node_identifier_t *type_name =
		    node_cast_identifier(type_node);
		
		anna_object_t **type_wrapper =
		    anna_stack_addr_get_str(parent_stack, type_name->name);

		CHECK(
		    type_wrapper, 
		    (anna_node_t *)type_name,
		    L"Unknown type: %ls",
		    type_name->name);

		argv[i] = 
		    anna_type_unwrap(*type_wrapper);

	    }
	    else if(val_node->node_type == ANNA_NODE_CLOSURE)
	    {
		anna_node_closure_t *cl = (anna_node_closure_t *)val_node;
		anna_function_t *derp = cl->payload;
		anna_function_setup_interface(derp, parent_stack);
		argv[i] = derp->wrapper->type;
	    }
	    else if(type_node->node_type == ANNA_NODE_DUMMY)
	    {
		anna_node_dummy_t *cl = (anna_node_dummy_t *)type_node;
		anna_type_t *derp = anna_type_unwrap(cl->payload);
		argv[i] = derp;
	    }
	    else
	    {
		anna_error(decl->child[1],  L"Could not determine argument type");
		anna_node_print(decl->child[1]);
		anna_node_print(type_node);
		CRASH;
	    }
	    
	    
	    
	    anna_stack_declare(
		f->stack_template, 
		argn[i],
		argv[i],
		null_object,
		0);
	    
//	    wprintf(L"Adding %ls\n", name->name);
	}
	else
	{
	    wprintf(L"Expected declaration: %ls\n", fun->name);
	    CRASH;
	}
	
/*	    
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
		    }
		}
	    }
*/
	    
    }
    return 0;
}

void anna_function_setup_interface(
    anna_function_t *f,
    anna_stack_frame_t *parent_stack)
{
    if(f->flags & ANNA_FUNCTION_PREPARED_INTERFACE)
    {
	return;
    }    
    f->flags |= ANNA_FUNCTION_PREPARED_INTERFACE;
    
//    wprintf(L"Set up interface for function/macro %ls\n", f->name);
    
    if(f->body)
    {
	size_t declaration_count = 1;
	if(!(f->flags & ANNA_FUNCTION_MACRO)){
	    anna_node_call_t *declarations = node_cast_call(f->definition->child[2]);
	    declaration_count = declarations->child_count;
	}
	f->stack_template = anna_node_register_declarations(
	    (anna_node_t *)f->body, declaration_count);
	f->stack_template->parent = parent_stack;

	if(f->flags & ANNA_FUNCTION_MACRO){
	    anna_stack_declare(f->stack_template, f->input_name[0], f->input_type[0], null_object, 0);
	    
	}
/*
	wprintf(
	    L"Function's internal declarations registered (%d)\n",
	    f->stack_template->count);
	anna_node_print(f->body);
*/	
#ifdef ANNA_CHECK_STACK_ENABLED
	f->stack_template->function = f;
#endif
    }
    
    if(!f->return_type)
    {
	anna_function_setup_arguments(f, parent_stack);
	
	anna_node_t *return_type_node = f->definition->child[1];
	if(return_type_node->node_type == ANNA_NODE_IDENTIFIER)
	{
	    anna_node_identifier_t *rti = (anna_node_identifier_t *)return_type_node;
	    anna_object_t *rto = anna_stack_get_str(parent_stack, rti->name);
	    if(!rto)
	    {
		anna_error(return_type_node, L"Unknown return type: %ls", rti->name);
		return;	
	    }
	    
	    f->return_type = anna_type_unwrap(rto);
	    
	    if(!f->return_type)
	    {
		anna_error(return_type_node, L"Return type is not a type: %ls", rti->name);
		return;	
	    }
	}
	else if(return_type_node->node_type == ANNA_NODE_NULL)
	{
//	    wprintf(L"Function %ls has unspecified return type, we need to investigate\n", f->name);
	    
	    if(f->body->child_count == 0)
	    {
		f->return_type = null_type;
	    }
	    else
	    {
		anna_node_t *last_expression = f->body->child[f->body->child_count-1];
		anna_node_calculate_type(last_expression, f->stack_template);
		f->return_type = last_expression->return_type;
	    }
	}
	else
	{
	    anna_error(return_type_node, L"Don't know how to handle function definition return type node");
	    return;
	}
    }
    
    if(!f->wrapper){
	anna_type_t *ft = 
	    anna_type_for_function(
		f->return_type,
		f->input_count,
		f->input_type,
		f->input_name,
		f->flags);
	
	f->wrapper = anna_object_create(ft);    
	memcpy(
	    anna_member_addr_get_mid(
		f->wrapper,
		ANNA_MID_FUNCTION_WRAPPER_PAYLOAD), 
	    &f,
	    sizeof(anna_function_t *));
	
	if(f->stack_template)
	{
	    memcpy(
		anna_member_addr_get_mid(
		    f->wrapper,
		    ANNA_MID_FUNCTION_WRAPPER_STACK),
		&f->stack_template->parent,
		sizeof(anna_stack_frame_t *));
	}
	else
	{
	    memset(
		anna_member_addr_get_mid(
		    f->wrapper,
		    ANNA_MID_FUNCTION_WRAPPER_STACK),
		0,
		sizeof(anna_stack_frame_t *));
	}
    }
}

void anna_function_setup_body(
    anna_function_t *f)
{
    if(f->flags & ANNA_FUNCTION_PREPARED_BODY)
    {
	return;
    }    
    f->flags |= ANNA_FUNCTION_PREPARED_BODY;
    
    if(f->body)
    {
	int i;
	for(i=0; i<f->body->child_count; i++)
	    anna_node_each((anna_node_t *)f->body->child[i], (anna_node_function_t)&anna_node_calculate_type, f->stack_template);
	anna_node_each((anna_node_t *)f->body, (anna_node_function_t)&anna_node_prepare_body,f->stack_template);
    }
}

anna_object_t *anna_function_wrap(anna_function_t *result)
{
    return result->wrapper;
}

anna_function_t *anna_function_unwrap(anna_object_t *obj)
{
#ifdef ANNA_WRAPPER_CHECK_ENABLED
    if(!obj)
    {
	wprintf(
	    L"Critical: Tried to unwrap null pointer as a function\n");
	CRASH;
    }
#endif

    anna_function_t **function_ptr = 
	(anna_function_t **)anna_member_addr_get_mid(
	    obj,
	    ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    if(function_ptr) 
    {
	//wprintf(L"Got object of type %ls with native method payload\n", obj->type->name);
	return *function_ptr;
    }
    else 
    {
	anna_object_t **function_wrapper_ptr =
	    anna_static_member_addr_get_mid(
		obj->type, 
		ANNA_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    //wprintf(L"Got object with __call__ member\n");
	    return anna_function_unwrap(
		*function_wrapper_ptr);	    
	}
	return 0;	
    }
}

anna_function_type_key_t *anna_function_unwrap_type(anna_type_t *type)
{
    if(!type)
    {
	wprintf(L"Critical: Tried to get function from non-existing type\n");
	CRASH;
    }
    
    //wprintf(L"Find function signature for call %ls\n", type->name);
    
    anna_function_type_key_t **function_ptr = 
	(anna_function_type_key_t **)anna_static_member_addr_get_mid(
	    type,
	    ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    if(function_ptr) 
    {
	//wprintf(L"Got member, has return type %ls\n", (*function_ptr)->result->name);
	return *function_ptr;
    }
    else 
    {
	//wprintf(L"Not a direct function, check for __call__ member\n");
	anna_object_t **function_wrapper_ptr = 
	    anna_static_member_addr_get_mid(
		type,
		ANNA_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    //wprintf(L"Found, we're unwrapping it now\n");
	    return anna_function_unwrap_type((*function_wrapper_ptr)->type);	    
	}
	return 0;	
    }
//     FIXME: Is there any validity checking we could do here?
  
}

anna_function_t *anna_function_create_from_definition(
    anna_node_call_t *definition)
{
    anna_function_t *result = calloc(
	1,
	sizeof(anna_function_t));
    result->definition = definition;
    //result->stack_template = anna_stack_create(64, 0);
    //al_push(&anna_function_list, result);

    wchar_t *name=0;
    if (definition->child[0]->node_type == ANNA_NODE_IDENTIFIER) 
    {	
	anna_node_identifier_t *name_identifier = (anna_node_identifier_t *)definition->child[0];
	name = name_identifier->name;
	result->name = wcsdup(name);
/*
	wprintf(L"Creating function '%ls' from ast\n", name);
	anna_node_print(definition);
*/	
    }
    else {
	result->name = wcsdup(L"<anonymous>");
    }
    result->body = node_cast_call(result->definition->child[4]);
    
/*
    wprintf(L"LALALAGGG\n");
    anna_node_print(definition);
*/  
    
    return result;
}

anna_function_t *anna_macro_create(
    wchar_t *name,
    struct anna_node_call *definition,
    wchar_t *arg_name)
{
    anna_function_t *result = calloc(
	1,
	sizeof(anna_function_t));
    result->definition = definition;
    result->body = (anna_node_call_t *)definition->child[2];
    result->name = wcsdup(name);
    wchar_t **argn=calloc(sizeof(wchar_t *), 1);
    anna_type_t **argv=calloc(sizeof(anna_type_t *), 1);
    argv[0] = node_wrapper_type;
    argn[0] = wcsdup(arg_name);
    result->return_type = node_wrapper_type;
    result->flags = ANNA_FUNCTION_MACRO;
    result->input_count=1;
    result->input_name = argn;
    result->input_type = argv;
    return result;
    
}


anna_function_t *anna_function_create_from_block(
    struct anna_node_call *body)
{
    
    anna_node_t *param[] = 
	{
	    (anna_node_t *)anna_node_create_null(&body->location), //Name
	    (anna_node_t *)anna_node_create_null(&body->location), //Return type
	    (anna_node_t *)anna_node_create_block(&body->location, 0, 0),//Declaration list
	    (anna_node_t *)anna_node_create_block(&body->location, 0, 0),//Attribute list
	    (anna_node_t *)body
	}
    ;
    anna_node_call_t *definition = anna_node_create_call(
	&body->location,
	(anna_node_t *)anna_node_create_identifier(&body->location, L"__function__"),
	5,
	param);
    anna_function_t *result = anna_function_create_from_definition(
	definition);
    return result;
    
}

anna_function_t *anna_native_create(
    wchar_t *name,
    int flags,
    anna_native_t native, 
    anna_type_t *return_type,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn,
    anna_stack_frame_t *location)
{
    int i;
    
    if(!(flags & ANNA_FUNCTION_MACRO)) {
	assert(return_type);
	if(argc) {
	    assert(argv);
	    assert(argn);
	}
    }
  
    anna_function_t *result = calloc(
	1,sizeof(anna_function_t));
    result->input_type = calloc(1, sizeof(anna_type_t *)*argc);
    result->input_name = calloc(1, sizeof(wchar_t *)*argc);

    result->flags=flags;
    result->native = native;
    result->name = wcsdup(name);
    result->return_type=return_type;
    result->input_count=argc;
    memcpy(
	result->input_type,
	argv, 
	sizeof(anna_type_t *)*argc);
    for(i=0;i<argc; i++)
    {
	result->input_name[i] = wcsdup(argn[i]);
	
    }
    
    anna_function_setup_interface(result, location);        
    //wprintf(L"Creating function %ls @ %d with macro flag %d\n", result->name, result, result->flags);
    
    return result;
}

void anna_function_print(anna_function_t *function)
{
    if(function->flags & ANNA_FUNCTION_MACRO)
    {
	wprintf(L"macro %ls(...)", function->name);	
    }
    else
    {
	wprintf(L"function %ls %ls(", function->return_type->name, function->name);
	int i;
	for(i=0;i<function->input_count; i++)
	{
	    wprintf(L"%ls %ls;", function->input_type[i]->name, function->input_name[i]);
	}
	wprintf(L")\n");
    }   

}
