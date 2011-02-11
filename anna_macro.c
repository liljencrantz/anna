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
#include "anna_util.h"
#include "anna_function.h"
#include "anna_prepare.h"
#include "anna_node_check.h"
#include "anna_node_create.h"

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

/*
static anna_node_t *anna_macro_module(
    anna_node_call_t *node,
    anna_function_t *function,
    anna_node_list_t *parent)
{
//    wprintf(L"Create new module with %d elements at %d\n", node->child_count, node);
    int return_pop_count = 1+function->return_pop_count;

    anna_function_t *result = 
	anna_function_create_from_block(
	    node,
	    function->stack_template,
	    return_pop_count);
    result->definition->child[1] = anna_node_create_identifier(0, L"Object");
    result->flags |= ANNA_FUNCTION_MODULE;
    assert(object_type);
    result->name = L"!moduleMacroFunction";
    
    //al_push(&function->child_function, result);
    return (anna_node_t *)anna_node_create_dummy(
	&node->location,
	anna_function_wrap(result),
	1);
}
*/
static anna_node_t *anna_macro_macro(anna_node_call_t *node)
{

    CHECK_CHILD_COUNT(node,L"macro definition", 3);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_BLOCK(node->child[2]);
/*
    anna_function_t *result;
    result = anna_function_create_from_definition(
	node,
	function->stack_template);
    result->flags |= ANNA_FUNCTION_MACRO;
    al_push(&function->child_function, result);
    
    anna_node_t *res = (anna_node_t *)anna_node_create_dummy(
	&node->location,
	anna_function_wrap(result),
	1);
      //FIXME: Last param, should it be 0 for feclarations???
    
    //wprintf(L"LALALA %d %d\n", res, anna_function_wrap(result));
    return res;
*/

    anna_node_identifier_t *name_identifier = (anna_node_identifier_t *)node->child[0];
    anna_node_call_t *declarations = node_cast_call(node->child[1]);
    CHECK_CHILD_COUNT(declarations,L"macro definition", 1);
    CHECK_NODE_TYPE(declarations->child[0], ANNA_NODE_IDENTIFIER);
    anna_node_identifier_t *arg = (anna_node_identifier_t *)declarations->child[0];
    
    //anna_stack_print(function->stack_template);
    
    anna_function_t *result;
    result = anna_macro_create(
	name_identifier->name,
	node,
	arg->name);
    return (anna_node_t *)
	anna_node_create_declare(
	    &node->location,
	    name_identifier->name,
	    anna_node_create_null(&node->location),
	    anna_node_create_closure(
		&node->location,
		result
		)
	    );
/*
    
    anna_stack_declare(
	function->stack_template,
	name_identifier->name, 
	anna_type_for_function(
	    result->return_type, 
	    result->input_count, 
	    result->input_type,
	    result->input_name,
	    0), 
	anna_function_wrap(result),
	0);

    return (anna_node_t *)anna_node_create_dummy(
	&node->location,
	anna_function_wrap(result),
	0);
*/  
}

#if 0

static anna_function_type_key_t *anna_function_key_get(anna_type_t *type,
						wchar_t *name)
{
    anna_type_t *member_type = anna_type_member_type_get(type, name);
    anna_prepare_type_interface(member_type);
    
    if(!member_type)
    {
	return 0;
    }
    
    anna_object_t **key_ptr=anna_static_member_addr_get_mid(
	member_type, 
	ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    return key_ptr?(anna_function_type_key_t *)(*key_ptr):0;
}
#endif
/*
anna_node_t *anna_macro_iter(anna_node_call_t *node,
			     anna_function_t *function, 
			     anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node,L"iteration macro", 2);
    
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_TYPE(node->function, ANNA_NODE_CALL);

    anna_node_call_t * mg = (anna_node_call_t *)node->function;
    mg->child[0] = anna_node_macro_expand(mg->child[0]);

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
    
    mid_t mid = anna_mid_get(method_name);

    anna_prepare_type_interface(lst_type);
    
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
    
    anna_object_t **sub_key_ptr = anna_static_member_addr_get_mid(function_key->argv[1], ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
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
	    anna_util_identifier_generate(L"anonymousIterationFunction", &(node->location)), 0, 
	    (anna_node_call_t *)node->child[1], 
	    object_type, 2, argv, argn,
	    function->stack_template, 
	    return_pop_count);
    
    al_push(&function->child_function, sub_func);
    
    anna_node_t *iter_function = (anna_node_t *)
	anna_node_create_dummy(
	    &node->location,
	    anna_function_wrap(sub_func),
	    1);	    

    return (anna_node_t *)anna_node_create_call(
	&node->location,
	(anna_node_t *)
	anna_node_create_member_get(
	    &node->location,
	    mg->child[0],
	    mid,
	    member_type,
	    1),   
	1,
	&iter_function);   
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
    anna_stack_declare(function->stack_template, name, type_type, anna_type_wrap(type), 0);
    //wprintf(L"Registered type %ls\n", name);
    
    al_push(
	&function->child_type,
	type);
    
    return (anna_node_t *)anna_node_create_dummy(&node->location,
						 anna_type_wrap(type),
						 0);
}


static anna_node_t *anna_macro_return(
    anna_node_call_t *node, 
    anna_function_t *function, 
    anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node,L"return", 1);
    return (anna_node_t *)anna_node_create_return(&node->location, anna_node_macro_expand(node->child[0]), function->return_pop_count+1);
}

static anna_node_t *anna_macro_collection(
    anna_node_call_t *node, 
    anna_function_t *function, 
    anna_node_list_t *parent)
{
    return (anna_node_t *)anna_node_create_call(
	&node->location,
	(anna_node_t *)anna_node_create_identifier(
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
	return (anna_node_t *)anna_node_create_identifier(&node->location,
							  result->name);    
    }
    
    anna_type_t *type = anna_type_create(L"!temporaryTypeName", base_type->stack);

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
	sb_printf(&sb_name, L"%ls%d", i==0?L"":L",", pair->child[1]);
    }
    sb_append(&sb_name, L">");
    
    wchar_t *name = sb_content(&sb_name); 
    type->name = name;
   
    type->definition = definition;
    anna_stack_declare(stack_global, name, type_type, anna_type_wrap(type), 0);
//    wprintf(L"Declare templatized type %ls in stack\n", type->name);
    //anna_stack_print_trace(stack_global);
    
    al_push(
	&function->child_type,
	type);

    return (anna_node_t *)
	anna_node_create_identifier(
	    &node->location,
	    name);    
}

static anna_node_t *anna_macro_import(
    anna_node_call_t *node, 
    anna_function_t *function, 
    anna_node_list_t *parent)
{
    return (anna_node_t *)anna_node_create_import(
	&node->location, 
	node->child[0]);
    
}
*/

static anna_node_t *anna_macro_ast(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"ast", 1);
    return (anna_node_t *)anna_node_create_dummy(
	&node->location,
	anna_node_wrap(node->child[0]),
	0);
}

static anna_node_t *anna_macro_def(
    anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node, L"__def__", 5);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);

    anna_function_t *fun = 
	anna_function_create_from_definition(
	    node);
    assert(fun);
    
    return (anna_node_t *)anna_node_create_closure(
	&node->location,
	fun);    
}

static anna_node_t *anna_macro_block(
    anna_node_call_t *node)
{
    //wprintf(L"Create new block with %d elements at %d\n", node->child_count, node);
     
    anna_function_t *fun = 
	anna_function_create_from_block(
	    node);

    return (anna_node_t *)anna_node_create_closure(
	&node->location,
	fun);    
}

static anna_node_t *anna_macro_declare(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"variable declaration", 3);
    
    anna_node_identifier_t *name = node_cast_identifier(node->child[0]);
    
    return (anna_node_t *)
	anna_node_create_declare(
	    &node->location,
	    name->name,
	    node->child[1],
	    node->child[2]
	    );
}


static void anna_macro_add(
    anna_stack_frame_t *stack, 
    wchar_t *name,
    anna_native_macro_t call)
{
    
    anna_function_t *f = anna_native_create(
	name,
	ANNA_FUNCTION_MACRO,
	(anna_native_t)call,
	node_call_wrapper_type,
	0,
	0,
	0,
	stack);
    
    anna_function_setup_interface(f, stack);
    anna_function_setup_body(f, stack);
    anna_stack_declare(
	stack,
	name,
	f->wrapper->type,
	f->wrapper,
	0);
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

    anna_macro_add(stack, L"__def__", &anna_macro_def);
    anna_macro_add(stack, L"__block__", &anna_macro_block);
    anna_macro_add(stack, L"__memberGet__", &anna_macro_member_get);
    anna_macro_add(stack, L"__memberSet__", &anna_macro_member_set);
    anna_macro_add(stack, L"__declare__", &anna_macro_declare);
    anna_macro_add(stack, L"__or__", &anna_macro_or);
    anna_macro_add(stack, L"__and__", &anna_macro_and);
    anna_macro_add(stack, L"__if__", &anna_macro_if);
    anna_macro_add(stack, L"ast", &anna_macro_ast);
    anna_macro_add(stack, L"__assign__", &anna_macro_assign);
    anna_macro_add(stack, L"__macro__", &anna_macro_macro);

/*    
    anna_macro_add(stack, L"__module__", &anna_macro_module);
    anna_macro_add(stack, L"__function__", &anna_macro_function);
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
    anna_macro_add(stack, L"__collection__", &anna_macro_collection);
    anna_macro_add(stack, L"cast", &anna_macro_cast);
    anna_macro_add(stack, L"__as__", &anna_macro_as);
    anna_macro_add(stack, L"import", &anna_macro_import);
*/  
/*
    for(i =0; i<sizeof(anna_assign_operator_names)/sizeof(wchar_t[2]); i++)
    {
	anna_macro_add(
	    stack,
	    anna_assign_operator_names[i][0],
	    &anna_macro_assign_operator);
    }
*/

    /*
      anna_macro_add(stack, L"while", &anna_macro_while);
    */

}
