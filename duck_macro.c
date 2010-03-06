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
#include "duck_macro.h"


#define CHECK_INPUT_COUNT(n, name, count) if(n->child_count != count)	\
    {									\
      duck_error((duck_node_t *)n,					\
		 L"Wrong number of arguments to %ls: Got %d, expected %d", \
		 name, n->child_count, count);			\
      return (duck_node_t *)duck_node_null_create(&node->location);	\
    }

#define CHECK_NODE_TYPE(n, type) if(n->node_type != type)		\
    {									\
      duck_error((duck_node_t *)node,					\
		 L"Unexpected argument type, expected a parameter of type %s", #type ); \
      return (duck_node_t *)duck_node_null_create(&node->location);	\
    }

#define CHECK_NODE_BLOCK(n) if(!check_node_block(n)) return (duck_node_t *)duck_node_null_create(&node->location)

#define CHECK_NODE_LOOKUP_NAME(n, name) if(!check_node_lookup_name(n, name)) return (duck_node_t *)duck_node_null_create(&node->location)

static int check_node_lookup_name(duck_node_t *node, wchar_t *name)
{
    if(node->node_type != DUCK_NODE_LOOKUP)
    {
	duck_error((duck_node_t *)node,
		   L"Unexpected argument type, expected an identifier");
	return 0;
    }
    duck_node_lookup_t *l = (duck_node_lookup_t *)node;
    if(wcscmp(l->name, name)!=0)
    {
	duck_error((duck_node_t *)node,
		   L"Unexpected identifier value, expected \"%ls\"", name);
	return 0;	
    }
    return 1;

}

static int check_node_block(duck_node_t *n)
{
    if(n->node_type != DUCK_NODE_CALL)
    {
	duck_error((duck_node_t *)n,
		   L"Unexpected argument type. Expected a block definition.");
	return 0;	    
    }
    {
	duck_node_call_t *__cnb_tmp = (duck_node_call_t *)n;
	if(__cnb_tmp->function->node_type != DUCK_NODE_LOOKUP)
	{
	    duck_error((duck_node_t *)__cnb_tmp->function,
		       L"Unexpected argument type. Expected a block definition.");
	    return 0;	    
	}
	duck_node_lookup_t *__cnb_tmp2 = (duck_node_lookup_t *)__cnb_tmp->function;
	if(wcscmp(__cnb_tmp2->name, L"__block__") != 0)
	{
	    duck_error((duck_node_t *)__cnb_tmp->function,
		       L"Unexpected argument type. Expected a block definition.");
	    return 0;	    
	}
    }
    return 1;
}

static wchar_t *duck_find_method(duck_type_t *type, wchar_t *prefix, 
				      size_t argc, duck_type_t *arg2_type)
{
    int i;
    wchar_t **members = calloc(sizeof(wchar_t *), type->member_count+type->static_member_count);
    wchar_t *match=0;
    int fault_count=0;
    
    assert(arg2_type);
    

    duck_type_get_member_names(type, members);    
    //wprintf(L"Searching for %ls[XXX]... in %ls\n", prefix, type->name);
    
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
	    if(mem_fun->argc != 2)
		continue;
	    
	    if(!mem_fun->argv[1])
	    {
		wprintf(L"Internal error. Type %ls has member named %ls with invalid second argument\n",
			type->name, members[i]);
		exit(1);
		
	    }
	    
	    
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
    int return_pop_count = 1+func->return_pop_count;
    
    return (duck_node_t *)duck_node_dummy_create(&node->location,
						 duck_function_create(L"!anonymous", 0, node, null_type, 0, 0, 0, func->stack_template, return_pop_count)->wrapper,
	1);
}

static duck_type_t *duck_sniff_return_type(duck_node_call_t *body)
{
  /*
    FIXME: Actually do some sniffing...
  */
  return int_type;
  
}



static duck_node_t *duck_macro_function_internal(duck_type_t *type, 
						 duck_node_call_t *node, 
						 duck_function_t *func, 
						 duck_node_list_t *parent)
{
    wchar_t *name=0;
    wchar_t *internal_name=0;
    CHECK_INPUT_COUNT(node,L"function definition", 5);
    
    if (node->child[0]->node_type == DUCK_NODE_LOOKUP) {
	duck_node_lookup_t *name_lookup = (duck_node_lookup_t *)node->child[0];
	internal_name = name = name_lookup->name;
    }
    else {
	CHECK_NODE_TYPE(node->child[0], DUCK_NODE_NULL);
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
    if(declarations->child_count > 0 || type)
    {
	argc = declarations->child_count;
	if(type)
	    argc++;
	
	argv = malloc(sizeof(duck_type_t *)*argc);
	argn = malloc(sizeof(wchar_t *)*argc);
	
	if(type)
	{
	    argv[0]=type;
	    argn[0]=L"this";
	}
	
	for(i=0; i<declarations->child_count; i++)
	{
	    duck_node_call_t *decl = node_cast_call(declarations->child[i]);
	    duck_node_lookup_t *name = node_cast_lookup(decl->child[0]);
	    duck_node_lookup_t *type_name = node_cast_lookup(decl->child[1]);
	    duck_object_t *type_wrapper = duck_stack_get_str(func->stack_template, type_name->name);
	    assert(type_wrapper);
	    argv[i+!!type] = duck_type_unwrap(type_wrapper);
	    argn[i+!!type] = name->name;
	}
    }

    if(type)
    {
      duck_function_t *result = duck_function_create(internal_name, 0, (duck_node_call_t *)body, null_type, argc, argv, argn, func->stack_template, 0);

	assert(name);
	assert(body->node_type == DUCK_NODE_CALL);
	duck_method_create(type, -1, name, 0, result);
	
    }
    else
    {
	duck_object_t *result;
	if(body->node_type == DUCK_NODE_CALL) {
	  result = duck_function_create(internal_name, 0, (duck_node_call_t *)body, null_type, argc, argv, argn, func->stack_template, 0)->wrapper;
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
    
}

static duck_node_t *duck_macro_function(duck_node_call_t *node,
					duck_function_t *func, 
					duck_node_list_t *parent)
{
    return duck_macro_function_internal(0, node, func, parent);
}

static duck_node_t *duck_macro_operator_wrapper(duck_node_call_t *node, duck_function_t *func, duck_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"operator", 2);
    
   duck_prepare_children(node, func, parent);
   duck_node_lookup_t *name_lookup = node_cast_lookup(node->function);
   if(wcslen(name_lookup->name) < 5)
   {
       duck_error((duck_node_t *)node, L"Invalid operator name: %ls", name_lookup->name);	
       return (duck_node_t *)duck_node_null_create(&node->location);       
   }
   

   wchar_t *name_prefix = wcsdup(name_lookup->name);
   name_prefix[wcslen(name_prefix)-2] = 0;
   //wprintf(L"Calling operator_wrapper as %ls\n", name);
   
    duck_type_t * t1 = duck_node_get_return_type(node->child[0], func->stack_template);
    duck_type_t * t2 = duck_node_get_return_type(node->child[1], func->stack_template);
    
    if(!t1) 
    {
	duck_error(node->child[1], L"Unknown type for first argument to operator %ls", name_lookup->name);
	return (duck_node_t *)duck_node_null_create(&node->location);	
    }
    if(!t2) 
    {	
	duck_error(node->child[1], L"Unknown type for second argument to operator %ls", name_lookup->name);	
	return (duck_node_t *)duck_node_null_create(&node->location);	
    }
    
    wchar_t *method_name = duck_find_method(t1, name_prefix, 2, t2);
    
    if(method_name)
    {
	    
	duck_node_t *mg_param[2]=
	    {
		node->child[0], (duck_node_t *)duck_node_lookup_create(&node->location,method_name)
	    }
	;
	
	duck_node_t *c_param[1]=
	    {
		node->child[1]
	    }
	;
	
	return (duck_node_t *)
	    duck_node_call_create(&node->location,
				  (duck_node_t *)
				  duck_node_call_create(&node->location,
							(duck_node_t *)
							duck_node_lookup_create(&node->location,
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
		node->child[1], (duck_node_t *)duck_node_lookup_create(&node->location, method_name)
	    }
	;
	
	duck_node_t *c_param[1]=
	    {
		node->child[0]
	    }
	;
	
	return (duck_node_t *)
	    duck_node_call_create(&node->location,
				  (duck_node_t *)
				  duck_node_call_create(&node->location,
							(duck_node_t *)
							duck_node_lookup_create(&node->location,
										L"__memberGet__"),
							2,
							mg_param),
				  1,
				  c_param);

	
    }
}

duck_node_t *duck_list_each(duck_node_call_t *node,
			    duck_function_t *func, 
			    duck_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"each", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_TYPE(node->function, DUCK_NODE_MEMBER_GET_WRAP);
    
    switch(node->child[0]->node_type)
    {
	case DUCK_NODE_LOOKUP:
	{
	    duck_node_lookup_t * value_name;

	    value_name = (duck_node_lookup_t *)node->child[0];

    duck_node_member_get_t * mg = (duck_node_member_get_t *)node->function;
    
    int return_pop_count = 1+func->return_pop_count;
    
    duck_type_t *argv[]=
	{
	    object_type
	}
    ;
    
    wchar_t *argn[]=
	{
	    value_name->name
	}
    ;

//    wprintf(L"Setting up each function, param name is %ls\n", name->name);
    
    
    wchar_t *method_name = L"__eachValue__";
    duck_type_t *lst_type = duck_node_get_return_type(mg->object, func->stack_template);
    
    size_t mid = duck_mid_get(method_name);
    duck_type_t *member_type = duck_type_member_type_get(lst_type, method_name);
    if(!member_type)
    {
	duck_error((duck_node_t *)node, L"Unable to calculate type of member %ls of object of type %ls", method_name, lst_type->name);
	return (duck_node_t *)duck_node_null_create(&node->location);	\
    }
    
    duck_node_t *function = (duck_node_t *)
	duck_node_dummy_create(&node->location,
			       duck_function_create(L"!anonymous", 0, node->child[1], 
						    null_type, 1, argv, &value_name->name, 
						    func->stack_template, 
						    return_pop_count)->wrapper,
			       1);
    
    
    return (duck_node_t *)duck_node_call_create(&node->location,
						duck_node_member_get_create(&node->location,
									    mg->object,
									    mid,
									    member_type,
									    1),   
						1,
						&function);
    
    

	}
	
	case DUCK_NODE_CALL:
	{
	    duck_node_lookup_t * key_name;
	    duck_node_lookup_t * value_name;

	    duck_node_call_t * decl = (duck_node_call_t *)node->child[0];
	    CHECK_NODE_LOOKUP_NAME(decl->function, L"Pair");
	    CHECK_INPUT_COUNT(decl,L"each", 2);
	    CHECK_NODE_TYPE(decl->child[0], DUCK_NODE_LOOKUP);
	    CHECK_NODE_TYPE(decl->child[1], DUCK_NODE_LOOKUP);
	    key_name = (duck_node_lookup_t *)decl->child[0];
	    value_name = (duck_node_lookup_t *)decl->child[1];
	    duck_node_member_get_t * mg = (duck_node_member_get_t *)node->function;
    
	    int return_pop_count = 1+func->return_pop_count;
    
	    duck_type_t *argv[]=
		{
		    int_type, object_type
		}
	    ;
	    
	    wchar_t *argn[]=
		{
		    key_name->name, value_name->name
		}
	    ;

	    //wprintf(L"Setting up each function, param names are %ls and %ls\n", key_name->name, value_name->name);
        
	    wchar_t *method_name = L"__eachPair__";
	    duck_type_t *lst_type = duck_node_get_return_type(mg->object, func->stack_template);
	    
	    size_t mid = duck_mid_get(method_name);
	    duck_type_t *member_type = duck_type_member_type_get(lst_type, method_name);
	    if(!member_type)
	    {
		duck_error((duck_node_t *)node, L"Unable to calculate type of member %ls of object of type %ls", method_name, lst_type->name);
		return (duck_node_t *)duck_node_null_create(&node->location); \
	    }
	    
	    duck_node_t *function = (duck_node_t *)
		duck_node_dummy_create(&node->location,
				       duck_function_create(L"!anonymous", 0, node->child[1], 
							    null_type, 2, argv, argn,
							    func->stack_template, 
							    return_pop_count)->wrapper,
			       1);
	    
	    
	    return (duck_node_t *)duck_node_call_create(&node->location,
							duck_node_member_get_create(&node->location,
										    mg->object,
										    mid,
										    member_type,
										    1),   
							1,
							&function);
	    
	}	
	default:
	    duck_error((duck_node_t *)node->child[0], L"Expected a value parameter name or a key:value parameter name pair");
	    return (duck_node_t *)duck_node_null_create(&node->location);
    }
    

    
}



static duck_node_t *duck_macro_get(duck_node_call_t *node, 
				   duck_function_t *func,
				   duck_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"__get__ operator", 2);
  duck_prepare_children(node, func, parent);
    
  duck_type_t * t1 = duck_node_get_return_type(node->child[0], func->stack_template);
  duck_type_t * t2 = duck_node_get_return_type(node->child[1], func->stack_template);
    
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
      node->child[0], (duck_node_t *)duck_node_lookup_create(&node->location, method_name)
    }
  ;
  
  duck_node_t *c_param[1]=
    {
      node->child[1]
    }
  ;
  
  duck_node_t *result = (duck_node_t *)
	duck_node_call_create(&node->location,
			      (duck_node_t *)
			      duck_node_call_create(&node->location,
						    (duck_node_t *)
						    duck_node_lookup_create(&node->location,
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



static duck_node_t *duck_macro_set(duck_node_call_t *node, 
				   duck_function_t *func,
				   duck_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"__set__ operator", 3);
  duck_prepare_children(node, func, parent);
    
  duck_type_t * t1 = duck_node_get_return_type(node->child[0], func->stack_template);
  duck_type_t * t2 = duck_node_get_return_type(node->child[1], func->stack_template);

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
      node->child[0], (duck_node_t *)duck_node_lookup_create(&node->location, method_name)
    }
  ;
  
  duck_node_t *c_param[2]=
    {
	node->child[1], node->child[2]
    }
  ;
  
  duck_node_t *result = (duck_node_t *)
	duck_node_call_create(&node->location,
			      (duck_node_t *)
			      duck_node_call_create(&node->location,
						    (duck_node_t *)
						    duck_node_lookup_create(&node->location,
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
    CHECK_INPUT_COUNT(node,L"variable declaration", 3);
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

static duck_node_t *duck_macro_member(duck_type_t *type,
				      struct duck_node_call *node, 
				      struct duck_function *function,
				      struct duck_node_list *parent)
{
    CHECK_INPUT_COUNT(node,L"variable declaration", 3);
    CHECK_NODE_TYPE(node->child[0], DUCK_NODE_LOOKUP);
    duck_prepare_children(node, function, parent);
    duck_node_lookup_t *name_lookup = node_cast_lookup(node->child[0]);
    duck_type_t *var_type;
    switch(node->child[1]->node_type) 
    {
	case DUCK_NODE_LOOKUP:
	{
	    duck_node_lookup_t *type_lookup;
	    type_lookup = node_cast_lookup(node->child[1]);
	    duck_object_t *type_wrapper = duck_stack_get_str(function->stack_template, type_lookup->name);
	    assert(type_wrapper);
	    var_type = duck_type_unwrap(type_wrapper);
	    break;
	}
	
	case DUCK_NODE_NULL:	
	    var_type = duck_node_get_return_type(node->child[2], function->stack_template);
	    //wprintf(L"Implicit var dec type: %ls\n", type->name);
	    break;

	default:
	    wprintf(L"Dang, wrong type thing\n");
	    exit(1);
    }
    
    assert(var_type);

    duck_member_create(type, -1, name_lookup->name, 0, var_type);
    
    duck_stack_declare(function->stack_template, name_lookup->name, type, null_object);
    
    /*
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
    */
}

static duck_node_t *duck_macro_assign(struct duck_node_call *node, 
				      struct duck_function *function,
				      struct duck_node_list *parent)
{
    CHECK_INPUT_COUNT(node,L"= operator", 2);
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
	   if(wcscmp(name_lookup->name, L"__get__")==0)
	   {
	       name_lookup->name=L"__set__";
	       duck_node_call_add_child(call, node->child[1]);
	       return (duck_node_t *)call;
	   }
	   else if(wcscmp(name_lookup->name, L"__memberGet__")==0)
	   {
	       name_lookup->name=L"__memberSet__";
	       duck_node_call_add_child(call, node->child[1]);
	       return (duck_node_t *)call;
	   }
       }
       
       default:
	   wprintf(L"Cricital: BLUPP\n");
	   CRASH;
   }
       
}

static duck_node_t *duck_macro_member_get(duck_node_call_t *node, 
					  duck_function_t *func, 
					  duck_node_list_t *parent)
{
/*
  wprintf(L"member_get on node at %d\n", node);
  duck_node_print((duck_node_t *)node);
  wprintf(L"\n");
*/
    CHECK_INPUT_COUNT(node,L". operator", 2);
    CHECK_NODE_TYPE(node->child[1], DUCK_NODE_LOOKUP);
    
    duck_prepare_children(node, func, parent);
    duck_type_t *object_type = duck_node_get_return_type(node->child[0], func->stack_template);
    if(!object_type) 
    {
	duck_error(node->child[0], L"Tried to access member in object of unknown type");
	return (duck_node_t *)duck_node_null_create(&node->location);	
    }
    
    duck_node_lookup_t *name_node = node_cast_lookup(node->child[1]);
    size_t mid = duck_mid_get(name_node->name);
    
    duck_type_t *member_type = duck_type_member_type_get(object_type, name_node->name);
    if(!member_type)
    {
	duck_error(node, L"Unable to calculate type of member %ls of object of type %ls", name_node->name, object_type->name);
	return (duck_node_t *)duck_node_null_create(&node->location);	\
    }
    
    int wrap = !!duck_static_member_addr_get_mid(member_type, DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    return (duck_node_t *)duck_node_member_get_create(&node->location,
						      node->child[0], 
						      mid,
						      member_type,
						      wrap);
}

static duck_node_t *duck_macro_member_set(duck_node_call_t *node, 
					  duck_function_t *func, 
					  duck_node_list_t *parent)
{
/*
  wprintf(L"member_get on node at %d\n", node);
  duck_node_print((duck_node_t *)node);
  wprintf(L"\n");
*/
    CHECK_INPUT_COUNT(node,L"member assignment", 3);
    CHECK_NODE_TYPE(node->child[1], DUCK_NODE_LOOKUP);
    
    duck_prepare_children(node, func, parent);
    duck_type_t *object_type = duck_node_get_return_type(node->child[0], func->stack_template);
    if(!object_type) 
    {
	duck_error(node->child[0], L"Tried to assign member in object of unknown type");
	return (duck_node_t *)duck_node_null_create(&node->location);	
    }
    
    duck_node_lookup_t *name_node = node_cast_lookup(node->child[1]);
    size_t mid = duck_mid_get(name_node->name);
    
    duck_type_t *member_type = duck_type_member_type_get(object_type, name_node->name);
    
    return (duck_node_t *)duck_node_member_set_create(&node->location,
						      node->child[0], 
						      mid,
						      node->child[2],
						      member_type);
}

static duck_node_t *duck_macro_if(duck_node_call_t *node,
				  duck_function_t *func, 
				  duck_node_list_t *parent)
{
/*
   wprintf(L"member_get on node at %d\n", node);
   duck_node_print((duck_node_t *)node);
   wprintf(L"\n");
*/
    CHECK_INPUT_COUNT(node,L"if macro", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    
    duck_node_t *argv[] = {
        node->child[0], node->child[1], duck_node_null_create(&node->location)
    };
    
    return (duck_node_t *)
        duck_node_call_create(&node->location, 
			      (duck_node_t *)
			      duck_node_lookup_create(&node->location, 
						      L"__if__"),
			      3,
			      argv);
}

static duck_node_t *duck_macro_else(duck_node_call_t *node,
				    duck_function_t *func, 
				    duck_node_list_t *parent)
{
/*
   wprintf(L"member_get on node at %d\n", node);
   duck_node_print((duck_node_t *)node);
   wprintf(L"\n");
*/
    CHECK_INPUT_COUNT(node,L"else macro", 1);
    CHECK_NODE_BLOCK(node->child[0]);
    
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
   prev_call->child[2] = duck_node_prepare(node->child[0], func, parent);

   return (duck_node_t *)
       duck_node_null_create(&node->location);
   
}

static duck_object_t *duck_function_or(duck_object_t **param)
{
  return param[0] == null_object?duck_function_wrapped_invoke(param[1], 0, 0, 0):param[0];
}

static duck_node_t *duck_macro_or(duck_node_call_t *node, duck_function_t *func, duck_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"or operator", 2);
    
    duck_prepare_children(node, func, parent);

    duck_type_t * t1 = duck_node_get_return_type(node->child[0], func->stack_template);
    duck_type_t * t2 = duck_node_get_return_type(node->child[1], func->stack_template);
    
    if(!t1) 
    {
	duck_error(node->child[1], L"Unknown type for first argument to operator or");
	return (duck_node_t *)duck_node_null_create(&node->location);	
    }
    if(!t2) 
    {	
	duck_error(node->child[1], L"Unknown type for second argument to operator or");	
	return (duck_node_t *)duck_node_null_create(&node->location);	
    }
    
    duck_type_t *return_type = duck_type_intersect(t1,t2);
    wchar_t *argn[]=
	{
	    L"condition1",
	    L"condition2"
	}
    ;
    
    duck_node_t *param[]=
	{
	    node->child[0],
	    (duck_node_t *)
	    duck_node_dummy_create(&node->location,
				   duck_function_create(L"!orConditionBlock", 0, 
							duck_node_call_create(&node->location,
									      (duck_node_t *)
									      duck_node_lookup_create(&node->location,
												      L"__block__"),
									      1,
									      &node->child[1]), 
							t2, 0, 0, 0, 
							func->stack_template, func->return_pop_count+1)->wrapper,
				   1)
	}
    ;
    duck_type_t *argv[]=
	{
	    t1,
	    duck_node_get_return_type(param[1], func->stack_template)
	}
    ;
    
    return (duck_node_t *)
	duck_node_call_create(&node->location, 
			      (duck_node_t *)
			      duck_node_dummy_create( &node->location,
						      duck_native_create(L"!orAnonymous",
									 DUCK_FUNCTION_FUNCTION,
									 (duck_native_t)duck_function_or,
									 return_type,
									 2,
									 argv,
									 argn)->wrapper,
						      0),
			      2,
			      param);
}

static duck_object_t *duck_function_and(duck_object_t **param)
{
  return (param[0] == null_object)?null_object:duck_function_wrapped_invoke(param[1], 0, 0, 0);
}

static duck_node_t *duck_macro_and(duck_node_call_t *node, duck_function_t *func, duck_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"and operator", 2);
    
    duck_prepare_children(node, func, parent);
    
    duck_type_t * t1 = duck_node_get_return_type(node->child[0], func->stack_template);
    duck_type_t * t2 = duck_node_get_return_type(node->child[1], func->stack_template);
    
    if(!t1) 
    {
	duck_error(node->child[1], L"Unknown type for first argument to operator and");
	return (duck_node_t *)duck_node_null_create(&node->location);	
    }
    if(!t2) 
    {	
	duck_error(node->child[1], L"Unknown type for second argument to operator and");	
	return (duck_node_t *)duck_node_null_create(&node->location);	
    }
    
    wchar_t *argn[]=
	{
	    L"condition1",
	    L"condition2"
	}
    ;
    duck_node_t *param[]=
	{
	    node->child[0],
	    (duck_node_t *)
	    duck_node_dummy_create(&node->location,
				   duck_function_create(L"!andConditionBlock", 0, 
							duck_node_call_create(&node->location,
									      (duck_node_t *)
									      duck_node_lookup_create(&node->location,
												      L"__block__"),
									      1,
									      &node->child[1]), 
							t2, 0, 0, 0, 
							func->stack_template, func->return_pop_count+1)->wrapper,
				   1)
	}
    ;

    duck_type_t *argv[]=
	{
	    t1,
	    duck_node_get_return_type(param[1], func->stack_template)
	}
    ;
    
    return (duck_node_t *)
	duck_node_call_create(&node->location, 
			      (duck_node_t *)
			      duck_node_dummy_create( &node->location,
						      duck_native_create(L"!andAnonymous",
									 DUCK_FUNCTION_FUNCTION,
									 (duck_native_t)duck_function_and,
									 t2,
									 2,
									 argv,
									 argn)->wrapper,
						      0),
			      2,
			      param);
}

static duck_object_t *duck_function_while(duck_object_t **param)
{
    duck_object_t *result = null_object;
    while(duck_function_wrapped_invoke(param[0], 0, 0, 0) != null_object)
    {
      result = duck_function_wrapped_invoke(param[1], 0, 0, 0);
    }
    return result;
}

static duck_node_t *duck_macro_while(duck_node_call_t *node, duck_function_t *func, duck_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"while macro", 2);
    CHECK_NODE_BLOCK(node->child[1]);

    duck_prepare_children(node, func, parent);

    duck_type_t *t2 = duck_node_get_return_type(node->child[1], func->stack_template);
    
    if(!t2) 
    {	
	duck_error(node->child[1], L"Unknown type for second argument to while");	
	return (duck_node_t *)duck_node_null_create(&node->location);	
    }
    
    duck_node_t *condition = 
	(duck_node_t *)
	duck_node_dummy_create(&node->location,
			       duck_function_create(L"!andConditionBlock", 0, 
						    duck_node_call_create(&node->location,
									  (duck_node_t *)
									  duck_node_lookup_create(&node->location,
												  L"__block__"),
									  1,
									  &node->child[0]), 
						    t2, 0, 0, 0, 
						    func->stack_template,
						    func->return_pop_count+1)->wrapper,
			       1);
    
    wchar_t *argn[]=
	{
	    L"condition",
	    L"body"
	}
    ;

    duck_node_t *param[]=
	{
	    condition,
	    node->child[1]
	}
    ;

    duck_type_t *argv[]=
	{
	    duck_node_get_return_type(param[0], func->stack_template),
	    t2
	}
    ;
    /*
      FIXME: I think the return values are all wrong here, need to
      make sure we're rturning the function result, not the functio
      type itself...
     */
    return (duck_node_t *)
	duck_node_call_create(&node->location,
			      (duck_node_t *)
			      duck_node_dummy_create( &node->location,
							 duck_native_create(L"!whileAnonymous",
									    DUCK_FUNCTION_FUNCTION,
									    (duck_native_t)duck_function_while,
									    t2,
									    2,
									    argv,
									    argn)->wrapper,
						      0),
			      2,
			      param);
}

static duck_node_t *duck_macro_type(duck_node_call_t *node, 
				    duck_function_t *func, 
				    duck_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"type macro", 4);
    CHECK_NODE_TYPE(node->child[0], DUCK_NODE_LOOKUP);
    CHECK_NODE_TYPE(node->child[1], DUCK_NODE_LOOKUP);
    CHECK_NODE_BLOCK(node->child[3]);

    wchar_t *name = ((duck_node_lookup_t *)node->child[0])->name;
    wchar_t *type_name = ((duck_node_lookup_t *)node->child[1])->name;
    int error_count=0;
    
    duck_type_t *type = duck_type_create(name, 64);
    duck_stack_declare(func->stack_template, name, type_type, type->wrapper);
    duck_node_call_t *body = (duck_node_call_t *)node->child[3];

    int i;
    for(i=0; i<body->child_count; i++)
    {
	duck_node_t *item = body->child[i];
	
	if(item->node_type != DUCK_NODE_CALL) 
	{
	    duck_error(item,
		       L"Only function declarations and variable declarations allowed directly inside a class body" );
	    error_count++;
	    continue;
	}
	
	duck_node_call_t *call = (duck_node_call_t *)item;
	if(call->function->node_type != DUCK_NODE_LOOKUP)
	{
	    duck_error(call->function,
		       L"Only function declarations and variable declarations allowed directly inside a class body" ); 
	    error_count++;
	    continue;
	}
	
	duck_node_lookup_t *declaration = (duck_node_lookup_t *)call->function;
	
	if(wcscmp(declaration->name, L"__function__")==0)
	{
	    duck_macro_function_internal(type, call, func, parent);
	}
	else if(wcscmp(declaration->name, L"__declare__")==0)
	{
	    duck_macro_member(type, call, func, parent);
	}
	else
	{
	    duck_error(call->function,
		       L"Only function declarations and variable declarations allowed directly inside a class body" );
	    error_count++;	    
	}
    }
    if(error_count)
	return (duck_node_t *)duck_node_null_create(&node->location);	\
    
    duck_object_t **constructor_ptr = duck_static_member_addr_get_mid(type, DUCK_MID_INIT_PAYLOAD);
    duck_function_t *constructor = duck_function_unwrap(*constructor_ptr);
    
    duck_type_t **argv= malloc(sizeof(duck_type_t *)*(constructor->input_count));
    wchar_t **argn= malloc(sizeof(wchar_t *)*(constructor->input_count));
    argv[0]=type_type;
    argn[0]=L"this";

    for(i=1; i<constructor->input_count; i++)
    {
	argv[i] = constructor->input_type[i];
	argn[i] = constructor->input_name[i];
    }
/*    
    duck_native_method_create(type, DUCK_MID_CALL_PAYLOAD, L"__call__",
			      0, (duck_native_t)&duck_construct,
			      type, constructor->input_count, argv, argn);
    wprintf(L"Create __call__ for non-native type %ls\n", type->name);
*/  
    return (duck_node_t *)duck_node_dummy_create(&node->location,
						 type->wrapper,
						 0);
}

static duck_node_t *duck_macro_return(duck_node_call_t *node, 
				      duck_function_t *func, 
				      duck_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"return", 1);
    return (duck_node_t *)duck_node_return_create(&node->location, node->child[0], func->return_pop_count+1);
}

static void duck_macro_add(duck_stack_frame_t *stack, 
			   wchar_t *name,
			   duck_native_macro_t call)
{
    duck_native_declare(stack, name, DUCK_FUNCTION_MACRO, (duck_native_t)call, 0, 0, 0, 0);
}


void duck_macro_init(duck_stack_frame_t *stack)
{
    int i;
    
    duck_macro_add(stack, L"__block__", &duck_macro_block);
    duck_macro_add(stack, L"__memberGet__", &duck_macro_member_get);
    duck_macro_add(stack, L"__memberSet__", &duck_macro_member_set);
    duck_macro_add(stack, L"__assign__", &duck_macro_assign);
    duck_macro_add(stack, L"__declare__", &duck_macro_declare);
    duck_macro_add(stack, L"__function__", &duck_macro_function);
    duck_macro_add(stack, L"if", &duck_macro_if);
    duck_macro_add(stack, L"else", &duck_macro_else);
    duck_macro_add(stack, L"__get__", &duck_macro_get);
    duck_macro_add(stack, L"__set__", &duck_macro_set);
    duck_macro_add(stack, L"__or__", &duck_macro_or);
    duck_macro_add(stack, L"__and__", &duck_macro_and);
    duck_macro_add(stack, L"while", &duck_macro_while);
    duck_macro_add(stack, L"__type__", &duck_macro_type);
    duck_macro_add(stack, L"return", &duck_macro_return);
    
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
	    L"__xor__",
	    L"__format__",
	    L"__cshl__",
	    L"__cshr__"
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
