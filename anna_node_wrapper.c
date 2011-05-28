#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_node_wrapper.h"
#include "anna_type.h"
#include "anna_string.h"
#include "anna_list.h"
#include "anna_int.h"
#include "anna_member.h"
#include "anna_function.h"
#include "anna_function_type.h"
#include "anna_vm.h"
#include "anna_intern.h"
#include "anna_stack.h"
#include "anna_util.h"
#include "anna_node_hash.h"

anna_type_t *node_wrapper_type, *node_identifier_wrapper_type, *node_call_wrapper_type;

static anna_type_t *anna_node_type_mapping[ANNA_NODE_TYPE_COUNT];

anna_object_t *anna_node_wrap(anna_node_t *node)
{
    if(node->wrapper)
	return node->wrapper;
    
    anna_type_t *type= anna_node_type_mapping[node->node_type] ? anna_node_type_mapping[node->node_type] : node_wrapper_type;

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
	    anna_member_create_method(
		type,
		-1,
		fun->name,
		fun);
	}
    }
    
}

static inline anna_entry_t *anna_node_wrapper_i_replace_i(anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *tree = anna_node_unwrap(this);
    anna_node_identifier_t *old = (anna_node_identifier_t *)anna_node_unwrap(anna_as_obj(param[1]));
    anna_node_t *new = anna_node_unwrap(anna_as_obj(param[2]));
    anna_node_t *res = anna_node_replace(anna_node_clone_deep(tree), old, new);
    return anna_from_obj(anna_node_wrap(res));
}
ANNA_VM_NATIVE(anna_node_wrapper_i_replace, 3)

static inline anna_entry_t *anna_generate_identifier_i(anna_entry_t **param)
{
    wchar_t *nam = anna_util_identifier_generate("", 0);
    return anna_from_obj(anna_string_create(wcslen(nam), nam));
}
ANNA_VM_NATIVE(anna_generate_identifier, 1)




static inline anna_entry_t *anna_node_wrapper_i_error_i(anna_entry_t **param)
{
    anna_node_t *this = anna_node_unwrap(anna_as_obj_fast(param[0]));
    wchar_t *msg;
    if(ANNA_VM_NULL(param[1]))
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
ANNA_VM_NATIVE(anna_node_wrapper_i_error, 2)

static inline anna_entry_t *anna_node_wrapper_i_to_string_i(anna_entry_t **param)
{
    anna_object_t *thiso = anna_as_obj_fast(param[0]);
    anna_node_t *this = anna_node_unwrap(thiso);
    wchar_t *str = anna_node_string(this);
    
    anna_object_t *res = anna_string_create(wcslen(str), str);
    free(str);
    return anna_from_obj(res);
}
ANNA_VM_NATIVE(anna_node_wrapper_i_to_string, 1)

static inline anna_entry_t *anna_node_wrapper_cmp_i(anna_entry_t **param)
{
    if(ANNA_VM_NULL(param[1]))
    {
	return anna_from_obj(null_object);
    }

    anna_node_t *o = anna_node_unwrap(anna_as_obj(param[1])); 
    if(!o)
    {
	return anna_from_obj(null_object);
    }
    
    return anna_from_int(anna_node_compare(
	anna_node_unwrap(anna_as_obj(param[0])),
	anna_node_unwrap(anna_as_obj(param[1])))); 
}
ANNA_VM_NATIVE(anna_node_wrapper_cmp, 2)

static inline anna_entry_t *anna_node_wrapper_hash_i(anna_entry_t **param)
{
    return anna_from_int(anna_node_hash_func(anna_node_unwrap(anna_as_obj(param[0]))));
}
ANNA_VM_NATIVE(anna_node_wrapper_hash, 1)


static inline anna_entry_t *anna_node_wrapper_copy_i(anna_entry_t **param)
{
    return anna_from_obj(
	anna_node_wrap(
	    anna_node_clone_deep(
		anna_node_unwrap(
		    anna_as_obj(
			param[0])))));
}
ANNA_VM_NATIVE(anna_node_wrapper_copy, 1)


static void anna_node_create_wrapper_type(anna_stack_template_t *stack)
{


    anna_type_t *replace_argv[] = 
	{
	    node_wrapper_type,
	    node_identifier_wrapper_type,
	    node_wrapper_type,
	}
    ;
    wchar_t *replace_argn[] =
	{
	    L"this", L"old", L"new"
	}
    ;
    
    anna_type_t *error_argv[] = 
	{
	    node_wrapper_type,
	    string_type
	}
    ;
    wchar_t *error_argn[] =
	{
	    L"this", L"message"
	}
    ;
    
    anna_member_create(
	node_wrapper_type, ANNA_MID_NODE_PAYLOAD,  L"!nodePayload", 
	0, null_type);
    
    anna_native_method_create(
	node_wrapper_type, -1, L"replace", 0, 
	&anna_node_wrapper_i_replace, 
	node_wrapper_type,
	3, replace_argv, replace_argn);
 
    anna_native_method_create(
	node_wrapper_type, -1, L"error", 0, 
	&anna_node_wrapper_i_error, 
	node_wrapper_type,
	2, error_argv, error_argn);
    
    anna_native_method_create(
	node_wrapper_type, -1, L"toString", 0, 
	&anna_node_wrapper_i_to_string, 
	string_type,
	1, error_argv, error_argn);    

    anna_type_t *cmp_argv[] = 
	{
	    node_wrapper_type, object_type
	}
    ;
    wchar_t *cmp_argn[]=
	{
	    L"this", L"other"
	}
    ;

    anna_native_method_create(
	node_wrapper_type,
	-1,
	L"__cmp__",
	0,
	&anna_node_wrapper_cmp, 
	int_type,
	2, cmp_argv, cmp_argn);

    anna_native_method_create(
	node_wrapper_type,
	-1,
	L"hashCode",
	0,
	&anna_node_wrapper_hash, 
	int_type,
	1, cmp_argv, cmp_argn);

    anna_native_method_create(
	node_wrapper_type,
	-1,
	L"copy",
	0,
	&anna_node_wrapper_copy, 
	node_wrapper_type,
	1, cmp_argv, cmp_argn);
}

#include "anna_node_call_wrapper.c"
#include "anna_node_identifier_wrapper.c"
#include "anna_node_int_literal_wrapper.c"
#include "anna_node_string_literal_wrapper.c"
#include "anna_node_char_literal_wrapper.c"
#include "anna_node_float_literal_wrapper.c"
#include "anna_node_null_wrapper.c"
#include "anna_node_dummy_wrapper.c"
#include "anna_node_closure_wrapper.c"
#include "anna_node_member_wrapper.c"
#include "anna_node_declare_wrapper.c"
#include "anna_node_cast_wrapper.c"
#include "anna_node_mapping_wrapper.c"

anna_stack_template_t *anna_node_create_wrapper_types()
{
    int i;
    anna_stack_template_t *stack = anna_stack_create(stack_global);
    stack->flags |= ANNA_STACK_NAMESPACE;
    
    node_wrapper_type = anna_type_native_create(L"Node", stack);
    node_identifier_wrapper_type = anna_type_native_create(L"Identifier", stack);
    anna_type_t *mapping_id_type = anna_type_native_create(L"MappingIdentifier", stack);
    anna_node_create_wrapper_type(stack);
    anna_node_create_identifier_wrapper_type(stack, node_identifier_wrapper_type, 0);
    anna_node_create_identifier_wrapper_type(stack, mapping_id_type, 1);

    node_call_wrapper_type = anna_node_create_call_wrapper_type(stack);
    anna_type_t *decl_type = anna_node_create_declare_wrapper_type(stack);

    anna_type_t *types[] = 
	{
	    node_call_wrapper_type,
	    node_identifier_wrapper_type,  
	    anna_node_create_int_literal_wrapper_type(stack),
	    anna_node_create_string_literal_wrapper_type(stack),
	    anna_node_create_char_literal_wrapper_type(stack),
	    anna_node_create_float_literal_wrapper_type(stack),
	    anna_node_create_null_wrapper_type(stack),
	    anna_node_create_dummy_wrapper_type(stack),
	    anna_node_create_closure_wrapper_type(stack),
	    0,
	    anna_node_create_member_wrapper_type(stack, ANNA_NODE_MEMBER_GET),
	    0,
	    anna_node_create_member_wrapper_type(stack, ANNA_NODE_MEMBER_SET),
	    0,
	    decl_type,
	    decl_type,
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
	    anna_node_create_cast_wrapper_type(stack),
	    anna_node_create_mapping_wrapper_type(stack),
	    mapping_id_type,
	    0
	};

    assert(sizeof(anna_node_type_mapping) >= sizeof(types));

    memset(anna_node_type_mapping, 0, sizeof(anna_node_type_mapping));
    memcpy(anna_node_type_mapping, types, sizeof(types));

    anna_type_copy_object(node_wrapper_type);
    anna_stack_declare(
	stack, node_wrapper_type->name, 
	type_type, anna_type_wrap(node_wrapper_type), ANNA_STACK_READONLY); 
    /*
      Insert all the cool stuff from node_wrapper and 
     */
    for(i=0; i<(sizeof(types)/sizeof(*types)); i++)
    {
	if(!types[i])
	    continue;
	anna_type_copy(types[i], node_wrapper_type);
	anna_type_copy_object(types[i]);
	/* Declare all types in our namespace.  Don't redeclare types
	   that are used for more than one mapping */
	if(!anna_stack_template_get(stack, types[i]->name))
	{
	    anna_stack_declare(
		stack, types[i]->name, 
		type_type, anna_type_wrap(types[i]), ANNA_STACK_READONLY); 
	}
    }


    static wchar_t *p_argn[]={L"hint"};
    anna_function_t *f = anna_native_create(
	L"identifier", 
	0,
	&anna_generate_identifier, 
	string_type, 1, &string_type, 
	p_argn, stack);

    anna_stack_declare(
	stack,
	L"identifier",
	f->wrapper->type,
	f->wrapper,
	ANNA_STACK_READONLY);
    

    
    
    anna_stack_declare(
	stack_global,
	L"parser",
	anna_stack_wrap(stack)->type,
	anna_stack_wrap(stack),
	ANNA_STACK_READONLY);

    return stack;
}
