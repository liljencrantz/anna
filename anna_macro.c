#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_stack.h"
#include "anna_macro.h"

#define FAIL(n, ...) anna_error((anna_node_t *)n, __VA_ARGS__); return (anna_node_t *)anna_node_null_create(&n->location);
#define CHECK_INPUT_COUNT(n, name, count) if(n->child_count != count)	\
    {									\
      anna_error((anna_node_t *)n,					\
		 L"Wrong number of arguments to %ls: Got %d, expected %d", \
		 name, n->child_count, count);			\
      return (anna_node_t *)anna_node_null_create(&node->location);	\
    }

#define CHECK_NODE_TYPE(n, type) if(n->node_type != type)		\
    {									\
      anna_error((anna_node_t *)node,					\
		 L"Unexpected argument type, expected a parameter of type %s", #type ); \
      return (anna_node_t *)anna_node_null_create(&node->location);	\
    }

#define CHECK_NODE_BLOCK(n) if(!check_node_block(n)) return (anna_node_t *)anna_node_null_create(&node->location)

#define CHECK_NODE_LOOKUP_NAME(n, name) if(!check_node_lookup_name(n, name)) return (anna_node_t *)anna_node_null_create(&node->location)

#define CHECK_TYPE(n) if(!extract_type(n, func->stack_template)) return (anna_node_t *)anna_node_null_create(&n->location)
#define EXTRACT_TYPE(n) extract_type(n, func->stack_template)

static anna_type_t *extract_type(anna_node_t *node, anna_stack_frame_t *stack)
{
    if(node->node_type != ANNA_NODE_LOOKUP)
    {
	anna_error(node, L"Expected type identifier");
	return 0;
    }
    anna_node_lookup_t *id = (anna_node_lookup_t *)node;
    anna_object_t *wrapper = anna_stack_get_str(stack, id->name);
    if(!wrapper)
    {
	anna_error(node, L"Unknown type: %ls", id->name);
	return 0;	
    }
    anna_type_t *type = anna_type_unwrap(wrapper);
    if(!type)
    {
	anna_error(node, L"Identifier is not a type: %ls", id->name);
    }
    return type;
    
}


static int check_node_lookup_name(anna_node_t *node, wchar_t *name)
{
    if(node->node_type != ANNA_NODE_LOOKUP)
    {
	anna_error((anna_node_t *)node,
		   L"Unexpected argument type, expected an identifier");
	return 0;
    }
    anna_node_lookup_t *l = (anna_node_lookup_t *)node;
    if(wcscmp(l->name, name)!=0)
    {
	anna_error((anna_node_t *)node,
		   L"Unexpected identifier value, expected \"%ls\"", name);
	return 0;	
    }
    return 1;

}

static int check_node_block(anna_node_t *n)
{
    if(n->node_type != ANNA_NODE_CALL)
    {
	anna_error((anna_node_t *)n,
		   L"Unexpected argument type. Expected a block definition.");
	return 0;	    
    }
    {
	anna_node_call_t *__cnb_tmp = (anna_node_call_t *)n;
	if(__cnb_tmp->function->node_type != ANNA_NODE_LOOKUP)
	{
	    anna_error((anna_node_t *)__cnb_tmp->function,
		       L"Unexpected argument type. Expected a block definition.");
	    return 0;	    
	}
	anna_node_lookup_t *__cnb_tmp2 = (anna_node_lookup_t *)__cnb_tmp->function;
	if(wcscmp(__cnb_tmp2->name, L"__block__") != 0)
	{
	    anna_error((anna_node_t *)__cnb_tmp->function,
		       L"Unexpected argument type. Expected a block definition.");
	    return 0;	    
	}
    }
    return 1;
}

static wchar_t *anna_find_method(anna_node_t *context, anna_type_t *type, wchar_t *prefix, 
				      size_t argc, anna_type_t *arg2_type)
{
    int i;
    wchar_t **members = calloc(sizeof(wchar_t *), type->member_count+type->static_member_count);
    wchar_t *match=0;
    int fault_count=0;
    
    assert(arg2_type);
    

    anna_type_get_member_names(type, members);    
    //wprintf(L"Searching for %ls[XXX]... in %ls\n", prefix, type->name);
    
    for(i=0; i<type->member_count+type->static_member_count; i++)
    {
	//wprintf(L"Check %ls\n", members[i]);
	if(wcsncmp(prefix, members[i], wcslen(prefix)) != 0)
	    continue;
	//wprintf(L"%ls matches, name-wise\n", members[i]);
	
	anna_type_t *mem_type = anna_type_member_type_get(type, members[i]);
	//wprintf(L"Is of type %ls\n", mem_type->name);
	anna_function_type_key_t *mem_fun = anna_function_unwrap_type(mem_type);
	if(mem_fun)
	{
	    if(mem_fun->argc != 2)
		continue;
	    
	    if(!mem_fun->argv[1])
	    {
		anna_error(context,L"Internal error. Type %ls has member named %ls with invalid second argument\n",
			   type->name, members[i]);
		return 0;
	    }
	    
	    if(mem_fun->argc == argc && anna_abides(arg2_type, mem_fun->argv[1]))
	    {
		int my_fault_count = anna_abides_fault_count(mem_fun->argv[1], arg2_type);
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


static size_t anna_parent_count(struct anna_node_list *parent)
{
   return parent?1+anna_parent_count(parent->parent):0;
}


static anna_node_t *anna_macro_block(anna_node_call_t *node, anna_function_t *func, anna_node_list_t *parent)
{
    //wprintf(L"Create new block with %d elements at %d\n", node->child_count, node);
    int return_pop_count = 1+func->return_pop_count;
    
    return (anna_node_t *)anna_node_dummy_create(&node->location,
						 anna_function_create(L"!anonymous", 0, node, null_type, 0, 0, 0, func->stack_template, return_pop_count)->wrapper,
	1);
}

static anna_type_t *anna_sniff_return_type(anna_node_call_t *body)
{
  /*
    FIXME: Actually do some sniffing...
  */
  return int_type;
  
}



static anna_node_t *anna_macro_function_internal(anna_type_t *type, 
						 anna_node_call_t *node, 
						 anna_function_t *func, 
						 anna_node_list_t *parent)
{
    wchar_t *name=0;
    wchar_t *internal_name=0;
    CHECK_INPUT_COUNT(node,L"function definition", 5);
    
    if (node->child[0]->node_type == ANNA_NODE_LOOKUP) {
	anna_node_lookup_t *name_lookup = (anna_node_lookup_t *)node->child[0];
	internal_name = name = name_lookup->name;
    }
    else {
	CHECK_NODE_TYPE(node->child[0], ANNA_NODE_NULL);
	internal_name = L"!anonymous";
    }
    
    anna_node_t *body = node->child[3];
    
    if(body->node_type != ANNA_NODE_NULL && body->node_type != ANNA_NODE_CALL)
    {
	anna_error(body, L"Invalid function body");
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    
    
    anna_type_t *out_type=0;
    anna_node_t *out_type_wrapper = node->child[1];
    if(out_type_wrapper->node_type == ANNA_NODE_NULL) 
    {
	
	if(body->node_type != ANNA_NODE_CALL)
	{
	    anna_error(body, L"Function declarations must have a return type");
	    return (anna_node_t *)anna_node_null_create(&node->location);	    
	}
	
	out_type = anna_sniff_return_type((anna_node_call_t *)body);
    }
    else
    {
	anna_node_lookup_t *type_lookup;
	type_lookup = node_cast_lookup(out_type_wrapper);
	anna_object_t *type_wrapper = anna_stack_get_str(func->stack_template, type_lookup->name);

	if(!type_wrapper)
	{
	    anna_error(type_lookup, L"Unknown type: %ls", type_lookup->name);
	    return (anna_node_t *)anna_node_null_create(&node->location);	    
	}
	out_type = anna_type_unwrap(type_wrapper);
    }
    
    size_t argc=0;
    anna_type_t **argv=0;
    wchar_t **argn=0;
    
    anna_node_call_t *declarations = node_cast_call(node->child[2]);
    int i;
    if(declarations->child_count > 0 || type)
    {
	argc = declarations->child_count;
	if(type)
	    argc++;
	
	argv = malloc(sizeof(anna_type_t *)*argc);
	argn = malloc(sizeof(wchar_t *)*argc);
	
	if(type)
	{
	    argv[0]=type;
	    argn[0]=L"this";
	}
	
	for(i=0; i<declarations->child_count; i++)
	{
	    anna_node_call_t *decl = node_cast_call(declarations->child[i]);
	    anna_node_lookup_t *name = node_cast_lookup(decl->child[0]);
	    anna_node_lookup_t *type_name = node_cast_lookup(decl->child[1]);
	    anna_object_t *type_wrapper = anna_stack_get_str(func->stack_template, type_name->name);
	    if(!type_wrapper)
	    {
		anna_error(type_name, L"Unknown type: %ls", type_name->name);
		return (anna_node_t *)anna_node_null_create(&node->location);	    
	    }
	    argv[i+!!type] = anna_type_unwrap(type_wrapper);
	    argn[i+!!type] = name->name;
	}
    }

    if(type)
    {
	anna_function_t *result = anna_function_create(internal_name, 0, (anna_node_call_t *)body, null_type, argc, argv, argn, func->stack_template, 0);
	
	if(!name)
	{
	    anna_error(node, L"Method definitions must have a name");
	    return (anna_node_t *)anna_node_null_create(&node->location);	    
	}

	CHECK_NODE_BODY(body);
	
	anna_method_create(type, -1, name, 0, result);	
    }
    else
    {
	anna_object_t *result;
	if(body->node_type == ANNA_NODE_CALL) {
	  result = anna_function_create(internal_name, 0, (anna_node_call_t *)body, null_type, argc, argv, argn, func->stack_template, 0)->wrapper;
	}
	else {
	    result = null_object;
	}
	
	if(name) {
	    anna_stack_declare(func->stack_template, name, anna_type_for_function(out_type, argc, argv), result);
	}
	return (anna_node_t *)anna_node_dummy_create(&node->location,
						     result,
						     1);
    }
    
}

static anna_node_t *anna_macro_function(anna_node_call_t *node,
					anna_function_t *func, 
					anna_node_list_t *parent)
{
    return anna_macro_function_internal(0, node, func, parent);
}

static anna_node_t *anna_macro_operator_wrapper(anna_node_call_t *node, anna_function_t *func, anna_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"operator", 2);
    
   anna_prepare_children(node, func, parent);
   anna_node_lookup_t *name_lookup = node_cast_lookup(node->function);
   if(wcslen(name_lookup->name) < 5)
   {
       anna_error((anna_node_t *)node, L"Invalid operator name: %ls", name_lookup->name);	
       return (anna_node_t *)anna_node_null_create(&node->location);       
   }
   

   wchar_t *name_prefix = wcsdup(name_lookup->name);
   name_prefix[wcslen(name_prefix)-2] = 0;
   //wprintf(L"Calling operator_wrapper as %ls\n", name);
   
    anna_type_t * t1 = anna_node_get_return_type(node->child[0], func->stack_template);
    anna_type_t * t2 = anna_node_get_return_type(node->child[1], func->stack_template);
    
    if(!t1) 
    {
	anna_error(node->child[1], L"Unknown type for first argument to operator %ls", name_lookup->name);
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    if(!t2) 
    {	
	anna_error(node->child[1], L"Unknown type for second argument to operator %ls", name_lookup->name);	
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    
    wchar_t *method_name = anna_find_method((anna_node_t *)node, t1, name_prefix, 2, t2);
    
    if(method_name)
    {
	    
	anna_node_t *mg_param[2]=
	    {
		node->child[0], (anna_node_t *)anna_node_lookup_create(&node->location,method_name)
	    }
	;
	
	anna_node_t *c_param[1]=
	    {
		node->child[1]
	    }
	;
	
	return (anna_node_t *)
	    anna_node_call_create(&node->location,
				  (anna_node_t *)
				  anna_node_call_create(&node->location,
							(anna_node_t *)
							anna_node_lookup_create(&node->location,
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
	method_name = anna_find_method((anna_node_t *)node, t2, reverse_name_prefix, 2, t1);
	sb_destroy(&buff);
	
	if(!method_name)
	{
	    FAIL(node, L"%ls__: No support for call with objects of types %ls and %ls\n",
		 name_prefix, t1->name, t2->name);
	}

	anna_node_t *mg_param[2]=
	    {
		node->child[1], (anna_node_t *)anna_node_lookup_create(&node->location, method_name)
	    }
	;
	
	anna_node_t *c_param[1]=
	    {
		node->child[0]
	    }
	;
	
	return (anna_node_t *)
	    anna_node_call_create(&node->location,
				  (anna_node_t *)
				  anna_node_call_create(&node->location,
							(anna_node_t *)
							anna_node_lookup_create(&node->location,
										L"__memberGet__"),
							2,
							mg_param),
				  1,
				  c_param);

	
    }
}

anna_node_t *anna_macro_iter(anna_node_call_t *node,
			     anna_function_t *func, 
			     anna_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"iteration macro", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_TYPE(node->function, ANNA_NODE_MEMBER_GET_WRAP);
    
    anna_node_member_get_t * mg = (anna_node_member_get_t *)node->function;
    int return_pop_count = 1+func->return_pop_count;
    anna_type_t *lst_type = anna_node_get_return_type(mg->object, func->stack_template);

    wchar_t * call_name = anna_mid_get_reverse(mg->mid);
    
    switch(node->child[0]->node_type)
    {
	case ANNA_NODE_LOOKUP:
	{
	    anna_node_lookup_t * value_name = (anna_node_lookup_t *)node->child[0];

	    
	    anna_type_t *argv[]=
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

	    
	    string_buffer_t sb;
	    sb_init(&sb);
	    sb_append(&sb, L"__");
	    sb_append(&sb, call_name);
	    sb_append(&sb, L"Value__");
	    
	    wchar_t *method_name = sb_content(&sb);
	    
	    size_t mid = anna_mid_get(method_name);
	    anna_type_t *member_type = anna_type_member_type_get(lst_type, method_name);
	    if(!member_type)
	    {
		anna_error((anna_node_t *)node, L"Unable to calculate type of member %ls of object of type %ls", method_name, lst_type->name);
		return (anna_node_t *)anna_node_null_create(&node->location); \
	    }
	    sb_destroy(&sb);

	    anna_node_t *function = (anna_node_t *)
		anna_node_dummy_create(&node->location,
				       anna_function_create(L"!anonymous", 0, node->child[1], 
							    null_type, 1, argv, &value_name->name, 
							    func->stack_template, 
							    return_pop_count)->wrapper,
				       1);
	    
	    
	    return (anna_node_t *)anna_node_call_create(&node->location,
							anna_node_member_get_create(&node->location,
										    mg->object,
										    mid,
										    member_type,
										    1),   
							1,
							&function);
	    
	}
	
	case ANNA_NODE_CALL:
	{
	    anna_node_lookup_t * key_name;
	    anna_node_lookup_t * value_name;

	    anna_node_call_t * decl = (anna_node_call_t *)node->child[0];
	    CHECK_NODE_LOOKUP_NAME(decl->function, L"Pair");
	    CHECK_INPUT_COUNT(decl, L"iteration macro", 2);
	    CHECK_NODE_TYPE(decl->child[0], ANNA_NODE_LOOKUP);
	    CHECK_NODE_TYPE(decl->child[1], ANNA_NODE_LOOKUP);
	    key_name = (anna_node_lookup_t *)decl->child[0];
	    value_name = (anna_node_lookup_t *)decl->child[1];
    
	    anna_type_t *argv[]=
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
	    
	    string_buffer_t sb;
	    sb_init(&sb);
	    sb_append(&sb, L"__");
	    sb_append(&sb, call_name);
	    sb_append(&sb, L"Pair__");
	    
	    wchar_t *method_name = sb_content(&sb);
	    
	    size_t mid = anna_mid_get(method_name);
	    anna_type_t *member_type = anna_type_member_type_get(lst_type, method_name);
	    if(!member_type)
	    {
		anna_error((anna_node_t *)node, L"Unable to calculate type of member %ls of object of type %ls", method_name, lst_type->name);
		return (anna_node_t *)anna_node_null_create(&node->location); \
	    }
	    sb_destroy(&sb);
	    
	    anna_node_t *function = (anna_node_t *)
		anna_node_dummy_create(&node->location,
				       anna_function_create(L"!anonymous", 0, node->child[1], 
							    null_type, 2, argv, argn,
							    func->stack_template, 
							    return_pop_count)->wrapper,
			       1);	    
	    
	    return (anna_node_t *)anna_node_call_create(&node->location,
							anna_node_member_get_create(&node->location,
										    mg->object,
										    mid,
										    member_type,
										    1),   
							1,
							&function);
	    
	}	
	default:
	    anna_error((anna_node_t *)node->child[0], L"Expected a value parameter name or a key:value parameter name pair");
	    return (anna_node_t *)anna_node_null_create(&node->location);
    }
    
}



static anna_node_t *anna_macro_get(anna_node_call_t *node, 
				   anna_function_t *func,
				   anna_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"__get__ operator", 2);
  anna_prepare_children(node, func, parent);
    
  anna_type_t * t1 = anna_node_get_return_type(node->child[0], func->stack_template);
  anna_type_t * t2 = anna_node_get_return_type(node->child[1], func->stack_template);
    
  wchar_t *method_name = anna_find_method((anna_node_t *)node, t1, L"__get", 2, t2);

  if(!method_name)
  {
      FAIL(node, L"Error: __get__: No support for call with objects of types %ls and %ls\n",
	      t1->name, t2->name);
  }
  
  anna_node_t *mg_param[2]=
    {
      node->child[0], (anna_node_t *)anna_node_lookup_create(&node->location, method_name)
    }
  ;
  
  anna_node_t *c_param[1]=
    {
      node->child[1]
    }
  ;
  
  anna_node_t *result = (anna_node_t *)
	anna_node_call_create(&node->location,
			      (anna_node_t *)
			      anna_node_call_create(&node->location,
						    (anna_node_t *)
						    anna_node_lookup_create(&node->location,
									    L"__memberGet__"),
						    2,
						    mg_param),
			      1,
			      c_param);
  /*
  anna_node_print(result);
  */
  return result;
  
}



static anna_node_t *anna_macro_set(anna_node_call_t *node, 
				   anna_function_t *func,
				   anna_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"__set__ operator", 3);
    anna_prepare_children(node, func, parent);
    
    anna_type_t * t1 = anna_node_get_return_type(node->child[0], func->stack_template);
    anna_type_t * t2 = anna_node_get_return_type(node->child[1], func->stack_template);
    
    wchar_t *method_name = anna_find_method((anna_node_t *)node, t1, L"__set", 3, t2);
    
    if(!method_name)
    {
	FAIL(node, L"__set__: No support for call with objects of types %ls and %ls\n",
	      t1->name, t2->name);
    }
	    
    anna_node_t *mg_param[2]=
	{
	    node->child[0], (anna_node_t *)anna_node_lookup_create(&node->location, method_name)
	}
    ;
    
    anna_node_t *c_param[2]=
	{
	    node->child[1], node->child[2]
	}
    ;
    
    anna_node_t *result = (anna_node_t *)
	anna_node_call_create(&node->location,
			      (anna_node_t *)
			      anna_node_call_create(&node->location,
						    (anna_node_t *)
						    anna_node_lookup_create(&node->location,
									    L"__memberGet__"),
						    2,
						    mg_param),
			      2,
			      c_param);
/*  wprintf(L"GGG\n");
  anna_node_print(result);
  wprintf(L"GGG\n");
*/
  return result;
  
}



static anna_node_t *anna_macro_declare(struct anna_node_call *node, 
				       struct anna_function *function,
				       struct anna_node_list *parent)
{
    CHECK_INPUT_COUNT(node,L"variable declaration", 3);
    assert(anna_parent_count(parent)==1);
    anna_prepare_children(node, function, parent);
    anna_node_lookup_t *name_lookup = node_cast_lookup(node->child[0]);
    anna_type_t *type;
    switch(node->child[1]->node_type) 
    {
	case ANNA_NODE_LOOKUP:
	{
	    anna_node_lookup_t *type_lookup;
	    type_lookup = node_cast_lookup(node->child[1]);
	    anna_object_t *type_wrapper = anna_stack_get_str(function->stack_template, type_lookup->name);
	    assert(type_wrapper);
	    type = anna_type_unwrap(type_wrapper);
	    break;
	}
	
       case ANNA_NODE_NULL:	
	   type = anna_node_get_return_type(node->child[2], function->stack_template);
	   //wprintf(L"Implicit var dec type: %ls\n", type->name);
	   break;

       default:
	   FAIL(node->child[1], L"Wrong type on second argument to declare - expected an identifier or a null node");

    }
    assert(type);
    anna_stack_declare(function->stack_template, name_lookup->name, type, null_object);

    anna_node_t *a_param[2]=
	{
	   node->child[0],
	   node->child[2]
	}
    ;
    
    return (anna_node_t *)
       anna_node_call_create(&node->location,
			     (anna_node_t *)anna_node_lookup_create(&node->location,
								    L"__assign__"),
			     2,
			     a_param);
}

static anna_node_t *anna_macro_member(anna_type_t *type,
				      struct anna_node_call *node, 
				      struct anna_function *function,
				      struct anna_node_list *parent)
{
    CHECK_INPUT_COUNT(node,L"variable declaration", 3);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_LOOKUP);
    anna_prepare_children(node, function, parent);
    anna_node_lookup_t *name_lookup = node_cast_lookup(node->child[0]);
    anna_type_t *var_type;
    switch(node->child[1]->node_type) 
    {
	case ANNA_NODE_LOOKUP:
	{
	    anna_node_lookup_t *type_lookup;
	    type_lookup = node_cast_lookup(node->child[1]);
	    anna_object_t *type_wrapper = anna_stack_get_str(function->stack_template, type_lookup->name);
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

    anna_member_create(type, -1, name_lookup->name, 0, var_type);
    
    anna_stack_declare(function->stack_template, name_lookup->name, type, null_object);
    
    /*
    anna_node_t *a_param[2]=
	{
	   node->child[0],
	   node->child[2]
	}
    ;
    
    return (anna_node_t *)
       anna_node_call_create(&node->location,
			     (anna_node_t *)anna_node_lookup_create(&node->location,
								    L"__assign__"),
			     2,
			     a_param);
    */
}

static anna_node_t *anna_macro_assign(struct anna_node_call *node, 
				      struct anna_function *function,
				      struct anna_node_list *parent)
{
    CHECK_INPUT_COUNT(node,L"= operator", 2);
   assert(anna_parent_count(parent)==1);

   switch(node->child[0]->node_type)
   {
       case ANNA_NODE_LOOKUP:
       {
	   anna_prepare_children(node, function, parent);
	   anna_node_lookup_t *name_lookup = node_cast_lookup(node->child[0]);
	   anna_sid_t sid = anna_stack_sid_create(function->stack_template, name_lookup->name);
	   
	   return (anna_node_t *)
	       anna_node_assign_create(&node->location,
				       sid,
				       node->child[1]);
       }
       
       case ANNA_NODE_CALL:
       {
	   anna_node_call_t *call = node_cast_call(node->child[0]);
	   //anna_node_print(call);
	   
	   anna_node_lookup_t *name_lookup = node_cast_lookup(call->function);
	   if(wcscmp(name_lookup->name, L"__get__")==0)
	   {
	       name_lookup->name=L"__set__";
	       anna_node_call_add_child(call, node->child[1]);
	       return (anna_node_t *)call;
	   }
	   else if(wcscmp(name_lookup->name, L"__memberGet__")==0)
	   {
	       name_lookup->name=L"__memberSet__";
	       anna_node_call_add_child(call, node->child[1]);
	       return (anna_node_t *)call;
	   }
       }
       
       default:
	   duck_error(node->child[0], L"Tried to assign to something that is not a variable");
	   return (anna_node_t *)anna_node_null_create(&node->location); \
   }
       
}

static anna_node_t *anna_macro_member_get(anna_node_call_t *node, 
					  anna_function_t *func, 
					  anna_node_list_t *parent)
{
/*
  wprintf(L"member_get on node at %d\n", node);
  anna_node_print((anna_node_t *)node);
  wprintf(L"\n");
*/
    CHECK_INPUT_COUNT(node,L". operator", 2);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_LOOKUP);
    
    anna_prepare_children(node, func, parent);
    anna_type_t *object_type = anna_node_get_return_type(node->child[0], func->stack_template);
    if(!object_type) 
    {
	anna_error(node->child[0], L"Tried to access member in object of unknown type");
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    
    anna_node_lookup_t *name_node = node_cast_lookup(node->child[1]);
    size_t mid = anna_mid_get(name_node->name);
    
    anna_type_t *member_type = anna_type_member_type_get(object_type, name_node->name);
    if(!member_type)
    {
	anna_error(node, L"Unable to calculate type of member %ls of object of type %ls", name_node->name, object_type->name);
	return (anna_node_t *)anna_node_null_create(&node->location);	\
    }
    
    int wrap = !!anna_static_member_addr_get_mid(member_type, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    return (anna_node_t *)anna_node_member_get_create(&node->location,
						      node->child[0], 
						      mid,
						      member_type,
						      wrap);
}

static anna_node_t *anna_macro_member_set(anna_node_call_t *node, 
					  anna_function_t *func, 
					  anna_node_list_t *parent)
{
/*
  wprintf(L"member_get on node at %d\n", node);
  anna_node_print((anna_node_t *)node);
  wprintf(L"\n");
*/
    CHECK_INPUT_COUNT(node,L"member assignment", 3);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_LOOKUP);
    
    anna_prepare_children(node, func, parent);
    anna_type_t *object_type = anna_node_get_return_type(node->child[0], func->stack_template);
    if(!object_type) 
    {
	anna_error(node->child[0], L"Tried to assign member in object of unknown type");
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    
    anna_node_lookup_t *name_node = node_cast_lookup(node->child[1]);
    size_t mid = anna_mid_get(name_node->name);
    
    anna_type_t *member_type = anna_type_member_type_get(object_type, name_node->name);
    
    return (anna_node_t *)anna_node_member_set_create(&node->location,
						      node->child[0], 
						      mid,
						      node->child[2],
						      member_type);
}

static anna_node_t *anna_macro_if(anna_node_call_t *node,
				  anna_function_t *func, 
				  anna_node_list_t *parent)
{
/*
   wprintf(L"member_get on node at %d\n", node);
   anna_node_print((anna_node_t *)node);
   wprintf(L"\n");
*/
    CHECK_INPUT_COUNT(node,L"if macro", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    
    anna_node_t *argv[] = {
        node->child[0], node->child[1], anna_node_null_create(&node->location)
    };
    
    return (anna_node_t *)
        anna_node_call_create(&node->location, 
			      (anna_node_t *)
			      anna_node_lookup_create(&node->location, 
						      L"__if__"),
			      3,
			      argv);
}

static anna_node_t *anna_macro_else(anna_node_call_t *node,
				    anna_function_t *func, 
				    anna_node_list_t *parent)
{
/*
   wprintf(L"member_get on node at %d\n", node);
   anna_node_print((anna_node_t *)node);
   wprintf(L"\n");
*/
    CHECK_INPUT_COUNT(node,L"else macro", 1);
    CHECK_NODE_BLOCK(node->child[0]);
    if(parent->idx == 0)
    {
	anna_error(node, L"else with no matching if call");
	return (anna_node_t *)anna_node_null_create(&node->location);	\
    }

    anna_node_call_t *parent_call = node_cast_call(parent->node);
    anna_node_t *prev = parent_call->child[parent->idx-1];
    /*
      anna_node_print(prev);   
      wprintf(L"\n");
    */
    anna_node_call_t *prev_call = node_cast_call(prev);
    anna_node_lookup_t *prev_call_name = node_cast_lookup(prev_call->function);
    
    if(wcscmp(prev_call_name->name, L"__if__")!=0)
    {
	anna_error(node, L"else with no matching if call");
	return (anna_node_t *)anna_node_null_create(&node->location);	\
    }
    
    if(prev_call->child_count != 3)
    {
	anna_error(prev_call, L"Bad if call");
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    
    if(prev_call->child[2]->node_type != ANNA_NODE_NULL)
    {
	anna_error(prev_call, L"Previous if statement already has an else clause");
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    
    prev_call->child[2] = anna_node_prepare(node->child[0], func, parent);
    
    return (anna_node_t *)
	anna_node_null_create(&node->location);
   
}

static anna_object_t *anna_function_or(anna_object_t **param)
{
  return param[0] == null_object?anna_function_wrapped_invoke(param[1], 0, 0, 0):param[0];
}

static anna_node_t *anna_macro_or(anna_node_call_t *node, anna_function_t *func, anna_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"or operator", 2);
    
    anna_prepare_children(node, func, parent);

    anna_type_t * t1 = anna_node_get_return_type(node->child[0], func->stack_template);
    anna_type_t * t2 = anna_node_get_return_type(node->child[1], func->stack_template);
    
    if(!t1) 
    {
	anna_error(node->child[1], L"Unknown type for first argument to operator or");
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    if(!t2) 
    {	
	anna_error(node->child[1], L"Unknown type for second argument to operator or");	
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    
    anna_type_t *return_type = anna_type_intersect(t1,t2);
    wchar_t *argn[]=
	{
	    L"condition1",
	    L"condition2"
	}
    ;
    
    anna_node_t *param[]=
	{
	    node->child[0],
	    (anna_node_t *)
	    anna_node_dummy_create(&node->location,
				   anna_function_create(L"!orConditionBlock", 0, 
							anna_node_call_create(&node->location,
									      (anna_node_t *)
									      anna_node_lookup_create(&node->location,
												      L"__block__"),
									      1,
									      &node->child[1]), 
							t2, 0, 0, 0, 
							func->stack_template, func->return_pop_count+1)->wrapper,
				   1)
	}
    ;
    anna_type_t *argv[]=
	{
	    t1,
	    anna_node_get_return_type(param[1], func->stack_template)
	}
    ;
    
    return (anna_node_t *)
	anna_node_call_create(&node->location, 
			      (anna_node_t *)
			      anna_node_dummy_create( &node->location,
						      anna_native_create(L"!orAnonymous",
									 ANNA_FUNCTION_FUNCTION,
									 (anna_native_t)anna_function_or,
									 return_type,
									 2,
									 argv,
									 argn)->wrapper,
						      0),
			      2,
			      param);
}

static anna_object_t *anna_function_and(anna_object_t **param)
{
  return (param[0] == null_object)?null_object:anna_function_wrapped_invoke(param[1], 0, 0, 0);
}

static anna_node_t *anna_macro_and(anna_node_call_t *node, anna_function_t *func, anna_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"and operator", 2);
    
    anna_prepare_children(node, func, parent);
    
    anna_type_t * t1 = anna_node_get_return_type(node->child[0], func->stack_template);
    anna_type_t * t2 = anna_node_get_return_type(node->child[1], func->stack_template);
    
    if(!t1) 
    {
	anna_error(node->child[1], L"Unknown type for first argument to operator and");
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    if(!t2) 
    {	
	anna_error(node->child[1], L"Unknown type for second argument to operator and");	
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    
    wchar_t *argn[]=
	{
	    L"condition1",
	    L"condition2"
	}
    ;
    anna_node_t *param[]=
	{
	    node->child[0],
	    (anna_node_t *)
	    anna_node_dummy_create(&node->location,
				   anna_function_create(L"!andConditionBlock", 0, 
							anna_node_call_create(&node->location,
									      (anna_node_t *)
									      anna_node_lookup_create(&node->location,
												      L"__block__"),
									      1,
									      &node->child[1]), 
							t2, 0, 0, 0, 
							func->stack_template, func->return_pop_count+1)->wrapper,
				   1)
	}
    ;

    anna_type_t *argv[]=
	{
	    t1,
	    anna_node_get_return_type(param[1], func->stack_template)
	}
    ;
    
    return (anna_node_t *)
	anna_node_call_create(&node->location, 
			      (anna_node_t *)
			      anna_node_dummy_create( &node->location,
						      anna_native_create(L"!andAnonymous",
									 ANNA_FUNCTION_FUNCTION,
									 (anna_native_t)anna_function_and,
									 t2,
									 2,
									 argv,
									 argn)->wrapper,
						      0),
			      2,
			      param);
}

static anna_object_t *anna_function_while(anna_object_t **param)
{
    anna_object_t *result = null_object;
    while(anna_function_wrapped_invoke(param[0], 0, 0, 0) != null_object)
    {
      result = anna_function_wrapped_invoke(param[1], 0, 0, 0);
    }
    return result;
}

static anna_node_t *anna_macro_while(anna_node_call_t *node, anna_function_t *func, anna_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"while macro", 2);
    CHECK_NODE_BLOCK(node->child[1]);

    anna_prepare_children(node, func, parent);

    anna_type_t *t2 = anna_node_get_return_type(node->child[1], func->stack_template);
    
    if(!t2) 
    {	
	anna_error(node->child[1], L"Unknown type for second argument to while");	
	return (anna_node_t *)anna_node_null_create(&node->location);	
    }
    
    anna_node_t *condition = 
	(anna_node_t *)
	anna_node_dummy_create(&node->location,
			       anna_function_create(L"!andConditionBlock", 0, 
						    anna_node_call_create(&node->location,
									  (anna_node_t *)
									  anna_node_lookup_create(&node->location,
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

    anna_node_t *param[]=
	{
	    condition,
	    node->child[1]
	}
    ;

    anna_type_t *argv[]=
	{
	    anna_node_get_return_type(param[0], func->stack_template),
	    t2
	}
    ;
    /*
      FIXME: I think the return values are all wrong here, need to
      make sure we're rturning the function result, not the functio
      type itself...
     */
    return (anna_node_t *)
	anna_node_call_create(&node->location,
			      (anna_node_t *)
			      anna_node_dummy_create( &node->location,
							 anna_native_create(L"!whileAnonymous",
									    ANNA_FUNCTION_FUNCTION,
									    (anna_native_t)anna_function_while,
									    t2,
									    2,
									    argv,
									    argn)->wrapper,
						      0),
			      2,
			      param);
}

static anna_node_t *anna_macro_type(anna_node_call_t *node, 
				    anna_function_t *func, 
				    anna_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"type macro", 4);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_LOOKUP);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_LOOKUP);
    CHECK_NODE_BLOCK(node->child[3]);

    wchar_t *name = ((anna_node_lookup_t *)node->child[0])->name;
    wchar_t *type_name = ((anna_node_lookup_t *)node->child[1])->name;
    int error_count=0;
    
    anna_type_t *type = anna_type_create(name, 64);
    anna_stack_declare(func->stack_template, name, type_type, type->wrapper);
    anna_node_call_t *body = (anna_node_call_t *)node->child[3];

    int i;
    for(i=0; i<body->child_count; i++)
    {
	anna_node_t *item = body->child[i];
	
	if(item->node_type != ANNA_NODE_CALL) 
	{
	    anna_error(item,
		       L"Only function declarations and variable declarations allowed directly inside a class body" );
	    error_count++;
	    continue;
	}
	
	anna_node_call_t *call = (anna_node_call_t *)item;
	if(call->function->node_type != ANNA_NODE_LOOKUP)
	{
	    anna_error(call->function,
		       L"Only function declarations and variable declarations allowed directly inside a class body" ); 
	    error_count++;
	    continue;
	}
	
	anna_node_lookup_t *declaration = (anna_node_lookup_t *)call->function;
	
	if(wcscmp(declaration->name, L"__function__")==0)
	{
	    anna_macro_function_internal(type, call, func, parent);
	}
	else if(wcscmp(declaration->name, L"__declare__")==0)
	{
	    anna_macro_member(type, call, func, parent);
	}
	else
	{
	    anna_error(call->function,
		       L"Only function declarations and variable declarations allowed directly inside a class body" );
	    error_count++;	    
	}
    }
    if(error_count)
	return (anna_node_t *)anna_node_null_create(&node->location);	\
    
    anna_object_t **constructor_ptr = anna_static_member_addr_get_mid(type, ANNA_MID_INIT_PAYLOAD);
    anna_function_t *constructor = anna_function_unwrap(*constructor_ptr);
    
    anna_type_t **argv= malloc(sizeof(anna_type_t *)*(constructor->input_count));
    wchar_t **argn= malloc(sizeof(wchar_t *)*(constructor->input_count));
    argv[0]=type_type;
    argn[0]=L"this";

    for(i=1; i<constructor->input_count; i++)
    {
	argv[i] = constructor->input_type[i];
	argn[i] = constructor->input_name[i];
    }
/*    
    anna_native_method_create(type, ANNA_MID_CALL_PAYLOAD, L"__call__",
			      0, (anna_native_t)&anna_construct,
			      type, constructor->input_count, argv, argn);
    wprintf(L"Create __call__ for non-native type %ls\n", type->name);
*/  
    return (anna_node_t *)anna_node_dummy_create(&node->location,
						 type->wrapper,
						 0);
}

static anna_node_t *anna_macro_return(anna_node_call_t *node, 
				      anna_function_t *func, 
				      anna_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node,L"return", 1);
    return (anna_node_t *)anna_node_return_create(&node->location, node->child[0], func->return_pop_count+1);
}

static anna_node_t *anna_macro_templatize(anna_node_call_t *node, 
					  anna_function_t *func, 
					  anna_node_list_t *parent)
{
    CHECK_INPUT_COUNT(node, L"template instantiation", 2);
    CHECK_TYPE(node->child[0]);
    
    anna_type_t *base_type = EXTRACT_TYPE(node->child[0]);

    wprintf(L"LALALA %ls\n", base_type->name);
    
    return base_type->wrapper;
    
    
    return (anna_node_t *)anna_node_null_create(&node->location);
}

static void anna_macro_add(anna_stack_frame_t *stack, 
			   wchar_t *name,
			   anna_native_macro_t call)
{
    anna_native_declare(stack, name, ANNA_FUNCTION_MACRO, (anna_native_t)call, 0, 0, 0, 0);
}


void anna_macro_init(anna_stack_frame_t *stack)
{
    int i;
    
    anna_macro_add(stack, L"__block__", &anna_macro_block);
    anna_macro_add(stack, L"__memberGet__", &anna_macro_member_get);
    anna_macro_add(stack, L"__memberSet__", &anna_macro_member_set);
    anna_macro_add(stack, L"__assign__", &anna_macro_assign);
    anna_macro_add(stack, L"__declare__", &anna_macro_declare);
    anna_macro_add(stack, L"__function__", &anna_macro_function);
    anna_macro_add(stack, L"if", &anna_macro_if);
    anna_macro_add(stack, L"else", &anna_macro_else);
    anna_macro_add(stack, L"__get__", &anna_macro_get);
    anna_macro_add(stack, L"__set__", &anna_macro_set);
    anna_macro_add(stack, L"__or__", &anna_macro_or);
    anna_macro_add(stack, L"__and__", &anna_macro_and);
    anna_macro_add(stack, L"while", &anna_macro_while);
    anna_macro_add(stack, L"__type__", &anna_macro_type);
    anna_macro_add(stack, L"return", &anna_macro_return);
    anna_macro_add(stack, L"__templatize__", &anna_macro_templatize);
    
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
	anna_macro_add(stack, op_names[i], &anna_macro_operator_wrapper);
    }

    /*
      anna_macro_add(stack, L"while", &anna_macro_while);
    */

}
