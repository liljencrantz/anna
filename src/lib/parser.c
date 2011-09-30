#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/base.h"
#include "anna/node.h"
#include "anna/node_create.h"
#include "anna/lib/parser.h"
#include "anna/type.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/int.h"
#include "anna/member.h"
#include "anna/function.h"
#include "anna/lib/function_type.h"
#include "anna/vm.h"
#include "anna/intern.h"
#include "anna/stack.h"
#include "anna/misc.h"
#include "anna/node_hash.h"
#include "anna/mid.h"
#include "anna/type_data.h"
#include "anna/module.h"
#include "anna/lib/clib.h"

anna_type_t *node_type, *node_identifier_type, *node_call_type;
static anna_type_t *anna_node_type_mapping[ANNA_NODE_TYPE_COUNT];

anna_object_t *anna_node_wrap(anna_node_t *node)
{
    if(node->wrapper)
	return node->wrapper;
    
    anna_type_t *type= anna_node_type_mapping[node->node_type] ? anna_node_type_mapping[node->node_type] : node_type;

    anna_object_t *obj= anna_object_create(type);
    *(anna_node_t **)anna_entry_get_addr(obj,ANNA_MID_NODE_PAYLOAD)=node;
    node->wrapper = obj;
    return obj;
}

anna_node_t *anna_node_unwrap(anna_object_t *this)
{
    if(this == null_object)
	return 0;
    anna_node_t **resp = (anna_node_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD);
    return resp?*resp:0;
}

void anna_node_wrapper_add_method(anna_function_t *fun)
{
    int i;
    mid_t mid = anna_mid_get(fun->name);
    for(i=0; i<ANNA_NODE_TYPE_COUNT; i++)
    {
	anna_type_t *type = anna_node_type_mapping[i];

	if(type && !type->mid_identifier[mid])
	{
	    anna_member_create_method(type, anna_mid_get(fun->name), fun);
	}
    }

    anna_member_create_method(node_type, anna_mid_get(fun->name), fun);
    
}

ANNA_VM_NATIVE(anna_node_wrapper_i_replace, 3)
{
    if((param[1] == null_entry) || (param[2] == null_entry))
    {
	return null_entry;
    }
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *tree = anna_node_unwrap(this);
    anna_node_identifier_t *old = (anna_node_identifier_t *)anna_node_unwrap(anna_as_obj(param[1]));    
    anna_node_t *new = anna_node_unwrap(anna_as_obj(param[2]));
    if(!old || !new)
    {
        return null_entry;
    }
    anna_node_t *res = anna_node_replace(anna_node_clone_deep(tree), old, new);
    return anna_from_obj(anna_node_wrap(res));
}

ANNA_VM_NATIVE(anna_generate_identifier, 1)
{
    wchar_t *ss = L"";

    if(!anna_entry_null(param[0]))
    {
	ss = anna_string_payload(anna_as_obj(param[0]));
    }
    
    wchar_t *nam = anna_util_identifier_generate(ss, 0);
    
    if(!anna_entry_null(param[0]))
    {
	free(ss);
    }
    
    return anna_from_obj(anna_string_create(wcslen(nam), nam));
}

ANNA_VM_NATIVE(anna_parse_i, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    
    wchar_t *str = anna_string_payload(anna_as_obj(param[0]));
    anna_node_t *res = anna_parse_string(str);
    if(res)
    {
	return anna_from_obj(anna_node_wrap(res));
    }
    return null_entry;
}


ANNA_VM_NATIVE(anna_node_wrapper_i_error, 2)
{
    anna_node_t *this = anna_node_unwrap(anna_as_obj_fast(param[0]));
    wchar_t *msg;
    if(anna_entry_null(param[1]))
    {
	msg = L"Unknown error";
	anna_error(this, L"%ls", msg);
    }
    else
    {
	msg = anna_string_payload(anna_as_obj(param[1]));
	anna_error(this, L"%ls", msg);
	free(msg);
    }
    return param[0];
}

ANNA_VM_NATIVE(anna_node_wrapper_i_to_string, 1)
{
    anna_object_t *thiso = anna_as_obj_fast(param[0]);
    anna_node_t *this = anna_node_unwrap(thiso);
    wchar_t *str = anna_node_string(this);
    
    anna_object_t *res = anna_string_create(wcslen(str), str);
    free(str);
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_node_wrapper_cmp, 2)
{
    if(anna_entry_null(param[1]))
    {
	return null_entry;
    }

    anna_node_t *o = anna_node_unwrap(anna_as_obj(param[1])); 
    if(!o)
    {
	return null_entry;
    }
    
    return anna_from_int(anna_node_compare(
	anna_node_unwrap(anna_as_obj(param[0])),
	anna_node_unwrap(anna_as_obj(param[1])))); 
}

ANNA_VM_NATIVE(anna_node_wrapper_hash, 1)
{
    return anna_from_int(
	anna_node_hash_func(
	    anna_node_unwrap(
		anna_as_obj(param[0]))));
}

ANNA_VM_NATIVE(anna_node_wrapper_copy, 1)
{
    return anna_from_obj(
	anna_node_wrap(
	    anna_node_clone_deep(
		anna_node_unwrap(
		    anna_as_obj(
	  		param[0])))));
}

static void anna_node_basic_create_type(anna_stack_template_t *stack)
{
    anna_type_t *replace_argv[] = 
	{
	    node_type,
	    node_identifier_type,
	    node_type,
	}
    ;
    wchar_t *replace_argn[] =
	{
	    L"this", L"old", L"new"
	}
    ;
    
    anna_type_t *error_argv[] = 
	{
	    node_type,
	    string_type
	}
    ;
    wchar_t *error_argn[] =
	{
	    L"this", L"message"
	}
    ;
    
    anna_member_create(node_type, ANNA_MID_NODE_PAYLOAD, 0, null_type);
    
    anna_member_create_native_method(
	node_type, anna_mid_get(L"replace"), 0,
	&anna_node_wrapper_i_replace,
	node_type, 3, replace_argv, replace_argn, 0,
	L"Replace all instances of the specified identifier AST node within a copy of the original AST tree with the specified replacement AST node and return the nesulting new tree. The original tree is not modified.");

    anna_member_create_native_method(
	node_type, anna_mid_get(L"error"), 0,
	&anna_node_wrapper_i_error,
	node_type, 2, error_argv, error_argn, 0,
	L"Report a compiler error at the source code location of the specified node");

    anna_member_create_native_method(
	node_type, anna_mid_get(L"toString"),
	0,
	&anna_node_wrapper_i_to_string,
	string_type, 1, error_argv, error_argn, 0,
	L"Returns a string representation of the specified AST tree");
    
    anna_type_t *cmp_argv[] = 
	{
	    node_type, object_type
	}
    ;
    wchar_t *cmp_argn[]=
	{
	    L"this", L"other"
	}
    ;

    anna_member_create_native_method(
	node_type, anna_mid_get(L"__cmp__"), 0,
	&anna_node_wrapper_cmp, 
	int_type, 2, cmp_argv, cmp_argn, 0, 0);

    anna_member_create_native_method(
	node_type, anna_mid_get(L"hashCode"), 0,
	&anna_node_wrapper_hash,
	int_type, 1, cmp_argv, cmp_argn, 0, 0);

    anna_member_create_native_method(
	node_type, anna_mid_get(L"copy"), 0,
	&anna_node_wrapper_copy,
	node_type, 1, cmp_argv, cmp_argn, 0,
	L"Return an identical copy of the specified AST tree");
}

#include "src/lib/parser/call.c"
#include "src/lib/parser/identifier.c"
#include "src/lib/parser/int_literal.c"
#include "src/lib/parser/string_literal.c"
#include "src/lib/parser/char_literal.c"
#include "src/lib/parser/float_literal.c"
#include "src/lib/parser/null.c"
#include "src/lib/parser/dummy.c"
#include "src/lib/parser/closure.c"
#include "src/lib/parser/mapping.c"

void anna_parser_create_types(anna_stack_template_t *stack)
{
    static anna_type_data_t type_data[] = 
	{
	    { &node_call_type, L"Call" },
	    { &node_type, L"Node" },
	    { &node_identifier_type, L"Identifier" },
	}
    ;
    
    anna_type_data_create(type_data, stack);
}

void anna_parser_load(anna_stack_template_t *stack)
{
    int i;
    stack->flags |= ANNA_STACK_NAMESPACE;
        
    anna_type_t *mapping_id_type = anna_type_native_create(L"InternalIdentifier", stack);
    anna_node_basic_create_type(stack);
    anna_node_create_identifier_type(stack, node_identifier_type, 0);
    anna_node_create_identifier_type(stack, mapping_id_type, 1);
    
    anna_node_create_call_type(stack, node_call_type);

    anna_type_t *types[] = 
	{
	    node_call_type,
	    node_identifier_type,  
	    anna_node_create_int_literal_type(stack),
	    anna_node_create_string_literal_type(stack),
	    anna_node_create_char_literal_type(stack),
	    anna_node_create_float_literal_type(stack),
	    anna_node_create_null_type(stack),
	    anna_node_create_dummy_type(stack),
	    anna_node_create_closure_type(stack),
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    anna_node_create_mapping_type(stack),
	    mapping_id_type,
	    0
	};

    assert(sizeof(anna_node_type_mapping) >= sizeof(types));

    memset(anna_node_type_mapping, 0, sizeof(anna_node_type_mapping));
    memcpy(anna_node_type_mapping, types, sizeof(types));

    anna_type_copy_object(node_type);
    anna_stack_declare(
	stack, node_type->name, 
	type_type, anna_from_obj(anna_type_wrap(node_type)),
	ANNA_STACK_READONLY); 
    /*
      Insert all the cool stuff from node_wrapper and 
     */
    for(i=0; i<(sizeof(types)/sizeof(*types)); i++)
    {
	if(!types[i])
	    continue;
	anna_type_copy(types[i], node_type);
	anna_type_copy_object(types[i]);
	/* Declare all types in our namespace.  Don't redeclare types
	   that are used for more than one mapping */
	anna_type_close(types[i]);
	if(!anna_stack_template_get(stack, types[i]->name))
	{
	    anna_stack_declare(
		stack, types[i]->name, 
		type_type, anna_from_obj(anna_type_wrap(types[i])),
		ANNA_STACK_READONLY); 
	}
    }

    static wchar_t *i_argn[]={L"hint"};
    anna_module_function(
	stack, L"identifier", 0, anna_generate_identifier,
	string_type, 1, &string_type, i_argn, 
	L"Generate a unique identifier name, suitable for use as an internal identifier in autogenerated code");
    
    static wchar_t *p_argn[]={L"input"};
    anna_module_function(
	stack, L"parse", 0, anna_parse_i,
	node_type, 1, &string_type, p_argn,
	L"Parse the specified string and return an AST tree representing it");
    
}
