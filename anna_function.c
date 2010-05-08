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

array_list_t anna_function_list = {0,0,0};

static anna_type_t *fake_function_type=0;

static void anna_function_wrapper_create(anna_function_t *f)
{
    if(!fake_function_type)
    {
	fake_function_type = anna_type_create(L"!FakeFunctionType", stack_global);
	fake_function_type->member_count = 2;
	fake_function_type->flags = 
	    ANNA_TYPE_REGISTERED | 
	    ANNA_TYPE_PREPARED_INTERFACE | 
	    ANNA_TYPE_PREPARED_IMPLEMENTATION;
	fake_function_type->mid_identifier = malloc(sizeof(anna_member_t *)*(ANNA_MID_FUNCTION_WRAPPER_PAYLOAD+1));
	anna_member_t *m = malloc(sizeof(anna_member_t));
	m->is_static = 0;
	m->offset = 0;
	fake_function_type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_PAYLOAD] = m;
	
    }
    f->wrapper = anna_object_create(fake_function_type);    
    f->wrapper->member[0] = (anna_object_t *)f;
    f->wrapper->member[1] = 0;
}

void anna_function_setup_type(anna_function_t *f, anna_stack_frame_t *location)
{
    if(!f->return_type)
    {
	wprintf(L"Critical: Function %ls lacks return type at setup time\n", f->name);
	CRASH;
	
    }
    
    
    anna_type_t *function_type = 
	anna_type_for_function(
	    f->return_type,
	    f->input_count,
	    f->input_type,
	    f->input_name,
	    ANNA_IS_VARIADIC(f));
    /*
      FIXME: The function->type field can probably be safely
      removed. Just use function->wrapper->type.
    */
    f->type = function_type;    
    f->wrapper->type = f->type;

    if(f->member_of)
    {

	anna_member_redeclare(
	    f->member_of,
	    f->mid,
	    f->type);
    }
        
    memcpy(
	anna_member_addr_get_mid(
	    f->wrapper,
	    ANNA_MID_FUNCTION_WRAPPER_PAYLOAD), 
	&f,
	sizeof(anna_function_t *));
    
    if(f->stack_template && f->stack_template->parent)
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

    if(f->member_of)
    {
	
    }
    else
    {
	if(!(f->flags & ANNA_FUNCTION_ANONYMOUS) && location && (!f->member_of))
	{
	    //wprintf(L"WOOWEEWOO, declare %ls\n", f->name);
	    anna_stack_declare(location, f->name, f->type, anna_function_wrap(f));
	}
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
//     FIXME: Is there any validity checking we could do here?
  
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

int anna_function_prepared(anna_function_t *t)
{
    return !!(t->flags & ANNA_FUNCTION_PREPARED_IMPLEMENTATION);
}

anna_function_t *anna_function_create(
    wchar_t *name,
    int flags,
    anna_node_call_t *body, 
    anna_type_t *return_type,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn,
    anna_stack_frame_t *parent_stack,
    int return_pop_count)
{
    int i;
    
    if(!(flags & ANNA_FUNCTION_MACRO)) {
	//assert(return_type);
	if(argc) {
	    assert(argv);
	    assert(argn);
	}
	for(i=0;i<argc; i++)
	{
	    assert(argv[i]);
	    assert(argn[i]);
	}
    }
    
    anna_function_t *result = calloc(
	1,sizeof(anna_function_t));
    result->input_type = calloc(1, sizeof(anna_type_t *)*argc);
//    if(flags & 
    result->native.function=0;
    result->flags=flags;
    result->flags |= ANNA_FUNCTION_PREPARED_INTERFACE;
    result->name = wcsdup(name);
    result->body = body;
    result->return_type=return_type;
    result->input_count=argc;
    result->return_pop_count = return_pop_count;
    result->this=0;
    al_push(&anna_function_list, result);
    
    if(!(flags & ANNA_FUNCTION_MACRO)) {
	result->input_name = malloc(argc*sizeof(wchar_t *));;
	memcpy(result->input_name, argn, sizeof(wchar_t *)*argc);
    }
    else
    {
	result->input_name = malloc(3*sizeof(wchar_t *));;
	memcpy(result->input_name, argn, sizeof(wchar_t *)*3);	
    }
        
    result->stack_template = anna_stack_create(64, parent_stack);
    
    result->stack_template->function = result;

    if(!(flags & ANNA_FUNCTION_MACRO)) 
    {
	int is_variadic = ANNA_IS_VARIADIC(result);
	memcpy(result->input_type, argv, sizeof(anna_type_t *)*argc);
	for(i=0; i<argc-is_variadic;i++)
	{
	    anna_stack_declare(
		result->stack_template,
		argn[i], 
		argv[i], 
		null_object);	
	}    
	if(is_variadic)
	{
	    /*
	      FIXME:
	      Templatize to right list subtype
	    */
	    anna_stack_declare(
		result->stack_template, 
		argn[argc-1], 
		list_type, 
		null_object);
	}
    }
    else
    {
	anna_stack_declare(
	    result->stack_template, 
	    argn[0], 
	    node_call_wrapper_type,
	    null_object);
    }
    
    //anna_function_prepare(result);
    
    anna_function_wrapper_create(result);
    anna_function_setup_type(result, parent_stack);
    
    //wprintf(L"Function object is %d, wrapper is %d\n", result, result->wrapper);
    /*
    wprintf(L"Created a new non-native function with definition:");
    anna_node_print(result->body);
    */
    
    return result;
}

anna_function_t *anna_function_create_from_definition(
    anna_node_call_t *definition,
    anna_stack_frame_t *scope)
{

    anna_function_t *result = calloc(
	1,
	sizeof(anna_function_t));
    result->definition = definition;
    result->stack_template = anna_stack_create(64, scope);
    result->return_pop_count = 1;
    al_push(&anna_function_list, result);

    wchar_t *name=0;
    if (definition->child[0]->node_type == ANNA_NODE_IDENTIFIER) 
    {	
	anna_node_identifier_t *name_identifier = (anna_node_identifier_t *)definition->child[0];
	name = name_identifier->name;
/*
	wprintf(L"Creating function '%ls' from ast\n", name);
	anna_node_print(definition);
*/	
    }
    else {
	if(definition->child[0]->node_type != ANNA_NODE_NULL)
	{
	    anna_error((anna_node_t *)definition, L"Invalid function name");
	    return 0;
	}
	name = L"!anonymousFunc";
	result->flags |= ANNA_FUNCTION_ANONYMOUS;
    }
    result->name = wcsdup(name);


/*
    wprintf(L"LALALAGGG\n");
    anna_node_print(definition);
*/  
    anna_function_wrapper_create(result);
    
#ifdef ANNA_CHECK_STACK_ENABLED
    result->stack_template->function = result;
#endif
    
    return result;
}

anna_function_t *anna_function_create_from_block(
    struct anna_node_call *body,
    anna_stack_frame_t *scope,
    int pop_count)
{
    
    anna_node_t *param[] = 
	{
	    (anna_node_t *)anna_node_null_create(&body->location),
	    (anna_node_t *)anna_node_null_create(&body->location),
	    (anna_node_t *)anna_node_block_create(&body->location),
	    (anna_node_t *)anna_node_block_create(&body->location),
	    (anna_node_t *)body
	}
    ;
    anna_node_call_t *definition = anna_node_call_create(
	&body->location,
	(anna_node_t *)anna_node_identifier_create(&body->location, L"__function__"),
	5,
	param);
    anna_function_t *result = anna_function_create_from_definition(
	definition,
	scope);
    result->return_pop_count = pop_count;
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
    if(!flags) {
	assert(return_type);
	if(argc) {
	    assert(argv);
	    assert(argn);
	}
    }
  
    anna_function_t *result = calloc(
	1,sizeof(anna_function_t));
    result->input_type = calloc(1, sizeof(anna_type_t *)*argc);

    result->flags=flags;
    result->flags |= ANNA_FUNCTION_PREPARED_INTERFACE;
    result->native = native;
    result->name = name;
    result->return_type=return_type;
    result->input_count=argc;
    memcpy(
	result->input_type,
	argv, 
	sizeof(anna_type_t *)*argc);
    result->input_name = argn;
    al_push(&anna_function_list, result);
    
    anna_function_wrapper_create(result);
    anna_function_setup_type(result, location);
        
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
	wprintf(L"function %ls...\n", function->name);
	wprintf(L"function %ls %ls(", function->return_type->name, function->name);
	int i;
	for(i=0;i<function->input_count; i++)
	{
	    wprintf(L"%ls %ls;", function->input_type[i]->name, function->input_name[i]);
	}
	wprintf(L")\n");
    }
    
    

}

