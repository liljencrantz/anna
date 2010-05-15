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
#include "anna_node_wrapper.h"
#include "anna_node_create.h"
#include "anna_list.h"

anna_object_t *anna_function_invoke_values(anna_function_t *function, 
					   anna_object_t *this,
					   anna_object_t **param,
					   anna_stack_frame_t *outer)
{
    if(function->flags & ANNA_FUNCTION_MACRO) 
    {
	wprintf(L"FATAL: Macro %ls at invoke!!!!\n", function->name);
	CRASH;
    }
    else
    {
	if(function->native.function) 
	{
	    anna_object_t **argv=param;
	    int i;
		
	    int offset=0;
	    if(this)
	    {
		argv=malloc(sizeof(anna_object_t *)*function->input_count);
		offset=1;
		argv[0]=this;		    
		for(i=0; i<(function->input_count-offset); i++) 
		{
		    argv[i+offset]=param[i];
		}
	    }
	    //wprintf(L"Invoking function with %d params\n", function->input_count);
	    
	    return function->native.function(argv);
	}
	else 
	{
	    anna_object_t *result = null_object;
	    int i;
	    anna_stack_frame_t *my_stack = anna_stack_clone(function->stack_template);//function->input_count+1);
	    my_stack->parent = outer?outer:stack_global;
	    /*
	    wprintf(L"Create new stack %d with frame depth %d while calling function %ls\n", my_stack, anna_stack_depth(my_stack), function->name);
	    wprintf(L"Run non-native function %ls with %d params and definition:\n", function->name, function->input_count);
	    anna_node_print(function->body);
	    */
	    int offset=0;
	    if(this)
	    {
		offset=1;
		anna_stack_declare(my_stack, 
				   function->input_name[0],
				   function->input_type[0],
				   this,
				   0);
	    }
		
	    for(i=0; i<(function->input_count-offset); i++) 
	    {
/*
  wprintf(L"Declare input variable %d with name %ls on stack\n",
  i, function->input_name[i+offset]);
*/
		anna_stack_set_str(my_stack, 
				   function->input_name[i+offset],
				   param[i]);
	    }
	    
	    for(i=0; i<function->body->child_count && !my_stack->stop; i++)
	    {
		/*
		wprintf(L"Run node %d of function %ls\n",
			i, function->name);
	      
		anna_node_print(function->body->child[i]);
		*/
		result = anna_node_invoke(function->body->child[i], my_stack);
		//wprintf(L"Result is %d\n", result);
	      
	    }
	    /*
	      if(my_stack->stop) 
	      {
	      }
	    */
	    return result;
	}
    }
}

struct anna_node *anna_macro_invoke(
    anna_function_t *macro, 
    anna_node_call_t *node,
    anna_function_t *function,
    anna_node_list_t *parent)
{
    if(macro->native.macro)
    {
	return macro->native.macro(
	    node, function, parent);
    }
    else
    {
	int i;
	anna_stack_frame_t *my_stack = anna_stack_clone(macro->stack_template);
	anna_object_t *result = null_object;
	
	anna_stack_set_str(my_stack,
			   macro->input_name[0],
			   anna_node_wrap((anna_node_t *)node));
	for(i=0; i<macro->body->child_count && !my_stack->stop; i++)
	{
	    result = anna_node_invoke(macro->body->child[i], my_stack);
	}
	if(result == null_object)
	{
	    return anna_node_create_null(&node->location);
	}
	return anna_node_unwrap(result);
    }
}

anna_object_t *anna_function_invoke(anna_function_t *function, 
				    anna_object_t *this,
				    anna_node_call_t *param, 
				    anna_stack_frame_t *param_invoke_stack,
				    anna_stack_frame_t *outer) 
{
/*
    wprintf(L"Executing function %ls\n", function->name);
    if(function->body)
    {
	anna_node_print(function->body);
    }
*/  
    if(!this)
    {
	this=function->this;
    }
    
    anna_object_t **argv;
    //wprintf(L"anna_function_invoke %ls %d; %d params\n", function->name, function, function->input_count);    
    if(likely(function->input_count < 8))
    {
	anna_object_t *argv_c[8];
	argv=argv_c;
    }
    else
    {
	argv=malloc(sizeof(anna_object_t *)*function->input_count);
    }
    
    int i;
    //wprintf(L"Argv: %d\n", argv);
    
    int offset=0;
    if(this)
    {	    
	offset=1;
	argv[0]=this;		    
	//wprintf(L"We have a this parameter: %d\n", this);
    }
    int is_variadic = ANNA_IS_VARIADIC(function);
    //  wprintf(L"Function %ls has variadic flag set to %d\n", function->name, function->flags);
    
    for(i=0; i<(function->input_count-offset-is_variadic); i++)
    {
//wprintf(L"eval param %d of %d \n", i, function->input_count - is_variadic - offset);
	argv[i+offset]=anna_node_invoke(param->child[i], param_invoke_stack);
    }

    if(is_variadic)
    {
	anna_object_t *lst = anna_list_create();
	int variadic_count = param->child_count - i;
	if(variadic_count < 0)
	{
	    anna_error(
		(anna_node_t *)param,
		L"Critical: Tried to call function %ls with %d arguments, need at least %d\n",
		function->name,
		param->child_count,
		function->input_count-1+offset);
	    CRASH;	    
	}
	
	anna_list_set_capacity(lst, variadic_count);
	for(; i<param->child_count;i++)
	{
	    //  wprintf(L"eval variadic param %d of %d \n", i, param->child_count);
	    anna_list_add(lst, anna_node_invoke(param->child[i], param_invoke_stack));
	}
	//wprintf(L"Set variadic var at offset %d to %d\n", function->input_count-1, lst);
	
	argv[function->input_count-1] = lst;	    
    }
    if(!likely(function->input_count < 8))
    {
	free(argv);
    }
    return anna_function_invoke_values(function, 0, argv, outer);
    
}

