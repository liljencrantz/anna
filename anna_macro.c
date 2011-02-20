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

//static hash_table_t templatize_lookup;
/*
typedef struct
{
    anna_type_t *base;
    size_t argc;
    anna_node_t **argv;
}
    templatize_key_t;
*/
/*
static wchar_t *anna_assign_operator_names[][2] = 
{
    {L"__increase__",L"__add__"},
    {L"__decrease__",L"__sub__"},
    {L"__append__",L"__join__"},
    {L"__next__",L"__getNext__"},
    {L"__prev__",L"__getPrev__"},
}
    ;
*/
/*
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
*/
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
	    (anna_node_t *)anna_node_create_closure(
		&node->location,
		result
		)
	    );
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

static anna_node_t *anna_macro_iter_declare(anna_node_t *id)
{
    anna_node_t *param[] ={
	id,
	(anna_node_t *)anna_node_create_null(&id->location),
	(anna_node_t *)anna_node_create_null(&id->location),
    };

    return (anna_node_t *)anna_node_create_call(
	&id->location,
	(anna_node_t *)anna_node_create_identifier(&id->location,L"__declare__"),
	3,
	param);    
}

anna_node_t *anna_macro_iter(anna_node_call_t *node)			    
{
    anna_node_t *n[]={0,0};
    anna_node_t *body;
    if(node->child_count < 2)
    {
	anna_error((anna_node_t *)node,
		   L"Invalid arguments for iteration");
    }
    if(node->child_count > 3)
    {
	anna_error((anna_node_t *)node,
		   L"Invalid arguments for iteration");
    }
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[node->child_count-1]);
    body = node->child[node->child_count-1];
    if(node->child_count == 2)
    {
	n[0] = anna_macro_iter_declare((anna_node_t *)anna_node_create_identifier(
	    &body->location,
	    L"!unused"));
	n[1] = anna_macro_iter_declare(node->child[0]);
    }
    else
    {
	CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
	n[0] = anna_macro_iter_declare(node->child[0]);
	n[1] = anna_macro_iter_declare(node->child[1]);
    }
    
    anna_node_call_t *attribute_list = anna_node_create_block(&body->location, 0, 0);
    anna_node_call_t *declaration_list = anna_node_create_block(&body->location, 2, n);
    
    anna_node_t *param[] ={
	(anna_node_t *)anna_node_create_identifier(
	    &body->location,
	    L"!anonymous"),
	(anna_node_t *)anna_node_create_null(&body->location),
	(anna_node_t *)declaration_list, 
	(anna_node_t *)attribute_list, 
	body
    };
    
    node->child[0] = (anna_node_t *)anna_node_create_call(
	&body->location,
	(anna_node_t *)anna_node_create_identifier(&body->location,L"__def__"), 
	5, 
	param);
    
    node->child_count = 1;
    anna_node_call_t *fun = (anna_node_call_t *)node->function;

    anna_node_identifier_t *orig_name = (anna_node_identifier_t *)fun->child[1];

    string_buffer_t sb;
    sb_init(&sb);
    sb_append(&sb, L"__");
    sb_append(&sb, orig_name->name);
    sb_append(&sb, L"__");
    

    fun->child[1] = (anna_node_t *)anna_node_create_identifier(
	&fun->child[1]->location,
	sb_content(&sb));
    sb_destroy(&sb);
    
    return (anna_node_t *)node;
    
}

static anna_node_t *anna_macro_type(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"type macro", 3);
    CHECK_NODE_TYPE(node->child[0], ANNA_NODE_IDENTIFIER);
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_BLOCK(node->child[2]);
    
    wchar_t *name = ((anna_node_identifier_t *)node->child[0])->name;
    anna_type_t *type = anna_type_create(name, node);
    
    return (anna_node_t *)anna_node_create_type(
	&node->location,
	type);
}


static anna_node_t *anna_macro_return(
    anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"return", 1);
    return (anna_node_t *)anna_node_create_return(&node->location, node->child[0]);
}

#if 0
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
#endif

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
    anna_function_setup_body(f);
    anna_stack_declare(
	stack,
	name,
	f->wrapper->type,
	f->wrapper,
	0);
}

static anna_node_t *anna_macro_specialize(anna_node_call_t *node)
{
    CHECK_CHILD_COUNT(node,L"type specialization", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    anna_node_call_t *res = (anna_node_call_t *)node->child[1];
    res->function = node->child[0];
    res->node_type = ANNA_NODE_SPECIALIZE;
    res->location = node->location;   
    return node->child[1];
}

static anna_node_t *anna_macro_collection(anna_node_call_t *node)
{
    if(node->child_count == 0)
    {
	anna_error((anna_node_t *)node,
		   L"At least one argument required");
	return (anna_node_t *)anna_node_create_null(&node->location);
    }

    anna_node_t *param[] = 
	{
	    (anna_node_t *)anna_node_create_type_lookup(
		&node->function->location,
		node->child[0])
	}
    ;

    node->function = (anna_node_t *)anna_node_create_specialize(
	&node->function->location,
	(anna_node_t *)anna_node_create_dummy(
	    &node->function->location,
	    anna_type_wrap(list_type), 0),
	1,
	param);
    return (anna_node_t *)node;
}

#include "anna_macro_attribute.c"
#include "anna_macro_conditional.c"
#include "anna_macro_operator.c"
#include "anna_macro_cast.c"

void anna_macro_init(anna_stack_frame_t *stack)
{
/*
    hash_init(&templatize_lookup,
	      &templatize_key_hash,
	      &templatize_key_compare);
*/
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
    anna_macro_add(stack, L"each", &anna_macro_iter);
    anna_macro_add(stack, L"map", &anna_macro_iter);
    anna_macro_add(stack, L"filter", &anna_macro_iter);
    anna_macro_add(stack, L"first", &anna_macro_iter);
    anna_macro_add(stack, L"__specialize__", &anna_macro_specialize);
    anna_macro_add(stack, L"__collection__", &anna_macro_collection);
    anna_macro_add(stack, L"type", &anna_macro_type);
    
/*    
    anna_macro_add(stack, L"while", &anna_macro_while);
    anna_macro_add(stack, L"return", &anna_macro_return);
    anna_macro_add(stack, L"__templateAttribute__", &anna_macro_template_attribute);
    anna_macro_add(stack, L"__extendsAttribute__", &anna_macro_extends_attribute);
    anna_macro_add(stack, L"__genericOperator__", &anna_macro_operator_wrapper);
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
}
