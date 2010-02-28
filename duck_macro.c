#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "duck.h"
#include "duck_node.h"
#include "duck_stack.h"


static wchar_t *duck_find_method(duck_type_t *type, wchar_t *prefix, 
				      size_t argc, duck_type_t *arg2_type)
{
    int i;
    wchar_t **members = calloc(sizeof(wchar_t *), type->member_count+type->static_member_count);
    wchar_t *match=0;
    int fault_count=0;
    
    duck_type_get_member_names(type, members);    
    //wprintf(L"Searching for %ls[XXX]...\n", prefix);
    
    for(i=0; i<type->member_count+type->static_member_count; i++)
    {
	//wprintf(L"Check %ls\n", members[i]);
	if(wcsncmp(prefix, members[i], wcslen(prefix)) != 0)
	    continue;
	//wprintf(L"%ls matches, name-wise\n", members[i]);
	
	duck_type_t *mem_type = duck_type_member_type_get(type, members[i]);
	//wprintf(L"Is of type %ls\n", mem_type->name);
	duck_function_type_key_t *mem_fun = duck_function_unwrap_type(mem_type);
	if(mem_fun)
	{
	    if(mem_fun->argc == argc && duck_abides(arg2_type, mem_fun->argv[1]))
	    {
		int my_fault_count = duck_abides_fault_count(mem_fun->argv[1], arg2_type);
		if(!match || my_fault_count < fault_count)
		{
		    match = members[i];
		    fault_count = my_fault_count;
		}
	    }
	}
	else
	{
	    //  wprintf(L"Not a function\n");
	}
	
    }
    return match;
        
}


static size_t duck_parent_count(struct duck_node_list *parent)
{
   return parent?1+duck_parent_count(parent->parent):0;
}


static duck_node_t *duck_macro_block(duck_node_call_t *node, duck_function_t *func, duck_node_list_t *parent)
{
    //wprintf(L"Create new block with %d elements at %d\n", node->child_count, node);
    return (duck_node_t *)duck_node_dummy_create(&node->location,
						 duck_function_create(L"!anonymous", 0, node, null_type, 0, 0, 0, func->stack_template)->wrapper,
	1);
}

static duck_type_t *duck_sniff_return_type(duck_node_call_t *body)
{
  /*
    FIXME: Actually do some sniffing...
  */
  return int_type;
  
}

static duck_node_t *duck_macro_function(duck_node_call_t *node, duck_function_t *func, duck_node_list_t *parent)
{
    wchar_t *name=0;
    wchar_t *internal_name=0;
    assert(node->child_count==5);
    
    if (node->child[0]->node_type == DUCK_NODE_LOOKUP) {
	duck_node_lookup_t *name_lookup = (duck_node_lookup_t *)node->child[0];
	internal_name = name = name_lookup->name;
    }
    else {
	assert(node->child[0]->node_type == DUCK_NODE_NULL);
	internal_name = L"!anonymous";
    }
    
    duck_node_t *body = node->child[3];
    
    assert(body->node_type == DUCK_NODE_NULL ||body->node_type == DUCK_NODE_CALL);
    
    duck_type_t *out_type=0;
    duck_node_t *out_type_wrapper = node->child[1];
    if(out_type_wrapper->node_type == DUCK_NODE_NULL) 
    {
	assert(body->node_type == DUCK_NODE_CALL);
	out_type = duck_sniff_return_type((duck_node_call_t *)body);
    }
    else
    {
	duck_node_lookup_t *type_lookup;
	type_lookup = node_cast_lookup(out_type_wrapper);
	duck_object_t *type_wrapper = duck_stack_get_str(func->stack_template, type_lookup->name);
	assert(type_wrapper);
	out_type = duck_type_unwrap(type_wrapper);
    }
    
    size_t argc=0;
    duck_type_t **argv=0;
    wchar_t **argn=0;
    
    duck_node_call_t *declarations = node_cast_call(node->child[2]);
    int i;
    if(declarations->child_count > 0)
    {
	argc = declarations->child_count;
	argv = malloc(sizeof(duck_type_t *)*declarations->child_count);
	argn = malloc(sizeof(wchar_t *)*declarations->child_count);
	
	for(i=0; i<declarations->child_count; i++)
	{
	    duck_node_call_t *decl = node_cast_call(declarations->child[i]);
	    duck_node_lookup_t *name = node_cast_lookup(decl->child[0]);
	    duck_node_lookup_t *type_name = node_cast_lookup(decl->child[1]);
	    duck_object_t *type_wrapper = duck_stack_get_str(func->stack_template, type_name->name);
	    assert(type_wrapper);
	    argv[i] = duck_type_unwrap(type_wrapper);
	    argn[i] = name->name;
	}
    }

    duck_object_t *result;
    if(body->node_type == DUCK_NODE_CALL) {
	result = duck_function_create(internal_name, 0, (duck_node_call_t *)body, null_type, argc, argv, argn, func->stack_template)->wrapper;
    }
    else {
	result = null_object;
    }
    
    if(name) {
	duck_stack_declare(func->stack_template, name, duck_type_for_function(out_type, argc, argv), result);
    }
    return (duck_node_t *)duck_node_dummy_create(&node->location,
						 result,
						 1);
}

static duck_node_t *duck_macro_operator_wrapper(duck_node_call_t *in, duck_function_t *func, duck_node_list_t *parent)
{
   if(in->child_count != 2)
   {
       duck_error((duck_node_t *)in, L"Wrong number of arguments to operator: Got %d, expected 2", in->child_count);	
       return (duck_node_t *)duck_node_null_create(&in->location);	
   }

   duck_prepare_children(in, func, parent);
   duck_node_lookup_t *name_lookup = node_cast_lookup(in->function);
   if(wcslen(name_lookup->name) < 5)
   {
       duck_error((duck_node_t *)in, L"Invalid operator name: %ls", name_lookup->name);	
       return (duck_node_t *)duck_node_null_create(&in->location);       
   }
   

   wchar_t *name_prefix = wcsdup(name_lookup->name);
   name_prefix[wcslen(name_prefix)-2] = 0;
   //wprintf(L"Calling operator_wrapper as %ls\n", name);
   
    duck_type_t * t1 = duck_node_get_return_type(in->child[0], func->stack_template);
    duck_type_t * t2 = duck_node_get_return_type(in->child[1], func->stack_template);
    
    if(!t1) 
    {
	duck_error(in->child[1], L"Unknown type for first argument to operator %ls", name_lookup->name);
	return (duck_node_t *)duck_node_null_create(&in->location);	
    }
    if(!t2) 
    {	
	duck_error(in->child[1], L"Unknown type for second argument to operator %ls", name_lookup->name);	
	return (duck_node_t *)duck_node_null_create(&in->location);	
    }
    
    wchar_t *method_name = duck_find_method(t1, name_prefix, 2, t2);
    
    if(method_name)
    {
	    
	duck_node_t *mg_param[2]=
	    {
		in->child[0], (duck_node_t *)duck_node_lookup_create(&in->location,method_name)
	    }
	;
	
	duck_node_t *c_param[1]=
	    {
		in->child[1]
	    }
	;
	
	return (duck_node_t *)
	    duck_node_call_create(&in->location,
				  (duck_node_t *)
				  duck_node_call_create(&in->location,
							(duck_node_t *)
							duck_node_lookup_create(&in->location,
										L"__memberGet__"),
							2,
							mg_param),
				  1,
				  c_param);
    }
    else
    {
	string_buffer_t buff;
	sb_init(&buff);
	sb_append(&buff, L"__r");
	sb_append(&buff, &name_prefix[2]);
	wchar_t *reverse_name_prefix = sb_content(&buff);
	method_name = duck_find_method(t2, reverse_name_prefix, 2, t1);
	sb_destroy(&buff);
	
	if(!method_name)
	{
	    wprintf(L"Error: %ls__: No support for call with objects of types %ls and %ls\n",
		    name_prefix, t1->name, t2->name);
	    exit(1);
	}

	duck_node_t *mg_param[2]=
	    {
		in->child[1], (duck_node_t *)duck_node_lookup_create(&in->location, method_name)
	    }
	;
	
	duck_node_t *c_param[1]=
	    {
		in->child[0]
	    }
	;
	
	return (duck_node_t *)
	    duck_node_call_create(&in->location,
				  (duck_node_t *)
				  duck_node_call_create(&in->location,
							(duck_node_t *)
							duck_node_lookup_create(&in->location,
										L"__memberGet__"),
							2,
							mg_param),
				  1,
				  c_param);

	
    }
}



static duck_node_t *duck_macro_get(duck_node_call_t *in, duck_function_t *func, duck_node_list_t *parent)
{
  assert(in->child_count == 2);
  duck_prepare_children(in, func, parent);
    
  duck_type_t * t1 = duck_node_get_return_type(in->child[0], func->stack_template);
  duck_type_t * t2 = duck_node_get_return_type(in->child[1], func->stack_template);
    
  wchar_t *method_name = duck_find_method(t1, L"__get", 2, t2);

  if(!method_name)
  {
      wprintf(L"Error: __get__: No support for call with objects of types %ls and %ls\n",
	      t1->name, t2->name);
      duck_stack_print(func->stack_template);
      exit(1);
  }
  
  duck_node_t *mg_param[2]=
    {
      in->child[0], (duck_node_t *)duck_node_lookup_create(&in->location, method_name)
    }
  ;
  
  duck_node_t *c_param[1]=
    {
      in->child[1]
    }
  ;
  
  duck_node_t *result = (duck_node_t *)
	duck_node_call_create(&in->location,
			      (duck_node_t *)
			      duck_node_call_create(&in->location,
						    (duck_node_t *)
						    duck_node_lookup_create(&in->location,
									    L"__memberGet__"),
						    2,
						    mg_param),
			      1,
			      c_param);
  /*
  duck_node_print(result);
  */
  return result;
  
}



static duck_node_t *duck_macro_set(duck_node_call_t *in, duck_function_t *func, duck_node_list_t *parent)
{
  assert(in->child_count == 3);
  duck_prepare_children(in, func, parent);
    
  duck_type_t * t1 = duck_node_get_return_type(in->child[0], func->stack_template);
  duck_type_t * t2 = duck_node_get_return_type(in->child[1], func->stack_template);

  wchar_t *method_name = duck_find_method(t1, L"__set", 3, t2);

  if(!method_name)
    {
      wprintf(L"Error: __set__: No support for call with objects of types %ls and %ls\n",
	      t1->name, t2->name);
      duck_stack_print(func->stack_template);
      exit(1);
    }
  
  duck_node_t *mg_param[2]=
    {
      in->child[0], (duck_node_t *)duck_node_lookup_create(&in->location, method_name)
    }
  ;
  
  duck_node_t *c_param[2]=
    {
	in->child[1], in->child[2]
    }
  ;
  
  duck_node_t *result = (duck_node_t *)
	duck_node_call_create(&in->location,
			      (duck_node_t *)
			      duck_node_call_create(&in->location,
						    (duck_node_t *)
						    duck_node_lookup_create(&in->location,
									    L"__memberGet__"),
						    2,
						    mg_param),
			      2,
			      c_param);
/*  wprintf(L"GGG\n");
  duck_node_print(result);
  wprintf(L"GGG\n");
*/
  return result;
  
}



static duck_node_t *duck_macro_declare(struct duck_node_call *node, 
			    struct duck_function *function,
			    struct duck_node_list *parent)
{
   assert(node->child_count == 3);
   assert(duck_parent_count(parent)==1);
   duck_prepare_children(node, function, parent);
   duck_node_lookup_t *name_lookup = node_cast_lookup(node->child[0]);
   duck_type_t *type;
    switch(node->child[1]->node_type) 
    {
	case DUCK_NODE_LOOKUP:
	{
	    duck_node_lookup_t *type_lookup;
	    type_lookup = node_cast_lookup(node->child[1]);
	    duck_object_t *type_wrapper = duck_stack_get_str(function->stack_template, type_lookup->name);
	    assert(type_wrapper);
	    type = duck_type_unwrap(type_wrapper);
	    break;
	}
	
       case DUCK_NODE_NULL:	
	   type = duck_node_get_return_type(node->child[2], function->stack_template);
	   //wprintf(L"Implicit var dec type: %ls\n", type->name);
	   break;

       default:
	  wprintf(L"Dang, wrong type thing\n");
	  exit(1);
    }
    assert(type);
    duck_stack_declare(function->stack_template, name_lookup->name, type, null_object);

    duck_node_t *a_param[2]=
	{
	   node->child[0],
	   node->child[2]
	}
    ;
    
    return (duck_node_t *)
       duck_node_call_create(&node->location,
			     (duck_node_t *)duck_node_lookup_create(&node->location,
								    L"__assign__"),
			     2,
			     a_param);
}

static duck_node_t *duck_macro_assign(struct duck_node_call *node, 
			   struct duck_function *function,
			   struct duck_node_list *parent)
{
   assert(node->child_count == 2);
   assert(duck_parent_count(parent)==1);

   switch(node->child[0]->node_type)
   {
       case DUCK_NODE_LOOKUP:
       {
	   duck_prepare_children(node, function, parent);
	   duck_node_lookup_t *name_lookup = node_cast_lookup(node->child[0]);
	   duck_sid_t sid = duck_stack_sid_create(function->stack_template, name_lookup->name);
	   
	   return (duck_node_t *)
	       duck_node_assign_create(&node->location,
				       sid,
				       node->child[1]);
       }
       
       case DUCK_NODE_CALL:
       {
	   duck_node_call_t *call = node_cast_call(node->child[0]);
	   //duck_node_print(call);
	   
	   duck_node_lookup_t *name_lookup = node_cast_lookup(call->function);
	   assert(wcscmp(name_lookup->name, L"__get__")==0);
	   name_lookup->name=L"__set__";
	   duck_node_call_add_child(call, node->child[1]);
	   return (duck_node_t *)call;
       }
       
       default:
	   wprintf(L"Cricital: BLUPP\n");
	   CRASH;
   }
       
}

static duck_object_t *duck_macro_while(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    assert(node->child_count == 2);
    duck_object_t *result = null_object;
    duck_object_t *body_object = duck_node_invoke(node->child[1], 0);
    while(1)
      {
	duck_object_t *test = duck_node_invoke(node->child[0], stack);
	if(test == null_object) {
	    break;
	}
	result = duck_function_wrapped_invoke(body_object, 0, stack);
      }
    return result;
}

static duck_node_t *duck_macro_member_get(duck_node_call_t *in, duck_function_t *func, duck_node_list_t *parent)
{
/*
  wprintf(L"member_get on node at %d\n", in);
  duck_node_print((duck_node_t *)in);
  wprintf(L"\n");
*/
    if(in->child_count != 2)
    {
	duck_error((duck_node_t *)in, L"Wrong number of arguments to __memberGet__: Got %d, expected 2", in->child_count);	
	return (duck_node_t *)duck_node_null_create(&in->location);	
    }
    
    duck_prepare_children(in, func, parent);
    duck_type_t *object_type = duck_node_get_return_type(in->child[0], func->stack_template);
    if(!object_type) 
    {
	duck_error(in->child[0], L"Tried to access member of object of unknown type");
	return (duck_node_t *)duck_node_null_create(&in->location);	
    }
    
    duck_node_lookup_t *name_node = node_cast_lookup(in->child[1]);
    size_t mid = duck_mid_get(name_node->name);
    
    duck_type_t *member_type = duck_type_member_type_get(object_type, name_node->name);
    
    int wrap = !!duck_static_member_addr_get_mid(member_type, DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    return (duck_node_t *)duck_node_member_get_create(&in->location,
						      in->child[0], 
						      mid,
						      member_type,
						      wrap);
}

static duck_node_t *duck_macro_if(duck_node_call_t *in, duck_function_t *func, duck_node_list_t *parent)
{
/*
   wprintf(L"member_get on node at %d\n", in);
   duck_node_print((duck_node_t *)in);
   wprintf(L"\n");
*/
   assert(in->child_count == 2);
   
   duck_node_t *argv[] = {
       in->child[0], in->child[1], duck_node_null_create(&in->location)
   };
   
   return (duck_node_t *)
     duck_node_call_create(&in->location, 
			   (duck_node_t *)
			   duck_node_lookup_create(&in->location, 
						   L"__if__"),
			   3,
			   argv);
      
}

static duck_node_t *duck_macro_else(duck_node_call_t *in, duck_function_t *func, duck_node_list_t *parent)
{
/*
   wprintf(L"member_get on node at %d\n", in);
   duck_node_print((duck_node_t *)in);
   wprintf(L"\n");
*/
   assert(in->child_count == 1);
   assert(parent->idx>0);

   duck_node_call_t *parent_call = node_cast_call(parent->node);
   duck_node_t *prev = parent_call->child[parent->idx-1];
   /*
   duck_node_print(prev);   
   wprintf(L"\n");
   */
   duck_node_call_t *prev_call = node_cast_call(prev);
   duck_node_lookup_t *prev_call_name = node_cast_lookup(prev_call->function);
   assert(wcscmp(prev_call_name->name, L"__if__")==0);
   assert(prev_call->child_count == 3);
   assert(prev_call->child[2]->node_type == DUCK_NODE_NULL);
   prev_call->child[2] = duck_node_prepare(in->child[0], func, parent);

   return (duck_node_t *)
       duck_node_null_create(&in->location);
   
}

static void duck_macro_add(duck_stack_frame_t *stack, wchar_t *name, duck_native_macro_t call)
{
  duck_native_declare(stack, name, DUCK_FUNCTION_MACRO, (duck_native_t)call, 0, 0, 0, 0);
}


void duck_macro_init(duck_stack_frame_t *stack)
{
    int i;
    
    duck_macro_add(stack, L"__block__", &duck_macro_block);
    duck_macro_add(stack, L"__memberGet__", &duck_macro_member_get);
    duck_macro_add(stack, L"__assign__", &duck_macro_assign);
    duck_macro_add(stack, L"__declare__", &duck_macro_declare);
    duck_macro_add(stack, L"__function__", &duck_macro_function);
    duck_macro_add(stack, L"if", &duck_macro_if);
    duck_macro_add(stack, L"else", &duck_macro_else);
    duck_macro_add(stack, L"__get__", &duck_macro_get);
    duck_macro_add(stack, L"__set__", &duck_macro_set);
    
    wchar_t *op_names[] = 
       {
	    L"__append__",
	    L"__join__",
	    L"__format__",
	    L"__add__",
	    L"__sub__",
	    L"__mul__",
	    L"__div__",
	    L"__join__",
	    L"__gt__",
	    L"__lt__",
	    L"__eq__",
	    L"__gte__",
	    L"__lte__",
	    L"__neq__",
	    L"__shl__",
	    L"__shr__",
	    L"__mod__",
	    L"__bitand__",
	    L"__bitor__",
	    L"__xor__"
	}
    ;

    for(i =0; i<sizeof(op_names)/sizeof(wchar_t *); i++)
    {
	duck_macro_add(stack, op_names[i], &duck_macro_operator_wrapper);
    }

    /*
      duck_macro_add(stack, L"while", &duck_macro_while);
    */

}
