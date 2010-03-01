#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "wutil.h"
#include "duck_node.h"
#include "duck_int.h"
#include "duck_float.h"
#include "duck_string.h"
#include "duck_char.h"

#define check(node, test, ...) if(!(test)) duck_error(node, __VA_ARGS__)

void duck_node_set_location(duck_node_t *node, duck_location_t *l)
{
    assert(l->filename);
    
    memcpy(&node->location, l, sizeof(duck_location_t));
}

duck_node_call_t *node_cast_call(duck_node_t *node) 
{
    assert(node->node_type==DUCK_NODE_CALL);
    return (duck_node_call_t *)node;
}

duck_node_lookup_t *node_cast_lookup(duck_node_t *node) 
{
    assert(node->node_type==DUCK_NODE_LOOKUP);
    return (duck_node_lookup_t *)node;
}

duck_node_int_literal_t *node_cast_int_literal(duck_node_t *node) 
{
    assert(node->node_type==DUCK_NODE_INT_LITERAL);
    return (duck_node_int_literal_t *)node;
}

duck_node_string_literal_t *node_cast_string_literal(duck_node_t *node) 
{
    assert(node->node_type==DUCK_NODE_STRING_LITERAL);
    return (duck_node_string_literal_t *)node;
}

duck_node_dummy_t *duck_node_dummy_create(duck_location_t *loc, struct duck_object *val, int is_trampoline)
{
   duck_node_dummy_t *result = calloc(1,sizeof(duck_node_dummy_t));
   result->node_type = is_trampoline?DUCK_NODE_TRAMPOLINE:DUCK_NODE_DUMMY;
   duck_node_set_location((duck_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;  
}

duck_node_member_get_t *duck_node_member_get_create(duck_location_t *loc, struct duck_node *object, size_t mid, struct duck_type *type, int wrap)
{
   duck_node_member_get_t *result = calloc(1,sizeof(duck_node_member_get_t));
   result->node_type = wrap?DUCK_NODE_MEMBER_GET_WRAP:DUCK_NODE_MEMBER_GET;
   duck_node_set_location((duck_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->object=object;
   result->mid=mid;
   result->type=type;
   return result;  
  
}


duck_node_assign_t *duck_node_assign_create(duck_location_t *loc, duck_sid_t sid, struct duck_node *value)
{
   duck_node_assign_t *result = calloc(1,sizeof(duck_node_assign_t));
   result->node_type = DUCK_NODE_ASSIGN;
   duck_node_set_location((duck_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->value=value;
   result->sid=sid;
   return result;  
}

duck_node_int_literal_t *duck_node_int_literal_create(duck_location_t *loc, int val)
{
   duck_node_int_literal_t *result = calloc(1,sizeof(duck_node_int_literal_t));
   result->node_type = DUCK_NODE_INT_LITERAL;
   duck_node_set_location((duck_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;
}

duck_node_float_literal_t *duck_node_float_literal_create(duck_location_t *loc, double val)
{
   duck_node_float_literal_t *result = calloc(1,sizeof(duck_node_float_literal_t));
   result->node_type = DUCK_NODE_FLOAT_LITERAL;
   duck_node_set_location((duck_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;
}

duck_node_char_literal_t *duck_node_char_literal_create(duck_location_t *loc, wchar_t val)
{
   duck_node_char_literal_t *result = calloc(1,sizeof(duck_node_char_literal_t));
   result->node_type = DUCK_NODE_CHAR_LITERAL;
   duck_node_set_location((duck_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;
}

duck_node_string_literal_t *duck_node_string_literal_create(duck_location_t *loc, size_t sz, wchar_t *str)
{
   duck_node_string_literal_t *result = calloc(1,sizeof(duck_node_string_literal_t));
   result->node_type = DUCK_NODE_STRING_LITERAL;
   duck_node_set_location((duck_node_t *)result,loc);
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = str;
   result->payload_size = sz;
   return result;
}

duck_node_call_t *duck_node_call_create(duck_location_t *loc, duck_node_t *function, size_t argc, duck_node_t **argv)
{
    duck_node_call_t *result = calloc(1,sizeof(duck_node_call_t));
    result->child = calloc(1,sizeof(duck_node_t *)*(argc));
    result->node_type = DUCK_NODE_CALL;
    duck_node_set_location((duck_node_t *)result,loc);
    result->function = function;
    result->child_count = argc;
    result->child_capacity = argc;
    memcpy(result->child, argv, sizeof(duck_node_t *)*(argc));
    /*
      FIXME: Create a nice and tidy wrapper
    */
    return result;
}

duck_node_lookup_t *duck_node_lookup_create(duck_location_t *loc, wchar_t *name)
{
    duck_node_lookup_t *result = calloc(1,sizeof(duck_node_call_t));
    result->node_type = DUCK_NODE_LOOKUP;
   duck_node_set_location((duck_node_t *)result,loc);
    result->name = name;
    /*
      FIXME: Create a nice and tidy wrapper
    */
    return result;
}

duck_node_t *duck_node_null_create(duck_location_t *loc)
{
    duck_node_t *result = calloc(1,sizeof(duck_node_t));
    result->node_type = DUCK_NODE_NULL;
   duck_node_set_location((duck_node_t *)result,loc);
    /*
      FIXME: Create a nice and tidy wrapper
    */
    return result;
}

void duck_node_call_add_child(duck_node_call_t *call, duck_node_t *child)
{
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 8 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(duck_node_t *));
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

void duck_node_call_prepend_child(duck_node_call_t *call, duck_node_t *child)
{
    if(call->child_count==0) 
    {
	duck_node_call_add_child(call, child);
	return;
    }
    
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 8 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(duck_node_t *));
	if(!call->child) 
	{
	    wprintf(L"Out of memory\n");
	    exit(1);
	}	
	call->child_capacity = new_capacity;
    }
//    wprintf(L"LALALA %d %d\n", call->child_capacity, call->child_count);
    memmove(&call->child[1], call->child[0], sizeof(duck_node_t *)*call->child_count);
    call->child[0] = child;
    call->child_count++;
}

void duck_node_call_set_function(duck_node_call_t *call, duck_node_t *function)
{
    call->function = function;
}

duck_node_t *duck_node_call_prepare(duck_node_call_t *node, duck_function_t *function, duck_node_list_t *parent)
{
   duck_node_list_t list = 
      {
	 (duck_node_t *)node, 0, parent
      }
   ;
/*
   wprintf(L"Prepare call node:\n");
   duck_node_print((duck_node_t *)node);
   wprintf(L"\n");
*/
   if(node->function->node_type == DUCK_NODE_LOOKUP)
   {
      duck_node_lookup_t *name=(duck_node_lookup_t *)node->function;      
      duck_object_t *obj = duck_stack_get_str(function->stack_template, name->name);
      duck_function_t *func=duck_function_unwrap(obj);
      
      if(func->flags == DUCK_FUNCTION_MACRO)
      {
	  return duck_node_prepare(func->native.macro(node, function, parent), function, parent);
      }
   }
   else 
   {
      node->function = duck_node_prepare(node->function, function, parent);	 
   }
   
   //wprintf(L"Regular function, prepare the kids\n");
   int i;
   for(i=0; i<node->child_count; i++)
   {
      list.idx = i;
      node->child[i] = duck_node_prepare(node->child[i], function, &list);	 
   }
   return (duck_node_t *)node;
}

duck_object_t *duck_node_call_invoke(duck_node_call_t *this, duck_stack_frame_t *stack)
{
    //wprintf(L"duck_node_call_invoke with stack %d\n", stack);
    duck_object_t *obj = duck_node_invoke(this->function, stack);
    if(obj == null_object){
	return obj;
    }
    
    return duck_function_wrapped_invoke(obj, this, stack);
}

duck_object_t *duck_node_int_literal_invoke(duck_node_int_literal_t *this, duck_stack_frame_t *stack)
{
  return duck_int_create(this->payload);
}

duck_object_t *duck_node_float_literal_invoke(duck_node_float_literal_t *this, duck_stack_frame_t *stack)
{
  return duck_float_create(this->payload);
}

duck_object_t *duck_node_string_literal_invoke(duck_node_string_literal_t *this, duck_stack_frame_t *stack)
{
  return duck_string_create(this->payload_size, this->payload);
}

duck_object_t *duck_node_char_literal_invoke(duck_node_char_literal_t *this, duck_stack_frame_t *stack)
{
  return duck_char_create(this->payload);
}

duck_object_t *duck_node_lookup_invoke(duck_node_lookup_t *this, duck_stack_frame_t *stack)
{
/*    wprintf(L"Lookup on string \"%ls\", sid is %d,%d\n", this->name, this->sid.frame, this->sid.offset);
    assert(duck_stack_get_sid(stack, this->sid) == duck_stack_get_str(stack, this->name));
    
    return duck_stack_get_sid(stack, this->sid);*/

  return duck_stack_get_str(stack, this->name);
}

duck_object_t *duck_node_assign_invoke(duck_node_assign_t *this, duck_stack_frame_t *stack)
{
   duck_object_t *result = duck_node_invoke(this->value, stack);
   duck_stack_set_sid(stack, this->sid, result);
   return result;
}

duck_type_t *duck_node_get_return_type(duck_node_t *this, duck_stack_frame_t *stack)
{
    switch(this->node_type)
    {
	case DUCK_NODE_CALL:
	{
	    duck_node_call_t *this2 =(duck_node_call_t *)this;	    

	    duck_type_t *func_type = duck_node_get_return_type(this2->function, stack);
	    /*
	      Special case constructors...
	     */
	    if(func_type == type_type)
	    {
		if(this2->function->node_type == DUCK_NODE_LOOKUP)
		{
		    duck_node_lookup_t *lookup = (duck_node_lookup_t *)this2->function;
		    duck_object_t *type_wrapper = duck_stack_get_str(stack, lookup->name);
		    assert(type_wrapper);
		    return duck_type_unwrap(type_wrapper);
		}
		wprintf(L"Illigal init\n");
		CRASH;
		
	    }
	    
	    duck_function_type_key_t *function_data = duck_function_unwrap_type(func_type);
	    return function_data->result;
	}
	
	case DUCK_NODE_TRAMPOLINE:
	case DUCK_NODE_DUMMY:
	{
	   duck_node_dummy_t *this2 =(duck_node_dummy_t *)this;	    
	   return this2->payload->type;   
	}
	
	case DUCK_NODE_INT_LITERAL:
	    return int_type;

	case DUCK_NODE_FLOAT_LITERAL:
	    return float_type;

	case DUCK_NODE_LOOKUP:
	{
	    duck_node_lookup_t *this2 =(duck_node_lookup_t *)this;	    
	    return duck_stack_get_type(stack, this2->name);
	}
	case DUCK_NODE_STRING_LITERAL:
	    return string_type;

	case DUCK_NODE_CHAR_LITERAL:
	    return char_type;

	case DUCK_NODE_NULL:
	    return null_type;

	case DUCK_NODE_MEMBER_GET:
	case DUCK_NODE_MEMBER_GET_WRAP:
	{
	    duck_node_member_get_t *this2 =(duck_node_member_get_t *)this;
	    return this2->type;
	}
	
	    
	default:
	    wprintf(L"SCRAP! Unknown node type when checking return type: %d\n", this->node_type);
	    exit(1);
    }
}

void duck_node_validate(duck_node_t *this, duck_stack_frame_t *stack)
{
    switch(this->node_type)
    {
	case DUCK_NODE_CALL:
	{
	    duck_node_call_t *this2 =(duck_node_call_t *)this;	    
	    int i;
	    
	    duck_node_validate(this2->function, stack);
	    for(i=0; i<this2->child_count; i++)
	    {
		duck_node_validate(this2->child[i], stack);
	    }
	    
	    duck_type_t *func_type = duck_node_get_return_type(this2->function, stack);
	    /*
	      Special case constructors...

	      FIXME: Doesn't actually test anything yet... :-/
	     */
	    if(func_type == type_type)
	    {
		if(this2->function->node_type == DUCK_NODE_LOOKUP)
		{
		    duck_node_lookup_t *lookup = (duck_node_lookup_t *)this2->function;
		    duck_object_t *type_wrapper = duck_stack_get_str(stack, lookup->name);
		    assert(type_wrapper);
		    return;
		}
		return;
	    }
	    
	    duck_function_type_key_t *function_data = duck_function_unwrap_type(func_type);
	    if(!function_data)
	    {
		duck_error(this, 
			   L"Unknown function");
		return;
	    }
	    
	    int is_method = (this2->function->node_type == DUCK_NODE_MEMBER_GET_WRAP);	    	    
	    check(this, function_data->argc-is_method == this2->child_count,
		  L"Wrong number of paramaters in function call. Should be %d, not %d.", 
		  function_data->argc-is_method, this2->child_count);
	    
	    for(i=is_method; i<mini(this2->child_count, function_data->argc); i++)
	    {
		if(!function_data->argv[i])
		{
		    duck_error(this, 
			  L"Unknown function");
		    break;
		}
		
		duck_type_t *ctype = duck_node_get_return_type(this2->child[i-is_method], stack);
		
		if(ctype)
		{
		    //duck_node_print(this);
		    check(this, duck_abides(ctype, function_data->argv[i]),
			  L"Wrong type of argument %d of %d, %ls does not abide by %ls", i+1, function_data->argc, ctype->name,
			  function_data->argv[i]->name);
		}
		else
		{
		    duck_error(this, L"Unknown type for for argument %d", i);
		}
	    }
	    
	    return;
	}
	
	case DUCK_NODE_TRAMPOLINE:
	case DUCK_NODE_DUMMY:
	{
	   duck_node_dummy_t *this2 =(duck_node_dummy_t *)this;	    
	   return;
	}
	
	case DUCK_NODE_INT_LITERAL:
	case DUCK_NODE_FLOAT_LITERAL:
	case DUCK_NODE_CHAR_LITERAL:
	case DUCK_NODE_NULL:
	    return;

	    

	case DUCK_NODE_LOOKUP:
	{
	    duck_node_lookup_t *this2 =(duck_node_lookup_t *)this;	    
	    check(this, !!this2->name, L"Invalid lookup node");
	    check(this, !!duck_stack_get_type(stack, this2->name), L"Unknown variable: %ls", this2->name);
	    return;
	}
	case DUCK_NODE_STRING_LITERAL:
	{
	    duck_node_string_literal_t *this2 =(duck_node_string_literal_t *)this;	    
	    check(this, !!this2->payload, L"Invalid string node");
	    return;
	}

	case DUCK_NODE_MEMBER_GET:
	case DUCK_NODE_MEMBER_GET_WRAP:
	{
	    duck_node_member_get_t *this2 =(duck_node_member_get_t *)this;
	    return;
	}
		    
	default:
	    check(this, 1, L"Unknown node type");
    }
}

duck_node_t *duck_node_prepare(duck_node_t *this, duck_function_t *function, duck_node_list_t *parent)
{
   
    switch(this->node_type)
    {
	case DUCK_NODE_CALL:
	    return duck_node_call_prepare((duck_node_call_t *)this, function, parent);
	    
	case DUCK_NODE_LOOKUP:
	{
	    /*
	      duck_node_lookup_t *this2 =(duck_node_lookup_t *)this;
	      this2->sid = duck_stack_sid_create(function->stack_template, this2->name);
	    */
	    return this;
	}	

	case DUCK_NODE_TRAMPOLINE:
	case DUCK_NODE_DUMMY:
	case DUCK_NODE_INT_LITERAL:
	case DUCK_NODE_FLOAT_LITERAL:
	case DUCK_NODE_STRING_LITERAL:
	case DUCK_NODE_CHAR_LITERAL:
	case DUCK_NODE_NULL:
	case DUCK_NODE_ASSIGN:
	case DUCK_NODE_MEMBER_GET:
	case DUCK_NODE_MEMBER_GET_WRAP:
	    return this;   

	default:
	    wprintf(L"HULP %d\n", this->node_type);
	    exit(1);
    }
}

duck_object_t *duck_node_member_get_invoke(duck_node_member_get_t *this, 
					   duck_stack_frame_t *stack)
{
    return *duck_member_addr_get_mid(duck_node_invoke(this->object, stack), this->mid);
}

duck_object_t *duck_node_member_get_wrap_invoke(duck_node_member_get_t *this, 
						duck_stack_frame_t *stack)
{
    duck_object_t *obj = duck_node_invoke(this->object, stack);
    return duck_method_wrap(*duck_member_addr_get_mid(obj, this->mid), obj);
}

static duck_object_t *duck_trampoline(duck_object_t *orig, 
				      duck_stack_frame_t *stack)
{
    duck_object_t *res = duck_object_create(orig->type);
    memcpy(duck_member_addr_get_mid(res,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD),
	   duck_member_addr_get_mid(orig,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD),
	   sizeof(duck_function_t *));    
    memcpy(duck_member_addr_get_mid(res,DUCK_MID_FUNCTION_WRAPPER_STACK),
	   &stack,
	   sizeof(duck_stack_frame_t *));
    return res;
}

duck_object_t *duck_node_invoke(duck_node_t *this, 
				duck_stack_frame_t *stack)
{
    //wprintf(L"duck_node_invoke with stack %d\n", stack);
//    wprintf(L"invoke %d\n", this->node_type);    
    switch(this->node_type)
    {
	case DUCK_NODE_CALL:
	    return duck_node_call_invoke((duck_node_call_t *)this, stack);

	case DUCK_NODE_DUMMY:
	{
	   duck_node_dummy_t *node = (duck_node_dummy_t *)this;
	   return node->payload;
	}
	
	case DUCK_NODE_TRAMPOLINE:
	{
	    duck_node_dummy_t *node = (duck_node_dummy_t *)this;
	    return duck_trampoline(node->payload, stack);
	}
	
	case DUCK_NODE_INT_LITERAL:
	    return duck_node_int_literal_invoke((duck_node_int_literal_t *)this, stack);

	case DUCK_NODE_FLOAT_LITERAL:
	   return duck_node_float_literal_invoke((duck_node_float_literal_t *)this, stack);

	case DUCK_NODE_LOOKUP:
	    return duck_node_lookup_invoke((duck_node_lookup_t *)this, stack);	    

	case DUCK_NODE_STRING_LITERAL:
	   return duck_node_string_literal_invoke((duck_node_string_literal_t *)this, stack);

	case DUCK_NODE_CHAR_LITERAL:
	   return duck_node_char_literal_invoke((duck_node_char_literal_t *)this, stack);

	case DUCK_NODE_NULL:
	    return null_object;

	case DUCK_NODE_ASSIGN:
	   return duck_node_assign_invoke((duck_node_assign_t *)this, stack);

	case DUCK_NODE_MEMBER_GET:
	   return duck_node_member_get_invoke((duck_node_member_get_t *)this, stack);

	case DUCK_NODE_MEMBER_GET_WRAP:
	   return duck_node_member_get_wrap_invoke((duck_node_member_get_t *)this, stack);

	default:
	    wprintf(L"HOLP\n");
	    exit(1);
    }    
}

void duck_node_print(duck_node_t *this)
{
    switch(this->node_type)
    {
	case DUCK_NODE_INT_LITERAL:
	{
	    duck_node_int_literal_t *this2 = (duck_node_int_literal_t *)this;
	    wprintf(L"%d", this2->payload);
	    break;
	}
	
	case DUCK_NODE_FLOAT_LITERAL:
	{
	    duck_node_float_literal_t *this2 = (duck_node_float_literal_t *)this;
	    wprintf(L"%f", this2->payload);
	    break;
	}
	
	case DUCK_NODE_STRING_LITERAL:
	{
	    duck_node_string_literal_t *this2 = (duck_node_string_literal_t *)this;
	    wprintf(L"\"%.*ls\"", this2->payload_size, this2->payload);
	    break;
	}
	
	case DUCK_NODE_CHAR_LITERAL:
	{
	    duck_node_char_literal_t *this2 = (duck_node_char_literal_t *)this;
	    wprintf(L"'%lc'", this2->payload);
	    break;
	}
	
	case DUCK_NODE_LOOKUP:
	{
	    duck_node_lookup_t *this2 = (duck_node_lookup_t *)this;
	    wprintf(L"%ls", this2->name);
	    break;
	}
	
	case DUCK_NODE_MEMBER_GET:
	{
	    duck_node_member_get_t *this2 = (duck_node_member_get_t *)this;
	    wprintf(L"__memberGet__(");
	    duck_node_print(this2->object);
	    wprintf(L", %d)", this2->mid);
	    break;
	}
	case DUCK_NODE_MEMBER_GET_WRAP:
	{
	    duck_node_member_get_t *this2 = (duck_node_member_get_t *)this;
	    wprintf(L"__wrapMethod__(__memberGet__(");
	    duck_node_print(this2->object);
	    wprintf(L", %d))", this2->mid);
	    break;
	}
	
	case DUCK_NODE_NULL:
	{
	    wprintf(L"null");
	    break;
	}
	
	case DUCK_NODE_CALL:
	{
	    duck_node_call_t *this2 = (duck_node_call_t *)this;	    
	    int i;
	    duck_node_print(this2->function);
	    wprintf(L"(");
	    for(i=0; i<this2->child_count; i++)
	    {
		if(i!=0) 
		{
		    wprintf(L"; ");
		}
		duck_node_print(this2->child[i]);
	    }
	    wprintf(L")" );
	    break;
	}

	
	
	default:
	{
	    wprintf(L"Don't know hos to print node of type %d\n", this->node_type);
	    break;
	}
    }
}

void duck_node_print_code(duck_node_t *node)
{
    int current_line=1;
    int current_column=0;
    int print=0;
    int mark=0;
    int is_after_first;
    int is_before_last;
    int is_marking=0;
    
    
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
	
	print = (current_line >=node->location.first_line) && (current_line <= node->location.last_line);

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


