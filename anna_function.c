#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna_function.h"
#include "anna_macro.h"
#include "anna_node_wrapper.h"
#include "anna_node_wrapper.h"

array_list_t anna_function_list = {0,0,0};

void anna_function_setup_type(anna_function_t *f)
{
    anna_type_t *function_type = 
	anna_type_for_function(
	    f->return_type,
	    f->input_count,
	    f->input_type,
	    f->input_name,
	    ANNA_IS_VARIADIC(f));
    f->type = function_type;    

    f->wrapper = anna_object_create(f->type);
    
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
    return !!(t->flags & ANNA_FUNCTION_PREPARED);
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
	1,sizeof(anna_function_t) + argc*sizeof(anna_type_t *));
//    if(flags & 
    result->native.function=0;
    result->flags=flags;
    result->name = wcsdup(name);
    result->body = body;
    result->return_type=return_type;
    result->input_count=argc;
    result->return_pop_count = return_pop_count;
    result->this=0;
    al_push(&anna_function_list, result);
    
    result->input_name = malloc(argc*sizeof(wchar_t *));;
    memcpy(result->input_name, argn, sizeof(wchar_t *)*argc);

    result->stack_template = anna_stack_create(64, parent_stack);

#ifdef ANNA_CHECK_STACK_ENABLED
    result->stack_template->function = result;
#endif

    if(!(flags & ANNA_FUNCTION_MACRO)) 
    {
	int is_variadic = ANNA_IS_VARIADIC(result);
	memcpy(&result->input_type, argv, sizeof(anna_type_t *)*argc);
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

    anna_function_setup_type(result);
    
    //wprintf(L"Function object is %d, wrapper is %d\n", result, result->wrapper);
    /*
    wprintf(L"Created a new non-native function with definition:");
    anna_node_print(result->body);
    */

    return result;
}

anna_function_t *anna_native_create(wchar_t *name,
				    int flags,
				    anna_native_t native, 
				    anna_type_t *return_type,
				    size_t argc,
				    anna_type_t **argv,
				    wchar_t **argn)
{
    if(!flags) {
	assert(return_type);
	if(argc) {
	    assert(argv);
	    assert(argn);
	}
    }
  
    anna_function_t *result =
	calloc(
	    1,
	    sizeof(anna_function_t) + argc*sizeof(anna_type_t *));
    result->flags=flags;
    result->native = native;
    result->name = name;
    result->return_type=return_type;
    result->input_count=argc;
    memcpy(
	&result->input_type,
	argv, 
	sizeof(anna_type_t *)*argc);
    result->input_name = argn;
    al_push(&anna_function_list, result);
    
    anna_function_setup_type(result);
        
    //wprintf(L"Creating function %ls @ %d with macro flag %d\n", result->name, result, result->flags);

    return result;
}


