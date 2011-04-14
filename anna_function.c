#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "anna_function.h"
#include "anna_type.h"
#include "anna_macro.h"
#include "anna_node_wrapper.h"
#include "anna_node_create.h"
#include "anna_node_check.h"
#include "anna_util.h"
#include "anna_alloc.h"
#include "anna_vm.h"
#include "anna_intern.h"
#include "anna_attribute.h"

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
	    (anna_node_t *)anna_node_create_dummy(0, anna_type_wrap(type));
    }
}

static anna_node_t *anna_function_setup_arguments(
    anna_function_t *f,
    anna_stack_template_t *parent_stack)
{
    if(f->input_type)
    {
	int i;
	for(i=0; i<f->input_count; i++)
	{
	    anna_stack_declare(
		f->stack_template, 
		f->input_name[i],
		f->input_type[i],
		null_object,
		0);
	}
	return 0;   
    }
    

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
    
//    wprintf(L"%d arguments!\n", argc);
    
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

	    argn[i] = anna_intern(name->name);		

	    anna_node_t *type_node = anna_node_macro_expand(decl->child[1], parent_stack);
	    anna_node_t *val_node = anna_node_macro_expand(decl->child[2], parent_stack);
	    
	    if(type_node->node_type == ANNA_NODE_IDENTIFIER)
	    {
		anna_node_identifier_t *type_name =
		    node_cast_identifier(type_node);
		
		anna_object_t **type_wrapper =
		    anna_stack_addr_get(parent_stack, type_name->name);

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
		anna_error(decl->child[1],  L"Could not determine argument type of %ls in function %ls", name->name, f->name);
//		anna_node_print(4, decl->child[1]);
//		anna_node_print(4, type_node);
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
    }
    return 0;
}

static void anna_function_setup_wrapper(
    anna_function_t *f)
{    
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
	
	memset(
	    anna_member_addr_get_mid(
		f->wrapper,
		ANNA_MID_FUNCTION_WRAPPER_STACK),
	    0,
	    sizeof(anna_stack_template_t *));
    }
}    

void anna_function_setup_interface(
    anna_function_t *f,
    anna_stack_template_t *parent_stack)
{
    if(f->flags & ANNA_FUNCTION_PREPARED_INTERFACE)
    {
	return;
    }    
    f->flags |= ANNA_FUNCTION_PREPARED_INTERFACE;
    
//    wprintf(L"Set up interface for function/macro %ls\n", f->name);
    
    if(f->body)
    {
	f->stack_template = anna_stack_create(parent_stack);
	
	anna_function_setup_arguments(f, parent_stack);
	
	anna_node_register_declarations(
	    f->stack_template, 
	    (anna_node_t *)f->body);

/*
	wprintf(
	    L"Function's internal declarations registered (%d)\n",
	    f->stack_template->count);
	anna_node_print(0, f->body);
*/	
	f->stack_template->function = f;
    }

    
    if(!f->return_type)
    {
	
	anna_node_t *return_type_node = f->definition->child[1];
	if(return_type_node->node_type == ANNA_NODE_IDENTIFIER)
	{
	    anna_node_identifier_t *rti = (anna_node_identifier_t *)return_type_node;
	    anna_object_t *rto = anna_stack_get(parent_stack, rti->name);
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
		if(last_expression->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
		{
		    return;
		}
		f->return_type = last_expression->return_type;
	    }
	}
	else
	{
	    anna_error(return_type_node, L"Don't know how to handle function definition return type node");
	    return;
	}
    }
    
    anna_function_setup_wrapper(f);
    
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
	array_list_t ret = AL_STATIC;
	int i;
	anna_node_calculate_type_children( f->body, f->stack_template);
	anna_node_find((anna_node_t *)f->body, ANNA_NODE_RETURN, &ret);	
	int step_count = 0;
	anna_function_t *fptr = f;
	
	while(fptr->flags & ANNA_FUNCTION_BLOCK)
	{
	    step_count++;
	    fptr = fptr->stack_template->parent->function;
	    if(!fptr)
	    {
		anna_error(f->definition, L"Blocks must be definied inside a function");
		break;
	    }
	}
	
	for(i=0; i<al_get_count(&ret); i++)
	{
	    anna_node_wrapper_t *wr = (anna_node_wrapper_t *)al_get(&ret, i);
	    wr->steps = step_count;
	}
	al_destroy(&ret);
    }
}

anna_object_t *anna_function_wrap(anna_function_t *result)
{
#ifdef ANNA_WRAPPER_CHECK_ENABLED
    if(!result->wrapper)
    {
	wprintf(
	    L"Critical: Tried to wrap a function with no wrapper\n");
	CRASH;
    }
#endif
    return result->wrapper;
}

anna_function_type_t *anna_function_unwrap_type(anna_type_t *type)
{
    if(!type)
    {
	wprintf(L"Critical: Tried to get function from non-existing type\n");
	CRASH;
    }
    
    //wprintf(L"Find function signature for call %ls\n", type->name);
    
    anna_function_type_t **function_ptr = 
	(anna_function_type_t **)anna_static_member_addr_get_mid(
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

static anna_node_call_t *anna_function_attribute(anna_function_t *fun)
{
    return fun->attribute;    
}

int anna_function_has_alias(anna_function_t *fun, wchar_t *name)
{
    return anna_attribute_has_alias(
	anna_function_attribute(fun),
	name);    
}

int anna_function_has_alias_reverse(anna_function_t *fun, wchar_t *name)
{
    return anna_attribute_has_alias_reverse(
	anna_function_attribute(fun),
	name);
}

void anna_function_alias_add(anna_function_t *fun, wchar_t *name)
{
    anna_node_call_t *attr = anna_node_create_call2(
	0,
	anna_node_create_identifier(0, L"alias"),
	anna_node_create_identifier(0, name));
    anna_node_call_add_child(fun->attribute, (anna_node_t *)attr);
}

void anna_function_alias_reverse_add(anna_function_t *fun, wchar_t *name)
{
    anna_node_call_t *attr = anna_node_create_call2(
	0,
	anna_node_create_identifier(0, L"aliasReverse"),
	anna_node_create_identifier(0, name));
    anna_node_call_add_child(fun->attribute, (anna_node_t *)attr);
}

anna_function_t *anna_function_create_from_definition(
    anna_node_call_t *definition)
{
    anna_function_t *result = anna_alloc_function();
    
    result->definition = definition;
    result->attribute = (anna_node_call_t *)definition->child[3];

    wchar_t *name=0;
    if (definition->child[0]->node_type == ANNA_NODE_IDENTIFIER) 
    {	
	anna_node_identifier_t *name_identifier = (anna_node_identifier_t *)definition->child[0];
	name = name_identifier->name;
	result->name = anna_intern(name);
    }
    else {
	result->name = L"<anonymous>";
    }
    result->body = node_cast_call(result->definition->child[4]);

    return result;
}

static void anna_function_attribute_empty(anna_function_t *fun)
{
    fun->attribute = anna_node_create_block2(0);
}


anna_function_t *anna_macro_create(
    wchar_t *name,
    struct anna_node_call *definition,
    wchar_t *arg_name)
{
    assert(arg_name);
    
    anna_function_t *result = anna_alloc_function();
    anna_function_attribute_empty(result);
    
    result->definition = definition;
    result->body = (anna_node_call_t *)definition->child[2];
    result->name = anna_intern(name);
    
    result->return_type = node_wrapper_type;
    result->flags |= ANNA_FUNCTION_MACRO;
    result->input_count=1;
    
    result->input_name = calloc(sizeof(wchar_t *), 1);
    result->input_name[0] = anna_intern(arg_name);
    
    result->input_type = calloc(sizeof(anna_type_t *), 1);
    result->input_type[0] = node_call_wrapper_type;

//    anna_function_setup_wrapper(result);
    return result;
}


anna_function_t *anna_function_create_from_block(
    struct anna_node_call *body)
{
    anna_node_call_t *definition = anna_node_create_call2(
	&body->location,
	anna_node_create_identifier(&body->location, L"__function__"),
	anna_node_create_null(&body->location), //Name
	anna_node_create_null(&body->location), //Return type
	anna_node_create_block2(&body->location),//Declaration list
	anna_node_create_block2(&body->location),//Attribute list
	body);
    anna_function_t *result = anna_function_create_from_definition(
	definition);
    result->flags |= ANNA_FUNCTION_BLOCK;
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
    anna_stack_template_t *location)
{
    int i;
    
    if(!(flags & ANNA_FUNCTION_MACRO)) {
	assert(return_type);
	if(argc) {
	    assert(argv);
	    assert(argn);
	}
    }
  
    anna_function_t *result = anna_alloc_function();
    anna_function_attribute_empty(result);    
    result->input_type = calloc(1, sizeof(anna_type_t *)*argc);
    result->input_name = calloc(1, sizeof(wchar_t *)*argc);

    result->flags |= flags;
    result->native = native;
    result->name = anna_intern(name);
    result->return_type=return_type;
    result->input_count=argc;
    memcpy(
	result->input_type,
	argv, 
	sizeof(anna_type_t *)*argc);
    for(i=0;i<argc; i++)
    {
	result->input_name[i] = anna_intern(argn[i]);	
    }
    
    anna_function_setup_interface(result, location);        
    //wprintf(L"Creating function %ls @ %d with macro flag %d\n", result->name, result, result->flags);
    anna_vm_compile(result);
    
    return result;
}

static anna_vmstack_t *anna_function_continuation(anna_vmstack_t *stack, anna_object_t *cont)
{
    anna_object_t *res = anna_vmstack_pop(stack);
    anna_vmstack_pop(stack);
    
    stack = (anna_vmstack_t *)*anna_member_addr_get_mid(cont, ANNA_MID_CONTINUATION_STACK);
    stack->code = (char *)*anna_member_addr_get_mid(cont, ANNA_MID_CONTINUATION_CODE_POS);
    anna_vmstack_push(stack, res);
    return stack;
}

anna_function_t *anna_continuation_create(
    anna_vmstack_t *stack,
    anna_type_t *return_type)
{
    anna_function_t *result = anna_alloc_function();
    result->flags = ANNA_FUNCTION_CONTINUATION;
    anna_function_attribute_empty(result);    
    result->input_type = 0;
    result->input_name = 0;
    
    result->native = anna_function_continuation;
    result->name = anna_intern_static(L"!continuation");
    result->return_type=return_type;
    result->input_count=0;
    
    anna_function_setup_interface(result, stack_global);
    anna_vm_compile(result);
    
    return result;
}

anna_function_t *anna_method_wrapper_create(
    anna_vmstack_t *stack,
    anna_type_t *return_type)
{
    anna_function_t *result = anna_alloc_function();
    result->flags = ANNA_FUNCTION_METHOD_WRAPPER;
    anna_function_attribute_empty(result);
    result->input_type = 0;
    result->input_name = 0;
    
    result->native = anna_vm_method_wrapper;
    result->name = anna_intern_static(L"!methodWrapper");
    result->return_type=return_type;
    result->input_count=0;
    
    anna_function_setup_interface(result, stack_global);
    anna_vm_compile(result);
    
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
