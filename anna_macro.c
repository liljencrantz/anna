#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_node_wrapper.h"
#include "anna_stack.h"
#include "anna_macro.h"
#include "anna_type.h"
#include "anna_function.h"
#include "anna_node_check.h"

static hash_table_t templatize_lookup;

typedef struct
{
    anna_type_t *base;
    size_t argc;
    anna_node_t **argv;
}
    templatize_key_t;

static wchar_t *anna_assign_operator_names[][2] = 
{
    {L"__increase__",L"__add__"},
    {L"__decrease__",L"__sub__"},
    {L"__append__",L"__join__"},
    {L"__next__",L"__getNext__"},
    {L"__prev__",L"__getPrev__"},
}
    ;


static int templatize_key_compare(void *k1, void *k2)
{
    templatize_key_t *key1 =(templatize_key_t *)k1;
    templatize_key_t *key2 =(templatize_key_t *)k2;
    if(key1->base != key2->base)
	return 0;
    if(key1->argc != key2->argc)
	return 0;
    int i;
    for(i=0; i<key1->argc; i++)
    {
	if(anna_node_compare(key1->argv[i], key2->argv[i]) == 0)
	    return 0;
    }
    return 1;
}

static int templatize_key_hash(void *k1)
{
    templatize_key_t *key1 =(templatize_key_t *)k1;
    int result;
   
    result = (int)key1->argc + (int)key1->base;
    return result;
}


static wchar_t *anna_find_method(anna_node_t *context, 
				 anna_type_t *type, 
				 wchar_t *prefix, 
				 size_t argc,
				 anna_type_t *arg2_type)
{
    int i;
    wchar_t **members = calloc(sizeof(wchar_t *), anna_type_member_count(type));
    wchar_t *match=0;
    int fault_count=0;
    
    assert(arg2_type);
    
    
    anna_type_get_member_names(type, members);    
    //wprintf(L"Searching for %ls[XXX]... in %ls\n", prefix, type->name);
    
    for(i=0; i<anna_type_member_count(type); i++)
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
	    if(mem_fun->argc != argc)
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

static anna_node_t *anna_macro_block(
    anna_node_call_t *node,
    anna_function_t *function,
    anna_node_list_t *parent)
{
    //wprintf(L"Create new block with %d elements at %d\n", node->child_count, node);
    int return_pop_count = 1+function->return_pop_count;
    
    anna_function_t *result = 
	anna_function_create(
	    L"!module", 
	    0,
	    node, 
	    0,
	    0,
	    0,
	    0,
	    function->stack_template, 
	    return_pop_count);
    al_push(&function->child_function, result);
    return (anna_node_t *)anna_node_dummy_create(
	&node->location,
	anna_function_wrap(result),
	1);
}

/**
   Macro to define a function. Expects 5 parameters:

   name of function (may be null)
   return type of function
   block of input declarations
   block of attributes
   body definition
*/

anna_node_t *anna_macro_function_internal(anna_type_t *type, 
					  anna_node_call_t *node, 
					  anna_function_t *function, 
					  anna_node_list_t *parent,
					  int declare)
{
    wchar_t *name=0;
    wchar_t *internal_name=0;
    int is_variadic=0;
    
    /*
      Set this to true if we need to snigg out the real function
      return type after we're done creating the function.
     */
    /*
    wprintf(L"\n\nCreate function from node:\n");
    anna_node_print(node);
    wprintf(L"\n");
    */
    CHECK_CHILD_COUNT(node,L"function definition", 5);
    
    if (node->child[0]->node_type == ANNA_NODE_IDENTIFIER) {
	
	anna_node_identifier_t *name_identifier = (anna_node_identifier_t *)node->child[0];
	internal_name = name = name_identifier->name;
    }
    else {
	CHECK_NODE_TYPE(node->child[0], ANNA_NODE_NULL);
	internal_name = L"!anonymous";
    }
    
    anna_node_t *body = node->child[4];
    
    if(body->node_type != ANNA_NODE_NULL && body->node_type != ANNA_NODE_CALL)
    {
        FAIL(body, L"Invalid function body");
    }
    
    anna_type_t *out_type=0;
    anna_node_t *out_type_wrapper = node->child[1];
    
    if(out_type_wrapper->node_type == ANNA_NODE_NULL) 
    {	
	if(body->node_type != ANNA_NODE_CALL)
	{
	    FAIL(body, L"Function declarations must have a return type");
	}
    }
    else
    {
        anna_node_prepare_child(node, 1, function, parent);
	out_type_wrapper = node->child[1];
	anna_node_identifier_t *type_identifier;
	type_identifier = node_cast_identifier(out_type_wrapper);
	anna_object_t *type_wrapper = anna_stack_get_str(
	    function->stack_template, 
	    type_identifier->name);

	if(!type_wrapper)
	{
	    FAIL(type_identifier, L"Unknown type: %ls", type_identifier->name);
	}
	out_type = anna_type_unwrap(type_wrapper);
    }
    
    size_t argc=0;
    anna_type_t **argv=0;
    wchar_t **argn=0;
    
    CHECK_NODE_TYPE(node->child[2], ANNA_NODE_CALL);
    anna_node_call_t *declarations = node_cast_call(node->child[2]);
    int i;
    if(declarations->child_count > 0 || type)
    {
	argc = declarations->child_count;
	if(type)
	{
	    argc++;
	}
	
	argv = malloc(sizeof(anna_type_t *)*argc);
	argn = malloc(sizeof(wchar_t *)*argc);
	
	if(type)
	{
	    argv[0]=type;
	    argn[0]=L"this";
	}
	
	for(i=0; i<declarations->child_count; i++)
	{
	    //declarations->child[i] = anna_node_prepare(declarations->child[i], function, parent);
	    CHECK_NODE_TYPE(declarations->child[i], ANNA_NODE_CALL);
	    anna_node_call_t *decl = node_cast_call(declarations->child[i]);
	    
	    CHECK_NODE_TYPE(decl->function, ANNA_NODE_IDENTIFIER);
	    anna_node_identifier_t *fun = node_cast_identifier(decl->function);
	    if(wcscmp(fun->name, L"__declare__") == 0)
	    {
		//CHECK_CHILD_COUNT(decl, L"variable declaration", 3);
		CHECK_NODE_TYPE(decl->child[0], ANNA_NODE_IDENTIFIER);
		anna_node_prepare_child(decl, 1, function, parent);
		anna_node_identifier_t *name = node_cast_identifier(decl->child[0]);
		anna_node_t *node = anna_node_prepare(decl->child[1], function, parent);
		
		anna_node_identifier_t *type_name = node_cast_identifier(decl->child[1]);
		
		anna_object_t **type_wrapper = anna_stack_addr_get_str(function->stack_template, type_name->name);
		if(!type_wrapper)
		{
		    anna_error((anna_node_t *)type_name, L"Unknown type: %ls", type_name->name);
		    return (anna_node_t *)anna_node_null_create(&node->location);	    
		}
		argv[i+!!type] = anna_type_unwrap(*type_wrapper);
		argn[i+!!type] = name->name;		
		
		if(decl->child_count ==3 && decl->child[2]->node_type == ANNA_NODE_IDENTIFIER) 
		{
		    anna_node_identifier_t *def = node_cast_identifier(decl->child[2]);
		    if(wcscmp(def->name,L"__variadic__") == 0)
		    {
			is_variadic = 1;
			if(i != (declarations->child_count-1))
			{
			    FAIL(def, L"Only the last argument to a function can be variadic");
			}
/*			else
			{
			    wprintf(L"Variadic function\n");
			}
*/
		    }
		}
		
	    }
	    else if(wcscmp(fun->name, L"__function__") == 0)
	    {
		anna_node_t *fun_wrap = anna_macro_function_internal(
		    0, decl, function, 
		    parent, 0);
		CHECK_NODE_TYPE(fun_wrap, ANNA_NODE_DUMMY);
		anna_node_dummy_t *fun_dummy = (anna_node_dummy_t *)fun_wrap;
		anna_function_t *fun = anna_function_unwrap(fun_dummy->payload);
		CHECK(fun, decl, L"Could not parse function declaration");		
		argv[i+!!type] = anna_function_wrap(fun)->type;
		argn[i+!!type] = fun->name;
	    }
	    else 
	    {
		FAIL(decl, L"Unknown declaration type");
	    }
	    
	}
    }

    if(type)
    {
	anna_function_t *result =
	    anna_function_create(
		internal_name, 
		(is_variadic?ANNA_FUNCTION_VARIADIC:0), 
		(anna_node_call_t *)body, 
		out_type, 
		argc,
		argv, 
		argn,
		function->stack_template,
		0);
	al_push(&function->child_function, result);
	assert(al_get_count(&function->child_function));
	
	if(!name)
	{
	    FAIL(node, L"Method definitions must have a name");
	}

	CHECK_NODE_BLOCK(body);
	
	anna_method_create(type, -1, name, 0, result);	
	return 0;
    }
    else
    {
	anna_function_t *result;
	if(body->node_type == ANNA_NODE_CALL) {
	    result = anna_function_create(internal_name, (is_variadic?ANNA_FUNCTION_VARIADIC:0), (anna_node_call_t *)body, out_type, argc, argv, argn, function->stack_template, 0);
	    al_push(&function->child_function, result);
/*	    
	    wprintf(L"Create subfunction %d in function %ls: %ls\n", 
		    al_get_count(&function->child_function),
		    function->name, result->name);
*/
	}
	else {
	    //wprintf(L"Creating emptry function as return for function declaration with no body for %ls\n", internal_name);
	    result = anna_native_create(internal_name, (is_variadic?ANNA_FUNCTION_VARIADIC:0), (anna_native_t)anna_i_null_function, out_type, argc, argv, argn);
	}
	
	if(name && declare) {
	    CHECK(name, node, L"Could not declare function, no function name given");
	    //wprintf(L"Declaring %ls as a function\n", name);
	    anna_stack_declare(function->stack_template, name, anna_type_for_function(result->return_type, result->input_count, result->input_type, result->input_name, is_variadic), anna_function_wrap(result));
	}
	return (anna_node_t *)anna_node_dummy_create(&node->location,
						     anna_function_wrap(result),
						     body->node_type == ANNA_NODE_CALL);
    }
    
}

static anna_node_t *anna_macro_function(anna_node_call_t *node,
					anna_function_t *function, 
					anna_node_list_t *parent)
{
    //return anna_macro_function_internal(0, node, function, parent, 1);
    anna_function_t *result;
    result = anna_function_create_from_definition(
	node,
	function->stack_template);
    
    al_push(&function->child_function, result);
    
    return (anna_node_t *)anna_node_dummy_create(
	&node->location,
	anna_function_wrap(result),
	1);
    /*
      FIXME: Last param, should it be 0 for feclarations???
    */
}

static anna_node_t *anna_macro_macro(anna_node_call_t *node,
					anna_function_t *function, 
					anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node,L"macro definition", 3);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_BLOCK(node->child[2]);
    anna_node_identifier_t *name_identifier = (anna_node_identifier_t *)node->child[0];
    anna_node_call_t *declarations = node_cast_call(node->child[1]);
    wchar_t **argn=calloc(sizeof(wchar_t *), 3);
    int i;
    CHECK_CHILD_COUNT(declarations,L"macro definition", 3);

    for(i=0; i<3; i++)
    {
	CHECK_NODE_TYPE(declarations->child[i], ANNA_NODE_IDENTIFIER);
	anna_node_identifier_t *arg = (anna_node_identifier_t *)declarations->child[i];
	argn[i] = wcsdup(arg->name);
    }

    anna_function_t *result;
    result = anna_function_create(
	name_identifier->name,
	ANNA_FUNCTION_MACRO,
	(anna_node_call_t *)node->child[2], 
	0,
	3, 
	0,
	argn, 
	function->stack_template, 
	0);
    al_push(&function->child_function, result);
    
    anna_stack_declare(
	function->stack_template,
	name_identifier->name, 
	anna_type_for_function(
	    result->return_type, 
	    result->input_count, 
	    result->input_type,
	    result->input_name,
	    0), 
	anna_function_wrap(result));
    
    return (anna_node_t *)anna_node_dummy_create(
	&node->location,
	anna_function_wrap(result),
	0);
}


anna_function_type_key_t *anna_function_key_get(anna_type_t *type,
						wchar_t *name)
{
    anna_type_t *member_type = anna_type_member_type_get(type, name);
    if(!member_type)
    {
	return 0;
    }

    anna_object_t **key_ptr=anna_static_member_addr_get_mid(
	member_type, 
	ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    return key_ptr?(anna_function_type_key_t *)(*key_ptr):0;
}


anna_node_t *anna_macro_iter(anna_node_call_t *node,
			     anna_function_t *function, 
			     anna_node_list_t *parent)
{
/*
    wprintf(L"LALALA\n");
    anna_node_print((anna_node_t *)node->function);
    wprintf(L"\n");
*/
    CHECK_CHILD_COUNT(node,L"iteration macro", 2);
    
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_TYPE(node->function, ANNA_NODE_CALL);

    anna_node_call_t * mg = (anna_node_call_t *)node->function;
    mg->child[0] = anna_node_prepare(mg->child[0], function, parent);

    CHECK_CHILD_COUNT(mg, L"iteration macro", 2);
    CHECK_NODE_TYPE(mg->child[1], ANNA_NODE_IDENTIFIER);
    
    anna_node_identifier_t * call_name_id = (anna_node_identifier_t *)mg->child[1];
    
    int return_pop_count = 1+function->return_pop_count;
    anna_type_t *lst_type = anna_node_get_return_type(mg->child[0], function->stack_template);
    
    wchar_t * call_name = call_name_id->name;
    
    anna_type_t *argv[]=
	{
	    0, 0
	}
    ;
    wchar_t **argn;
	    

    string_buffer_t sb;
    sb_init(&sb);
    sb_append(&sb, L"__");
    sb_append(&sb, call_name);
    sb_append(&sb, L"__");
    
    wchar_t *method_name = sb_content(&sb);
    
    size_t mid = anna_mid_get(method_name);

    anna_type_t *member_type = anna_type_member_type_get(lst_type, method_name);
    
    CHECK(member_type, node, L"No method named %ls in type %ls\n", method_name, lst_type->name);

    switch(node->child[0]->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t * value_name = (anna_node_identifier_t *)node->child[0];
	    
	    wchar_t *argn_c[]=
		{
		    L"!key", value_name->name
		}
	    ;
	    argn = argn_c;
	    break;
	    
	}
	
	case ANNA_NODE_CALL:
	{
	    anna_node_identifier_t * key_name;
	    anna_node_identifier_t * value_name;

	    anna_node_call_t * decl = (anna_node_call_t *)node->child[0];
	    CHECK_NODE_IDENTIFIER_NAME(decl->function, L"Pair");
	    CHECK_CHILD_COUNT(decl, L"iteration macro", 2);
	    CHECK_NODE_TYPE(decl->child[0], ANNA_NODE_IDENTIFIER);
	    CHECK_NODE_TYPE(decl->child[1], ANNA_NODE_IDENTIFIER);
	    
	    key_name = (anna_node_identifier_t *)decl->child[0];
	    value_name = (anna_node_identifier_t *)decl->child[1];
	    
	    wchar_t *argn_c[]=
		{
		    key_name->name, value_name->name
		}
	    ;
	    argn = argn_c;
	    break;
	    
	}	
	default:
	    FAIL(
		node->child[0], 
		L"Expected a value parameter name or a key:value parameter name pair");
    }

    anna_function_type_key_t *function_key = anna_function_key_get(lst_type, method_name);
    CHECK(function_key, node, L"Not a function: %ls", method_name);
    anna_object_t **sub_key_ptr=anna_static_member_addr_get_mid(function_key->argv[1], ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    CHECK(sub_key_ptr, node, L"Not a function: %ls", function_key->argv[1]->name);
    anna_function_type_key_t *sub_function_key = (anna_function_type_key_t *)(*sub_key_ptr);
    CHECK(sub_function_key->argc == 2, node, L"Function %ls has wrong number of parameters: Got %d, expected %d", method_name, sub_function_key->argc, 2);
    argv[0] = sub_function_key->argv[0];
    argv[1] = sub_function_key->argv[1];
    
    if(!member_type)
    {
	FAIL(
	    node, 
	    L"Unable to calculate type of member %ls of object of type %ls", 
	    method_name, lst_type->name);
    }
    sb_destroy(&sb);
	    

    anna_function_t *sub_func =
	anna_function_create(
	    L"!anonymous", 0, 
	    (anna_node_call_t *)node->child[1], 
	    0, 2, argv, argn,
	    function->stack_template, 
	    return_pop_count);
    
    al_push(&function->child_function, sub_func);
    
    anna_node_t *iter_function = (anna_node_t *)
	anna_node_dummy_create(
	    &node->location,
	    anna_function_wrap(sub_func),
	    1);	    

    return (anna_node_t *)anna_node_call_create(
	&node->location,
	(anna_node_t *)
	anna_node_member_get_create(
	    &node->location,
	    mg->child[0],
	    mid,
	    member_type,
	    1),   
	1,
	&iter_function);
    
    
}




static anna_node_t *anna_macro_declare(struct anna_node_call *node, 
				       struct anna_function *function,
				       struct anna_node_list *parent)
{
    CHECK_CHILD_COUNT(node,L"variable declaration", 3);
    CHECK_PARENT_IS_ROOT;
    
    anna_node_prepare_child(node, 1, function, parent);
    anna_node_prepare_child(node, 2, function, parent);
    anna_node_identifier_t *name_identifier = node_cast_identifier(node->child[0]);
    anna_type_t *type;
    switch(node->child[1]->node_type) 
    {
	case ANNA_NODE_IDENTIFIER:
	{
	    CHECK_TYPE(node->child[1]);
	    type = EXTRACT_TYPE(node->child[1]);
	    break;
	}
	
	case ANNA_NODE_NULL:	
	    CHECK(node->child[2]->node_type != ANNA_NODE_NULL, node, L"Unspecified variable type");
	  	    
	    type = anna_node_get_return_type(node->child[2], function->stack_template);
	    CHECK(type, node->child[2], L"Could not determine return type");
	    	    
	    break;

	default:
	    FAIL(node->child[1], L"Wrong type on second argument to declare - expected an identifier or a null node");

    }
    anna_stack_declare(function->stack_template, name_identifier->name, type, null_object);

    anna_node_t *a_param[2]=
	{
	    node->child[0],
	    node->child[2]
	}
    ;
    
    return (anna_node_t *)
	anna_node_call_create(&node->location,
			      (anna_node_t *)anna_node_identifier_create(&node->location,
									 L"__assign__"),
			      2,
			      a_param);
}


static anna_node_t *anna_macro_type(anna_node_call_t *node, 
				    anna_function_t *function, 
				    anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node,L"type macro", 4);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[3]);

    wchar_t *name = ((anna_node_identifier_t *)node->child[0])->name;
    anna_type_t *type = anna_type_create(name, function->stack_template);

    type->definition = node;
    anna_stack_declare(function->stack_template, name, type_type, anna_type_wrap(type));
    al_push(
	&function->child_type,
	type);
    
    return (anna_node_t *)anna_node_dummy_create(&node->location,
						 anna_type_wrap(type),
						 0);
}


static anna_node_t *anna_macro_return(anna_node_call_t *node, 
				      anna_function_t *function, 
				      anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node,L"return", 1);
    return (anna_node_t *)anna_node_return_create(&node->location, anna_node_prepare(node->child[0], function, parent), function->return_pop_count+1);
}

static anna_node_t *anna_macro_ast(anna_node_call_t *node, 
				      anna_function_t *function, 
				      anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node,L"return", 1);
    return (anna_node_t *)anna_node_dummy_create(
	&node->location,
	anna_node_wrap(node->child[0]),
	0);
}

static anna_node_t *anna_macro_list(anna_node_call_t *node, 
				      anna_function_t *function, 
				      anna_node_list_t *parent)
{
    return (anna_node_t *)anna_node_call_create(
	&node->location,
	(anna_node_t *)anna_node_identifier_create(
	    &node->function->location,
	    L"List"), 
	node->child_count,
	node->child);
}

static anna_node_t *anna_macro_templatize(anna_node_call_t *node, 
					  anna_function_t *function, 
					  anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node, L"template instantiation", 2);
    CHECK_TYPE(node->child[0]);
    CHECK_NODE_BLOCK(node->child[1]);
    int i;
    
    anna_type_t *base_type = EXTRACT_TYPE(node->child[0]);
    anna_node_call_t *param = 
	(anna_node_call_t *)node->child[1];
    
    templatize_key_t key;
    key.base = base_type;
    key.argc = param->child_count;
    
    key.argv = param->child;    
    anna_type_t *result = (anna_type_t *)hash_get(&templatize_lookup, &key);
    if(result)
    {
	return (anna_node_t *)anna_node_identifier_create(&node->location,
							  result->name);    
    }
    
    anna_type_t *type = anna_type_create(L"!temporaryTypeName", function->stack_template);
    templatize_key_t *new_key = malloc(sizeof(templatize_key_t));
    memcpy(new_key, &key,sizeof(templatize_key_t));
    new_key->argv = malloc(sizeof(anna_node_t *)*new_key->argc);
    memcpy(new_key->argv,param->child,  sizeof(anna_node_t *)*new_key->argc);
    hash_put(&templatize_lookup, new_key, type);
    
    string_buffer_t sb_name;
    sb_init(&sb_name);
        
    anna_node_call_t *definition = 
	(anna_node_call_t *)anna_node_clone_deep((anna_node_t *)base_type->definition);
    sb_append(&sb_name, base_type->name);
    sb_append(&sb_name, L"<");
    
    int param_done=0;
    
//    wprintf(L"We will templatize the %ls type\n", base_type->name);
    anna_node_call_t *attribute_list = 
        (anna_node_call_t *)definition->child[2];
/* 
   anna_node_print(base_type->definition);
   wprintf(L"\n");
*/  
    
    for(i=0; i<attribute_list->child_count; i++)
    {
	CHECK_NODE_TYPE(attribute_list->child[i], ANNA_NODE_CALL);
	anna_node_call_t *attribute = 
	    (anna_node_call_t *)attribute_list->child[i];
	
	CHECK_NODE_TYPE(attribute->function, ANNA_NODE_IDENTIFIER);
	CHECK_CHILD_COUNT(attribute, L"template instantiation", 1);
	
	anna_node_identifier_t *id = 
	    (anna_node_identifier_t *)attribute->function;
	if(wcscmp(id->name, L"template") != 0)
	    continue;
	
	CHECK_NODE_TYPE(attribute->child[0], ANNA_NODE_CALL);
	
	anna_node_call_t *pair = 
	    (anna_node_call_t *)attribute->child[0];
	
	CHECK_NODE_IDENTIFIER_NAME(pair->function, L"Pair");
	CHECK_CHILD_COUNT(pair, L"template instantiation", 2);
	CHECK_NODE_TYPE(pair->child[0], ANNA_NODE_IDENTIFIER);

	pair->child[1] = param->child[param_done++];
	sb_printf(&sb_name, L"%d", pair->child[1]);
    }
    sb_append(&sb_name, L">");
    
/*
  wprintf(L"Result\n");
  anna_node_print(definition);
  wprintf(L"\n");    
*/  
    wchar_t *name = sb_content(&sb_name); 
    type->name = name;
   
    type->definition = definition;
    anna_stack_declare(function->stack_template, name, type_type, anna_type_wrap(type));
/*
    anna_node_t *type_result = anna_macro_type_setup(type, function, parent);
    if(type_result->node_type == ANNA_NODE_NULL){
	return type_result;
    }
*/
    al_push(
	&function->child_type,
	type);

    return (anna_node_t *)
	anna_node_identifier_create(
	    &node->location,
	    name);    
}


static void anna_macro_add(anna_stack_frame_t *stack, 
			   wchar_t *name,
			   anna_native_macro_t call)
{
    anna_native_declare(stack, name, ANNA_FUNCTION_MACRO, (anna_native_t)call, 0, 0, 0, 0);
}

#include "anna_macro_attribute.c"
#include "anna_macro_conditional.c"
#include "anna_macro_operator.c"
#include "anna_macro_cast.c"


void anna_macro_init(anna_stack_frame_t *stack)
{
    int i;
    hash_init(&templatize_lookup,
	      &templatize_key_hash,
	      &templatize_key_compare);

    anna_macro_add(stack, L"__block__", &anna_macro_block);
    anna_macro_add(stack, L"__memberGet__", &anna_macro_member_get);
    anna_macro_add(stack, L"__memberSet__", &anna_macro_member_set);
    anna_macro_add(stack, L"__assign__", &anna_macro_assign);
    anna_macro_add(stack, L"__declare__", &anna_macro_declare);
    anna_macro_add(stack, L"__function__", &anna_macro_function);
    anna_macro_add(stack, L"__macro__", &anna_macro_macro);
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
    anna_macro_add(stack, L"__templateAttribute__", &anna_macro_template_attribute);
    anna_macro_add(stack, L"__extendsAttribute__", &anna_macro_extends_attribute);
    anna_macro_add(stack, L"__genericOperator__", &anna_macro_operator_wrapper);
    anna_macro_add(stack, L"each", &anna_macro_iter);
    anna_macro_add(stack, L"map", &anna_macro_iter);
    anna_macro_add(stack, L"filter", &anna_macro_iter);
    anna_macro_add(stack, L"first", &anna_macro_iter);
    anna_macro_add(stack, L"__list__", &anna_macro_list);
    anna_macro_add(stack, L"cast", &anna_macro_cast);
    anna_macro_add(stack, L"__as__", &anna_macro_as);
    anna_macro_add(stack, L"AST", &anna_macro_ast);
    
    wchar_t *op_names[] = 
	{
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
	}
    ;


    for(i =0; i<sizeof(op_names)/sizeof(wchar_t *); i++)
    {
	anna_macro_add(stack, op_names[i], &anna_macro_operator_wrapper);
    }

    for(i =0; i<sizeof(anna_assign_operator_names)/sizeof(wchar_t[2]); i++)
    {
	anna_macro_add(
	    stack,
	    anna_assign_operator_names[i][0],
	    &anna_macro_assign_operator);
    }


    /*
      anna_macro_add(stack, L"while", &anna_macro_while);
    */

}
