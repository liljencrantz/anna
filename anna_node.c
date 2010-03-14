#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "wutil.h"
#include "anna_node.h"
#include "anna_int.h"
#include "anna_float.h"
#include "anna_string.h"
#include "anna_char.h"

#define check(node, test, ...) if(!(test)) anna_error(node, __VA_ARGS__)

void anna_node_set_location(anna_node_t *node, anna_location_t *l)
{
    if(l)    
    {
	memcpy(&node->location, l, sizeof(anna_location_t));
    }
    else
    {
	memset(&node->location, 0, sizeof(anna_location_t));	
    }
    
}

anna_node_call_t *node_cast_call(anna_node_t *node) 
{
    
    if(node->node_type!=ANNA_NODE_CALL)
    {
	anna_error(node, L"Expected a call node");
	CRASH;
    }
    
    return (anna_node_call_t *)node;
}

anna_node_identifier_t *node_cast_identifier(anna_node_t *node) 
{
    assert(node->node_type==ANNA_NODE_IDENTIFIER);
    return (anna_node_identifier_t *)node;
}

anna_node_int_literal_t *node_cast_int_literal(anna_node_t *node) 
{
    assert(node->node_type==ANNA_NODE_INT_LITERAL);
    return (anna_node_int_literal_t *)node;
}

anna_node_string_literal_t *node_cast_string_literal(anna_node_t *node) 
{
    assert(node->node_type==ANNA_NODE_STRING_LITERAL);
    return (anna_node_string_literal_t *)node;
}

anna_node_dummy_t *anna_node_dummy_create(anna_location_t *loc, struct anna_object *val, int is_trampoline)
{
   anna_node_dummy_t *result = calloc(1,sizeof(anna_node_dummy_t));
   result->node_type = is_trampoline?ANNA_NODE_TRAMPOLINE:ANNA_NODE_DUMMY;
   anna_node_set_location((anna_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;  
}

anna_node_return_t *anna_node_return_create(anna_location_t *loc, struct anna_node *val, int steps)
{
   anna_node_return_t *result = calloc(1,sizeof(anna_node_return_t));
   result->node_type = ANNA_NODE_RETURN;
   anna_node_set_location((anna_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   result->steps=steps;
   return result;  
}

anna_node_member_get_t *anna_node_member_get_create(anna_location_t *loc, struct anna_node *object, size_t mid, struct anna_type *type, int wrap)
{
   anna_node_member_get_t *result = calloc(1,sizeof(anna_node_member_get_t));
   result->node_type = wrap?ANNA_NODE_MEMBER_GET_WRAP:ANNA_NODE_MEMBER_GET;
   anna_node_set_location((anna_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->object=object;
   result->mid=mid;
   result->type=type;
   return result;  
  
}

anna_node_member_set_t *anna_node_member_set_create(anna_location_t *loc, struct anna_node *object, size_t mid, struct anna_node *value, struct anna_type *type)
{
   anna_node_member_set_t *result = calloc(1,sizeof(anna_node_member_set_t));
   result->node_type = ANNA_NODE_MEMBER_SET;
   anna_node_set_location((anna_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->object=object;
   result->value=value;
   result->mid=mid;
   result->type=type;
   return result;  
    
}



anna_node_assign_t *anna_node_assign_create(anna_location_t *loc, anna_sid_t sid, struct anna_node *value)
{
   anna_node_assign_t *result = calloc(1,sizeof(anna_node_assign_t));
   result->node_type = ANNA_NODE_ASSIGN;
   anna_node_set_location((anna_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->value=value;
   result->sid=sid;
   return result;  
}

anna_node_int_literal_t *anna_node_int_literal_create(anna_location_t *loc, int val)
{
   anna_node_int_literal_t *result = calloc(1,sizeof(anna_node_int_literal_t));
   result->node_type = ANNA_NODE_INT_LITERAL;
   anna_node_set_location((anna_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;
}

anna_node_float_literal_t *anna_node_float_literal_create(anna_location_t *loc, double val)
{
   anna_node_float_literal_t *result = calloc(1,sizeof(anna_node_float_literal_t));
   result->node_type = ANNA_NODE_FLOAT_LITERAL;
   anna_node_set_location((anna_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;
}

anna_node_char_literal_t *anna_node_char_literal_create(anna_location_t *loc, wchar_t val)
{
   anna_node_char_literal_t *result = calloc(1,sizeof(anna_node_char_literal_t));
   result->node_type = ANNA_NODE_CHAR_LITERAL;
   anna_node_set_location((anna_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;
}

anna_node_string_literal_t *anna_node_string_literal_create(anna_location_t *loc, size_t sz, wchar_t *str)
{
   anna_node_string_literal_t *result = calloc(1,sizeof(anna_node_string_literal_t));
   result->node_type = ANNA_NODE_STRING_LITERAL;
   anna_node_set_location((anna_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = str;
   result->payload_size = sz;
   return result;
}

anna_node_call_t *anna_node_call_create(anna_location_t *loc, anna_node_t *function, size_t argc, anna_node_t **argv)
{
    anna_node_call_t *result = calloc(1,sizeof(anna_node_call_t));
    result->child = calloc(1,sizeof(anna_node_t *)*(argc));
    result->node_type = ANNA_NODE_CALL;
    anna_node_set_location((anna_node_t *)result,loc);
    result->function = function;
    result->child_count = argc;
    result->child_capacity = argc;
    memcpy(result->child, argv, sizeof(anna_node_t *)*(argc));
    /*
      FIXME: Create a nice and tidy wrapper
    */
    return result;
}

anna_node_identifier_t *anna_node_identifier_create(anna_location_t *loc, wchar_t *name)
{
    anna_node_identifier_t *result = calloc(1,sizeof(anna_node_call_t));
    result->node_type = ANNA_NODE_IDENTIFIER;
    anna_node_set_location((anna_node_t *)result,loc);
    result->name = name;
    /*
      FIXME: Create a nice and tidy wrapper
    */
    return result;
}

anna_node_t *anna_node_null_create(anna_location_t *loc)
{
    anna_node_t *result = calloc(1,sizeof(anna_node_t));
    result->node_type = ANNA_NODE_NULL;
    anna_node_set_location((anna_node_t *)result,loc);
    /*
      FIXME: Create a nice and tidy wrapper
    */
    return result;
}

anna_node_call_t *anna_node_native_method_declare_create(
    anna_location_t *loc,
    ssize_t mid,
    wchar_t *name,
    int flags,
    anna_native_t func,
    anna_node_t *result,
    size_t argc,
    anna_node_t **argv,
    wchar_t **argn)
{

    anna_node_call_t *r =
	anna_node_call_create(
	    loc,
	    (anna_node_t *)anna_node_identifier_create(
		loc,
		L"__functionNative__"),	    
	    0,
	    0);

    anna_node_call_t *param_list = 
	anna_node_call_create(
	    loc,
	    (anna_node_t *)anna_node_identifier_create(
		loc,
		L"__block__"),
	    0,
	    0);

    int i;
    for(i=0;i<argc;i++)
    {
	anna_node_call_t *param =
	    anna_node_call_create(
		loc,
		(anna_node_t *)anna_node_identifier_create(
		    loc,
		    L"__block__"),
		
		0,
		0);
	anna_node_call_add_child(param, (anna_node_t *)anna_node_identifier_create(loc,argn[i]));
	anna_node_call_add_child(param, (anna_node_t *)argv[i]);
	anna_node_call_add_child(param_list, (anna_node_t *)param);
    }


    anna_node_call_add_child(r, (anna_node_t *)anna_node_identifier_create(loc,name));
    anna_node_call_add_child(r, result?result:anna_node_null_create(loc));
    anna_node_call_add_child(r, (anna_node_t *)param_list);
    anna_node_call_add_child(r, (anna_node_t *)anna_node_int_literal_create(loc,mid));
    anna_node_call_add_child(r, (anna_node_t *)anna_node_int_literal_create(loc,flags));
    anna_node_call_add_child(
	r, 
	(anna_node_t *)anna_node_dummy_create(
	    loc,
	    (anna_object_t *)func.function, 0));
/*
    anna_node_print(r);
    wprintf(L"\n\n");
*/  
    return r;
}


anna_node_call_t *anna_node_member_declare_create(
    anna_location_t *loc,
    ssize_t mid,
    wchar_t *name,
    int is_static,
    anna_node_t *member_type)
{
    anna_node_call_t *r =
	anna_node_call_create(
	    loc,
	    (anna_node_t *)anna_node_identifier_create(
		loc,
		L"__declareNative__"),	    
	    0,
	    0);


    anna_node_call_add_child(r, (anna_node_t *)anna_node_identifier_create(loc,name));
    anna_node_call_add_child(r, member_type);
    anna_node_call_add_child(r, (anna_node_t *)anna_node_int_literal_create(loc,mid));
    anna_node_call_add_child(r, (anna_node_t *)anna_node_int_literal_create(loc,is_static));

/*
    anna_node_print(r);
    wprintf(L"\n\n");
*/
    return r;
}



anna_node_t *anna_node_function_declaration_create(
   anna_location_t *loc,
   anna_node_t *result,
   size_t argc,
   anna_node_t **argv,
   wchar_t **argn)
{
   anna_node_t *a_argv[argc];
   int i;

   
   for(i=0; i<argc;i++)
   {
      anna_node_t *d_argv[] = 
	 {
	    (anna_node_t *)anna_node_identifier_create(loc, argn[i]),
	    argv[i]
	 }
      ;
      
      a_argv[i] = (anna_node_t *)anna_node_call_create(
	 loc,
	 (anna_node_t *)anna_node_identifier_create(loc, L"__declare__"),
	 2,
	 d_argv);
   }
   
   anna_node_call_t *argv_node =
      anna_node_call_create(
	 loc,
	 (anna_node_t *)anna_node_identifier_create(loc, L"__block__"),
	 argc,
	 a_argv);
      
   anna_node_t *f_argv[] = 
      {
	 anna_node_null_create(loc),
	 result,
	 (anna_node_t *)argv_node,
	 (anna_node_t *)anna_node_null_create(loc),
	 (anna_node_t *)anna_node_null_create(loc)
      }
   ;
   
   return (anna_node_t *)anna_node_call_create(
      loc,
      (anna_node_t *)anna_node_identifier_create(loc, L"__function__"),
      5,
      f_argv);
   
}

anna_node_t *anna_node_templated_type_create(
    anna_location_t *loc,
    anna_node_t *type,
    size_t argc,
    anna_node_t **argv)
{
   
   anna_node_call_t *argv_node =
      anna_node_call_create(
	 loc,
	 (anna_node_t *)anna_node_identifier_create(loc, L"__block__"),
	 argc,
	 argv);

   anna_node_t *t_argv[] = 
      {
	 type,
	 (anna_node_t *)argv_node
      }
   ;
   
   return (anna_node_t *)anna_node_call_create(
      loc,
      (anna_node_t *)anna_node_identifier_create(loc, L"__templatize__"),
      2,
      t_argv);
   
}





void anna_node_call_add_child(anna_node_call_t *call, anna_node_t *child)
{
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 8 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(anna_node_t *));
	if(!call->child) 
	{
	    wprintf(L"Out of memory\n");
	    exit(1);
	}	
	call->child_capacity = new_capacity;
    }
//    wprintf(L"LALALA %d %d\n", call->child_capacity, call->child_count);
    
    call->child[call->child_count++] = child;
}

void anna_node_call_prepend_child(anna_node_call_t *call, anna_node_t *child)
{
    if(call->child_count==0) 
    {
	anna_node_call_add_child(call, child);
	return;
    }
    
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 8 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(anna_node_t *));
	if(!call->child) 
	{
	    wprintf(L"Out of memory\n");
	    exit(1);
	}	
	call->child_capacity = new_capacity;
    }
//    wprintf(L"LALALA %d %d\n", call->child_capacity, call->child_count);
    memmove(&call->child[1], call->child[0], sizeof(anna_node_t *)*call->child_count);
    call->child[0] = child;
    call->child_count++;
}

void anna_node_call_set_function(anna_node_call_t *call, anna_node_t *function)
{
    call->function = function;
}

anna_function_t *anna_node_macro_get(anna_node_call_t *node, anna_stack_frame_t *stack)
{
/*
    wprintf(L"Checking for macros in node (%d)\n", node->function->node_type);
    anna_node_print(node);
    wprintf(L"\n");
*/
    switch(node->function->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *name=(anna_node_identifier_t *)node->function;

	    anna_object_t **obj = anna_stack_addr_get_str(stack, name->name);
	    if(obj && *obj != null_object)
	    {
		
		anna_function_t *func=anna_function_unwrap(*obj);
		//wprintf(L"Tried to find object %ls on stack, got %d, revealing internal function ptr %d\n", name->name, obj, func);
		
		if(func && func->flags == ANNA_FUNCTION_MACRO)
		{
		    return func;
		}
	    }
	    
	    break;
	}


	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *call=(anna_node_call_t *)node->function;
	    if(call->function->node_type != ANNA_NODE_IDENTIFIER)
	    {
		break;
	    }
	    anna_node_identifier_t *name=(anna_node_identifier_t *)call->function;	    
	    
	    if(wcscmp(name->name, L"__memberGet__") == 0)
	    {
//		wprintf(L"It's a member lookup\n");

		if(call->child_count == 2 && 
		   call->child[1]->node_type == ANNA_NODE_IDENTIFIER)
		{
		    anna_node_identifier_t *member_name=
			(anna_node_identifier_t *)call->child[1];
//		    wprintf(L"Looking up member %ls\n", member_name->name);
		    anna_object_t **obj = anna_stack_addr_get_str(stack, member_name->name);
		    if(obj)
		    {
			anna_function_t *func=anna_function_unwrap(*obj);
		    
			if(func && func->flags == ANNA_FUNCTION_MACRO)
			{
			    //wprintf(L"Found macro!\n");
			    
			    return func;
			}
		    }
		    
		}	
	    }
	}
	
/*	
	case ANNA_NODE_MEMBER_GET_WRAP:
	case ANNA_NODE_MEMBER_GET:
	{
	    wprintf(L"Looking for macro in member get node\n");
	    
	    anna_node_member_get_t *get = (anna_node_member_get_t *)node;
	    wchar_t *name = anna_mid_get_reverse(get->mid);
	    wprintf(L"Got a name! %ls\n", name);
	    anna_object_t **obj = anna_stack_addr_get_str(stack, name);
	    if(obj)
	    {
	       anna_function_t *func=anna_function_unwrap(*obj);
	    
	       if(func && func->flags == ANNA_FUNCTION_MACRO)
	       {
		  return func;
	       }
	    }
	    
	    break;
	}
*/	
	default:
	{
/*	    wprintf(L"Function is not an identifier, not a macro:\n");
	    anna_node_print(node->function);
	    wprintf(L"\n");
*/
	}
	
    }
    return 0;
    
}

anna_node_t *anna_node_call_prepare(
    anna_node_call_t *node, 
    anna_function_t *function,
    anna_node_list_t *parent)
{
    
    anna_node_list_t list = 
	{
	    (anna_node_t *)node, 0, parent
	}
    ;
/*
   wprintf(L"Prepare call node:\n");
   anna_node_print((anna_node_t *)node);
   wprintf(L"\n");
*/ 
   if(node->node_type == ANNA_NODE_CALL)
   {       
       anna_function_t *macro_definition = anna_node_macro_get(node, function->stack_template);
       
       if(macro_definition)
       {       
	   return anna_node_prepare(macro_definition->native.macro(node, function, parent), function, parent);
       }
       
       node->function = anna_node_prepare(node->function, function, parent);
       anna_type_t *func_type = anna_node_get_return_type(node->function, function->stack_template);       
       if(func_type == type_type)
       {
	   /*
	     Constructor!
	   */
	   node->node_type = ANNA_NODE_CONSTRUCT;
	   node->function = (anna_node_t *)anna_node_dummy_create(&node->location, 
								  anna_node_invoke(node->function, function->stack_template),
								  0);
	   node->function = anna_node_prepare(node->function, function, &list);
	   //wprintf(L"Woo, changing call into constructor!\n");
	   
	   return (anna_node_t *)node;
       }
   }
   else 
   {
      node->function = anna_node_prepare(node->function, function, &list);
   }

   int i;
   for(i=0; i<node->child_count; i++)
   {
      list.idx = i;
      node->child[i] = anna_node_prepare(node->child[i], function, &list);	 
   }
   return (anna_node_t *)node;
}

anna_object_t *anna_node_call_invoke(anna_node_call_t *this, anna_stack_frame_t *stack)
{
    //wprintf(L"anna_node_call_invoke with stack %d\n", stack);
    anna_object_t *obj = anna_node_invoke(this->function, stack);
    if(obj == null_object){
	return obj;
    }
    
    return anna_function_wrapped_invoke(obj, 0, this, stack);
}

anna_object_t *anna_node_int_literal_invoke(anna_node_int_literal_t *this, anna_stack_frame_t *stack)
{
  return anna_int_create(this->payload);
}

anna_object_t *anna_node_float_literal_invoke(anna_node_float_literal_t *this, anna_stack_frame_t *stack)
{
  return anna_float_create(this->payload);
}

anna_object_t *anna_node_string_literal_invoke(anna_node_string_literal_t *this, anna_stack_frame_t *stack)
{
  return anna_string_create(this->payload_size, this->payload);
}

anna_object_t *anna_node_char_literal_invoke(anna_node_char_literal_t *this, anna_stack_frame_t *stack)
{
  return anna_char_create(this->payload);
}

anna_object_t *anna_node_identifier_invoke(anna_node_identifier_t *this, anna_stack_frame_t *stack)
{
/*    wprintf(L"Lookup on string \"%ls\", sid is %d,%d\n", this->name, this->sid.frame, this->sid.offset);
    assert(anna_stack_get_sid(stack, this->sid) == anna_stack_get_str(stack, this->name));
    
    return anna_stack_get_sid(stack, this->sid);*/

  return anna_stack_get_str(stack, this->name);
}

anna_object_t *anna_node_assign_invoke(anna_node_assign_t *this, anna_stack_frame_t *stack)
{
   anna_object_t *result = anna_node_invoke(this->value, stack);
   anna_stack_set_sid(stack, this->sid, result);
   return result;
}

anna_type_t *anna_node_get_return_type(anna_node_t *this, anna_stack_frame_t *stack)
{
    switch(this->node_type)
    {
	case ANNA_NODE_CONSTRUCT:
	{
	    anna_node_call_t *this2 =(anna_node_call_t *)this;	    
	    return anna_type_unwrap(anna_node_invoke(this2->function, 0));
	}
	
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *this2 =(anna_node_call_t *)this;
/*	    
	    wprintf(L"Get return type of node\n");
	    anna_node_print(this);
	    wprintf(L"\n");
*/	    
	    anna_type_t *func_type = anna_node_get_return_type(this2->function, stack);
	    /*
	      Special case constructors...
	    */
	    if(func_type == type_type)
	    {
		if(this2->function->node_type == ANNA_NODE_IDENTIFIER)
		{
		    anna_node_identifier_t *identifier = (anna_node_identifier_t *)this2->function;
		    anna_object_t *type_wrapper = anna_stack_get_str(stack, identifier->name);
		    assert(type_wrapper);
		    return anna_type_unwrap(type_wrapper);
		}
		wprintf(L"Illigal init\n");
		CRASH;
	    }
	    else if(!func_type)
	    {
		wprintf(L"Tried to get return type of call node\n");
		anna_node_print(this);
		wprintf(L",\nbut function was of unknown type\n");
		CRASH;
	    }
	    
	    anna_function_type_key_t *function_data = anna_function_unwrap_type(func_type);
	    if(!function_data)
	    {
		anna_error(this, L"Could not determine return type of function call");
		return 0;
	    }
	    
	    if(!function_data->result)
	    {
		wprintf(L"Critical: Failed to determine return type of node\n");
		anna_node_print(this);
		wprintf(L"\n");
		CRASH;
		
	    }
	    
	    return function_data->result;
	}
	
	case ANNA_NODE_TRAMPOLINE:
	case ANNA_NODE_DUMMY:
	{
	   anna_node_dummy_t *this2 =(anna_node_dummy_t *)this;	    
	   return this2->payload->type;   
	}
	
	case ANNA_NODE_INT_LITERAL:
	    return int_type;

	case ANNA_NODE_FLOAT_LITERAL:
	    return float_type;

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *this2 =(anna_node_identifier_t *)this;	    
	    return anna_stack_get_type(stack, this2->name);
	}
	case ANNA_NODE_STRING_LITERAL:
	    return string_type;

	case ANNA_NODE_CHAR_LITERAL:
	    return char_type;

	case ANNA_NODE_NULL:
	    return null_type;

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_get_t *this2 =(anna_node_member_get_t *)this;
	    return this2->type;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_set_t *this2 =(anna_node_member_set_t *)this;
	    return this2->type;
	}

	case ANNA_NODE_RETURN:
	{
	    anna_node_return_t *this2 =(anna_node_return_t *)this;
	    return anna_node_get_return_type(this2->payload, stack);
	}	
	    
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *this2 =(anna_node_assign_t *)this;
	    return anna_node_get_return_type(this2->value, stack);
	}
	
	    
	default:
	    wprintf(L"SCRAP! Unknown node type when checking return type: %d\n", this->node_type);
	    exit(1);
    }
}

void anna_node_validate(anna_node_t *this, anna_stack_frame_t *stack)
{
    switch(this->node_type)
    {
	case ANNA_NODE_CONSTRUCT:
	{
	    /*
	      FIXME: Do some actual checking!
	    */
	    anna_node_call_t *this2 =(anna_node_call_t *)this;	    
	    int i;
	    
	    anna_node_validate(this2->function, stack);
	    for(i=0; i<this2->child_count; i++)
	    {
		anna_node_validate(this2->child[i], stack);
	    }

	    break;
	}
	
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *this2 =(anna_node_call_t *)this;	    
	    int i;
	    
	    anna_node_validate(this2->function, stack);
	    for(i=0; i<this2->child_count; i++)
	    {
		anna_node_validate(this2->child[i], stack);
	    }
	    
	    anna_type_t *func_type = anna_node_get_return_type(this2->function, stack);

	    anna_function_type_key_t *function_data = anna_function_unwrap_type(func_type);
	    if(!function_data)
	    {
		anna_error(this, 
			   L"Unknown function");
		return;
	    }
	    
	    int is_method = (this2->function->node_type == ANNA_NODE_MEMBER_GET_WRAP);	    	    
	    check(this, function_data->argc-is_method == this2->child_count,
		  L"Wrong number of paramaters in function call. Should be %d, not %d.", 
		  function_data->argc-is_method, this2->child_count);
	    
	    for(i=is_method; i<mini(this2->child_count, function_data->argc); i++)
	    {
		if(!function_data->argv[i])
		{
		    anna_error(this, 
			  L"Unknown function");
		    break;
		}
		
		anna_type_t *ctype = anna_node_get_return_type(this2->child[i-is_method], stack);
		
		if(ctype)
		{
		    //anna_node_print(this);
		    check(this, anna_abides(ctype, function_data->argv[i]),
			  L"Wrong type of argument %d of %d, %ls does not abide by %ls", i+1, function_data->argc, ctype->name,
			  function_data->argv[i]->name);
		}
		else
		{
		    anna_error(this2->child[i-is_method], L"Unknown type for for argument %d of function call", i+1);
		}
	    }
	    
	    return;
	}
	
	case ANNA_NODE_TRAMPOLINE:
	case ANNA_NODE_DUMMY:
	{
	   anna_node_dummy_t *this2 =(anna_node_dummy_t *)this;	    
	   return;
	}
	
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_NULL:
	    return;

	    

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *this2 =(anna_node_identifier_t *)this;	    
	    check(this, !!this2->name, L"Invalid identifier node");
	    check(this, !!anna_stack_get_type(stack, this2->name), L"Unknown variable: %ls", this2->name);
	    return;
	}
	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *this2 =(anna_node_string_literal_t *)this;	    
	    check(this, !!this2->payload, L"Invalid string node");
	    return;
	}

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_get_t *this2 =(anna_node_member_get_t *)this;
	    return;
	}
		    
	default:
	    check(this, 1, L"Unknown node type");
    }
}

anna_node_t *anna_node_prepare(anna_node_t *this, anna_function_t *function, anna_node_list_t *parent)
{
    anna_node_list_t list = 
	{
	    this, 0, parent
	}
    ;

    ANNA_PREPARED(this);    
   
    switch(this->node_type)
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	    return anna_node_call_prepare((anna_node_call_t *)this, function, parent);

	case ANNA_NODE_RETURN:
	{
	  anna_node_return_t * result = (anna_node_return_t *)this;
	  result->payload=anna_node_prepare(result->payload, function, &list);
	  return (anna_node_t *)result;
	}
	
	case ANNA_NODE_IDENTIFIER:
	{
	    /*
	      anna_node_identifier_t *this2 =(anna_node_identifier_t *)this;
	      this2->sid = anna_stack_sid_create(function->stack_template, this2->name);
	    */
	    return this;
	}	

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_get_t * result = (anna_node_member_get_t *)this;	    
	    result->object = anna_node_prepare(result->object, function, &list);
	    return this;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t * result = (anna_node_assign_t *)this;	    
	    result->value = anna_node_prepare(result->value, function, &list);
	    return this;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_set_t * result = (anna_node_member_set_t *)this;	    
	    result->value = anna_node_prepare(result->value, function, &list);
	    result->object = anna_node_prepare(result->object, function, &list);
	    return this;
	}


	case ANNA_NODE_TRAMPOLINE:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_NULL:
	    return this;   

	default:
	    wprintf(L"HULP %d\n", this->node_type);
	    exit(1);
    }
}

anna_object_t *anna_node_member_get_invoke(anna_node_member_get_t *this, 
					   anna_stack_frame_t *stack)
{
/*
    assert(this->object);
    assert(anna_node_invoke(this->object, stack));
    if(!anna_member_addr_get_mid(anna_node_invoke(this->object, stack), this->mid))
    {
	anna_error(this->object, L"Critical: Object %ls does not have a member %ls",
		   anna_node_invoke(this->object, stack)->type->name,
		   anna_mid_get_reverse(this->mid));
	
    }
*/  
    return *anna_member_addr_get_mid(anna_node_invoke(this->object, stack), this->mid);
}

anna_object_t *anna_node_member_set_invoke(anna_node_member_set_t *this, 
					   anna_stack_frame_t *stack)
{
    return (*anna_member_addr_get_mid(anna_node_invoke(this->object, stack), this->mid)) = anna_node_invoke(this->value, stack);
    //return *anna_member_addr_get_mid(anna_node_invoke(this->object, stack), this->mid);
}

anna_object_t *anna_node_member_get_wrap_invoke(anna_node_member_get_t *this, 
						anna_stack_frame_t *stack)
{
    anna_object_t *obj = anna_node_invoke(this->object, stack);
    return anna_method_wrap(*anna_member_addr_get_mid(obj, this->mid), obj);
}

static anna_object_t *anna_trampoline(anna_object_t *orig, 
				      anna_stack_frame_t *stack)
{
    anna_object_t *res = anna_object_create(orig->type);
    memcpy(anna_member_addr_get_mid(res,ANNA_MID_FUNCTION_WRAPPER_PAYLOAD),
	   anna_member_addr_get_mid(orig,ANNA_MID_FUNCTION_WRAPPER_PAYLOAD),
	   sizeof(anna_function_t *));    
    memcpy(anna_member_addr_get_mid(res,ANNA_MID_FUNCTION_WRAPPER_STACK),
	   &stack,
	   sizeof(anna_stack_frame_t *));
    return res;
}

anna_object_t *anna_node_invoke(anna_node_t *this, 
				anna_stack_frame_t *stack)
{
    //wprintf(L"anna_node_invoke with stack %d\n", stack);
    //wprintf(L"invoke %d\n", this->node_type);    
    ANNA_CHECK_NODE_PREPARED(this);
    
    switch(this->node_type)
    {
	case ANNA_NODE_CALL:
	    return anna_node_call_invoke((anna_node_call_t *)this, stack);

	case ANNA_NODE_CONSTRUCT:
	{
	    anna_node_call_t *call = (anna_node_call_t *)this;
	    //wprintf(L"Wee, calling construct with %d parameter, %d\n", call->child_count, call->child[0]);
	    
	    
	    return anna_construct(anna_type_unwrap(anna_node_invoke(call->function, stack)),
				  call,
				  stack);
	}
    
	case ANNA_NODE_RETURN:
	{
	    int i;
	    anna_node_return_t *node = (anna_node_return_t *)this;
	    anna_object_t *result = anna_node_invoke(node->payload, stack);
	    stack->stop=1;
	    for(i=1; i<node->steps;i++)
	    {
		stack=stack->parent;
		stack->stop=1;
	    }
	    
	    return result;
	}
	
	case ANNA_NODE_DUMMY:
	{
	   anna_node_dummy_t *node = (anna_node_dummy_t *)this;
	   return node->payload;
	}
	
	case ANNA_NODE_TRAMPOLINE:
	{
	    anna_node_dummy_t *node = (anna_node_dummy_t *)this;
	    return anna_trampoline(node->payload, stack);
	}
	
	case ANNA_NODE_INT_LITERAL:
	    return anna_node_int_literal_invoke((anna_node_int_literal_t *)this, stack);

	case ANNA_NODE_FLOAT_LITERAL:
	   return anna_node_float_literal_invoke((anna_node_float_literal_t *)this, stack);

	case ANNA_NODE_IDENTIFIER:
	    return anna_node_identifier_invoke((anna_node_identifier_t *)this, stack);	    

	case ANNA_NODE_STRING_LITERAL:
	   return anna_node_string_literal_invoke((anna_node_string_literal_t *)this, stack);

	case ANNA_NODE_CHAR_LITERAL:
	   return anna_node_char_literal_invoke((anna_node_char_literal_t *)this, stack);

	case ANNA_NODE_NULL:
	    return null_object;

	case ANNA_NODE_ASSIGN:
	   return anna_node_assign_invoke((anna_node_assign_t *)this, stack);

	case ANNA_NODE_MEMBER_GET:
	   return anna_node_member_get_invoke((anna_node_member_get_t *)this, stack);

	case ANNA_NODE_MEMBER_SET:
	   return anna_node_member_set_invoke((anna_node_member_set_t *)this, stack);

	case ANNA_NODE_MEMBER_GET_WRAP:
	   return anna_node_member_get_wrap_invoke((anna_node_member_get_t *)this, stack);

	default:
	    wprintf(L"HOLP\n");
	    exit(1);
    }    
}

void anna_node_print(anna_node_t *this)
{
    switch(this->node_type)
    {
	case ANNA_NODE_INT_LITERAL:
	{
	    anna_node_int_literal_t *this2 = (anna_node_int_literal_t *)this;
	    wprintf(L"%d", this2->payload);
	    break;
	}
	
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_node_float_literal_t *this2 = (anna_node_float_literal_t *)this;
	    wprintf(L"%f", this2->payload);
	    break;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *this2 = (anna_node_string_literal_t *)this;
	    int i;
	    
	    wprintf(L"\"");
	    for(i=0;i<this2->payload_size;i++)
	    {
		wchar_t c = this2->payload[i];
		if(c<32) 
		{
		    wprintf(L"\\x%.2x", c);		    
		}
		else
		{
		    wprintf(L"%lc", c);
		}
	    }
	    wprintf(L"\"");
	    
	    break;
	}
	
	case ANNA_NODE_CHAR_LITERAL:
	{
	    anna_node_char_literal_t *this2 = (anna_node_char_literal_t *)this;
	    wprintf(L"'%lc'", this2->payload);
	    break;
	}
	
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *this2 = (anna_node_identifier_t *)this;
	    wprintf(L"%ls", this2->name);
	    break;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *this2 = (anna_node_assign_t *)this;
	    wprintf(L"__assign__(");

	    wprintf(L"%d:%d", this2->sid.frame, this2->sid.offset);
	    wprintf(L"; ");
	    anna_node_print(this2->value);
	    wprintf(L")");
	    
	    break;
	}
	
	case ANNA_NODE_TRAMPOLINE:
	case ANNA_NODE_DUMMY:
	{
	    anna_node_dummy_t *this2 = (anna_node_dummy_t *)this;
	    wprintf(L"<Dummy:%ls>", this2->payload->type->name);
	    break;
	}
	

	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_get_t *this2 = (anna_node_member_get_t *)this;
	    wprintf(L"__memberGet__(");
	    anna_node_print(this2->object);
	    wprintf(L", %ls)", anna_mid_get_reverse(this2->mid));
	    break;
	}
	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_get_t *this2 = (anna_node_member_get_t *)this;
	    wprintf(L"__memberGet__(");
	    anna_node_print(this2->object);
	    wprintf(L", %ls)", anna_mid_get_reverse(this2->mid));
	    break;
	}
	
	case ANNA_NODE_NULL:
	{
	    wprintf(L"null");
	    break;
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *this2 = (anna_node_call_t *)this;	    
	    int i;
//	    wprintf(L"/*%d*/", this2);
	    anna_node_print(this2->function);
	    wprintf(L"(");
	    for(i=0; i<this2->child_count; i++)
	    {
		if(i!=0) 
		{
		    wprintf(L"; ");
		}
		anna_node_print(this2->child[i]);
	    }
	    wprintf(L")" );
	    break;
	}
	
	default:
	{
	    wprintf(L"<Don't know how to print node of type %d>", this->node_type);
	    break;
	}
    }
}

void anna_node_print_code(anna_node_t *node)
{
    int current_line=1;
    int current_column=0;
    int print=0;
    int mark=0;
    int is_after_first;
    int is_before_last;
    int is_marking=0;
    
    if(!node->location.filename)
      return;
    
    
    FILE *file = wfopen(node->location.filename, "r");
    if(!file)
    {
	fwprintf(stderr, L"Error: %ls: Not found\n", node->location.filename);
	return;
    }    
    while(1)
    {
	wint_t res = fgetwc(file);
	switch(res)
	{
	    case WEOF:
		if(is_marking)
		{
		    fwprintf(stderr, L"\e[0m");		
		}
		return;
	}
	
	print = (current_line >=(node->location.first_line-1)) && (current_line <= node->location.last_line+1);

	is_after_first  = (current_line >node->location.first_line) || (current_line == node->location.first_line && current_column >= node->location.first_column);
	is_before_last  = (current_line <node->location.last_line) || (current_line == node->location.last_line && current_column < node->location.last_column);
	
	if(current_column == 0 && print)
	{
	    if(is_marking)
	    {
		fwprintf(stderr, L"\e[0m");		
	    }
	    is_marking=0;
	    fwprintf(stderr, L"%*d: ", 6, current_line);
	    
	}
	
	
	mark = is_after_first && is_before_last;
	if(print && mark != is_marking)
	{
	    if(mark)
	    {
		fwprintf(stderr, L"\e[31m");
	    }
	    else 
	    {
		fwprintf(stderr, L"\e[0m");		
	    }
	    
	}
	is_marking = mark;
	if(print)
	{
	    fputwc(res,stderr);
	}

	switch(res)
	{
	    case L'\n':
		current_line++;
		current_column=0;
		break;
	    default:
		current_column++;
		break;
	}	
    }    
}

static size_t anna_node_size(anna_node_t *n)
{
    switch(n->node_type)
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	    return sizeof(anna_node_call_t);
	    
	case ANNA_NODE_IDENTIFIER:
	    return sizeof(anna_node_identifier_t);

	case ANNA_NODE_INT_LITERAL:
	    return sizeof(anna_node_int_literal_t);
	case ANNA_NODE_STRING_LITERAL:
	    return sizeof(anna_node_string_literal_t);
	case ANNA_NODE_CHAR_LITERAL:
	    return sizeof(anna_node_char_literal_t);
	case ANNA_NODE_FLOAT_LITERAL:
	    return sizeof(anna_node_float_literal_t);
	case ANNA_NODE_NULL:
	    return sizeof(anna_node_t);
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_TRAMPOLINE:
	    return sizeof(anna_node_dummy_t);
	case ANNA_NODE_ASSIGN:
	    return sizeof(anna_node_assign_t);
	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	    return sizeof(anna_node_member_get_t);
	case ANNA_NODE_MEMBER_SET:
	    return sizeof(anna_node_member_set_t);
	case ANNA_NODE_RETURN:
	    return sizeof(anna_node_return_t);
	default:
	    anna_error(n, L"Unknown node type while determining size\n");
	    exit(1);
    }
    
}

anna_node_t *anna_node_clone_shallow(anna_node_t *n)
{
    size_t sz = anna_node_size(n);
    anna_node_t *r = malloc(sz);
    memcpy(r,n,sz);
    return r;
}

anna_node_t *anna_node_clone_deep(anna_node_t *n)
{
    switch(n->node_type)
    {
	/*
	  Clone a call node. This invlolves a shallow clone of the
	  node as well as calling the deep clone of every child and
	  the function node.
	 */
	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	{
	    anna_node_t *r = anna_node_clone_shallow(n);
	    int i;
	    anna_node_call_t *r2=(anna_node_call_t *)r;
	    anna_node_call_t *n2=(anna_node_call_t *)n;
	    r2->child = malloc(sizeof(anna_node_t *)*r2->child_capacity);
	    memcpy(r2->child, n2->child, sizeof(anna_node_t *)*r2->child_count);
	    r2->function = anna_node_clone_deep(r2->function);

	    for(i=0;i<r2->child_count; i++)
	    {
		r2->child[i] = anna_node_clone_deep(r2->child[i]);
	    }
	    return r;
	}
	
	/*
	  These nodes are not mutable and they have no child nodes, so
	  we can return them as is.
	*/
	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_TRAMPOLINE:
	    return n;
	    
	    /*
	      These nodes are not yet handled, but that should be
	      perfectly ok for now, since they are only ever created
	      by the prepare system, and cloning a prepared AST is
	      never supported or possible.
	     */
	case ANNA_NODE_ASSIGN:
	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	case ANNA_NODE_MEMBER_SET:
	case ANNA_NODE_RETURN:
	default:
	    anna_error(n, L"Unsupported node type %d for deep copy!\n", n->node_type);
	    exit(1);
	
    }
}

anna_node_t *anna_node_replace(anna_node_t *tree, anna_node_identifier_t *from, anna_node_t *to)
{
    switch(tree->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *tree2 = (anna_node_identifier_t *)tree;
	    return (wcscmp(tree2->name,from->name)==0)?
		anna_node_clone_deep(to):tree;
	}

	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_TRAMPOLINE:
	{
	    return tree;
	}

	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	{
	    int i;
	    anna_node_call_t *this2 =(anna_node_call_t *)anna_node_clone_shallow(tree);	    
	    this2->function = anna_node_replace(this2->function,
						from, to);
	    for(i=0;i<this2->child_count;i++)
	    {
		this2->child[i] = anna_node_replace(this2->child[i],
						    from, to);
	    }
	    return (anna_node_t *)this2;	    
	}
	
	default:
	    wprintf(L"OOPS! Unknown node type when replacing: %d\n", tree->node_type);
	    exit(1);
    }
    
}

int anna_node_compare(anna_node_t *node1, anna_node_t *node2)
{
    if(node1->node_type != node2->node_type)
    {
        return 0;
    }
   
    switch(node1->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	{
	   anna_node_identifier_t *id1 = (anna_node_identifier_t *)node1;
	   anna_node_identifier_t *id2 = (anna_node_identifier_t *)node2;
	   return wcscmp(id1->name, id2->name) == 0;
	}
	
	case ANNA_NODE_CALL:
	{
	   anna_node_call_t *n1 = (anna_node_call_t *)node1;
	   anna_node_call_t *n2 = (anna_node_call_t *)node2;
	   if(n1->child_count != n2->child_count)
	     return 0;
	   if(!anna_node_compare(n1->function, n2->function))
	     return 0;
	   int i;
	   for(i=0; i<n1->child_count;i++)
	   {
	      if(!anna_node_compare(n1->child[i], n2->child[i]))
		 return 0;
	   }
	   return 1;
	}
	
	case ANNA_NODE_INT_LITERAL:
	{
	   anna_node_int_literal_t *n1 = (anna_node_int_literal_t *)node1;
	   anna_node_int_literal_t *n2 = (anna_node_int_literal_t *)node2;
	   return n1->payload == n2->payload;
	}
	
	case ANNA_NODE_CHAR_LITERAL:
	{
	   anna_node_char_literal_t *n1 = (anna_node_char_literal_t *)node1;
	   anna_node_char_literal_t *n2 = (anna_node_char_literal_t *)node2;
	   return n1->payload == n2->payload;
	}
	
	case ANNA_NODE_FLOAT_LITERAL:
	{
	   anna_node_float_literal_t *n1 = (anna_node_float_literal_t *)node1;
	   anna_node_float_literal_t *n2 = (anna_node_float_literal_t *)node2;
	   return n1->payload == n2->payload;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	   anna_node_string_literal_t *n1 = (anna_node_string_literal_t *)node1;
	   anna_node_string_literal_t *n2 = (anna_node_string_literal_t *)node2;
	   if(n1->payload_size != n2->payload_size)
	      return 0;
	   return wcscmp(n1->payload, n2->payload) == 0;	   
	}
	
	default:
	    wprintf(L"OOPS! Unknown node type when comparing: %d\n", node1->node_type);
	    exit(1);
	
    }

}

