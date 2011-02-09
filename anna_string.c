#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_string.h"
#include "anna_int.h"
#include "anna_char.h"
#include "anna_type.h"
#include "anna_string_internal.h"

static inline anna_string_t *as_unwrap(anna_object_t *obj)
{
    return (anna_string_t *)anna_member_addr_get_mid(obj,ANNA_MID_STRING_PAYLOAD);
}

void anna_string_print(anna_object_t *obj)
{
    anna_string_t *str = as_unwrap(obj);
    asi_print_regular(str);
}

anna_object_t *anna_string_create(size_t sz, wchar_t *data)
{
    anna_object_t *obj= anna_object_create(string_type);
    //  wprintf(L"Create new string \"%.*ls\" at %d\n", sz, data, obj);
    
    asi_init_from_ptr(as_unwrap(obj),  data, sz);
    return obj;
}

size_t anna_string_get_count(anna_object_t *this)
{
    return asi_get_length(as_unwrap(this));
}

wchar_t *anna_string_payload(anna_object_t *obj)
{
//    wprintf(L"Get payload from string at %d\n", obj);
    anna_string_t *str = as_unwrap(obj);
    return asi_cstring(str);
}

size_t anna_string_count(anna_object_t *obj)
{
//    wprintf(L"Get payload from string at %d\n", obj);
    anna_string_t *str = as_unwrap(obj);
    return asi_get_length(str);
}


static anna_object_t *anna_string_i_set_int(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    wchar_t ch = anna_char_get(param[2]);
    asi_set_char(as_unwrap(param[0]), anna_int_get(param[1]), ch);
    return param[2];
}

static anna_object_t *anna_string_i_get_int(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    return anna_char_create(asi_get_char(as_unwrap(param[0]), anna_int_get(param[1])));
}

static anna_object_t *anna_string_i_get_range(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    
    anna_object_t *range = param[1];
    int from = anna_int_get(*anna_member_addr_get_mid(range, ANNA_MID_FROM));
    int to = anna_int_get(*anna_member_addr_get_mid(range, ANNA_MID_TO));
    int step = anna_int_get(*anna_member_addr_get_mid(range, ANNA_MID_STEP));
    
    assert(step=1);
    
    anna_object_t *res= anna_object_create(string_type);
    asi_init(as_unwrap(res));
    asi_append(as_unwrap(res), as_unwrap(param[0]), from, to-from);
    
    return res;
    
}

static anna_object_t *anna_string_i_set_range(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;

    if(param[2]==null_object)
	return null_object;

    anna_object_t *range = param[1];
    int from = anna_int_get(*anna_member_addr_get_mid(range, ANNA_MID_FROM));
    int to = anna_int_get(*anna_member_addr_get_mid(range, ANNA_MID_TO));
    int step = anna_int_get(*anna_member_addr_get_mid(range, ANNA_MID_STEP));
    
    assert(step=1);
    
    asi_replace(
	as_unwrap(param[0]), 
	as_unwrap(param[2]), 
	from,
	to-from,
	0,
	asi_get_length(as_unwrap(param[2])));
    
    return param[0];
}

static anna_object_t *anna_string_i_init(anna_object_t **param)
{
    asi_init(as_unwrap(param[0]));
    return param[0];
}

static anna_object_t *anna_string_i_get_count(anna_object_t **param)
{
    return anna_int_create(asi_get_length(as_unwrap(param[0])));
}

static anna_object_t *anna_string_i_set_count(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    int sz = anna_int_get(param[1]);
    asi_truncate(as_unwrap(param[0]), sz);
    return param[1];
}

static anna_object_t *anna_string_i_join(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;

    anna_string_t *str1 = as_unwrap(param[0]);
    anna_string_t *str2 = as_unwrap(param[1]);
    anna_object_t *obj= anna_object_create(string_type);

    asi_init(as_unwrap(obj));
    asi_append(as_unwrap(obj), str1, 0, asi_get_length(str1));
    asi_append(as_unwrap(obj), str2, 0, asi_get_length(str2));
    
    return obj;
}

static anna_object_t *anna_string_i_each(anna_object_t **param)
{
    anna_object_t *body_object;
    anna_object_t *result=null_object;
    anna_string_t *str = as_unwrap(param[0]);
    
    body_object=param[1];

    size_t sz = asi_get_length(str);
    size_t i;
    
    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_stack_frame_t *stack = stack_ptr?*stack_ptr:0;
    assert(function_ptr);
/*
  wprintf(L"each loop got function %ls\n", (*function_ptr)->name);
  wprintf(L"with param %ls\n", (*function_ptr)->input_name[0]);
*/  
    anna_object_t *o_param[2];
    for(i=0;i<sz;i++)
    {
	/*
	  wprintf(L"Run the following code:\n");
	  anna_node_print((*function_ptr)->body);
	  wprintf(L"\n");
	*/
	o_param[0] = anna_int_create(i);
	o_param[1] = anna_char_create(asi_get_char(str, i));
	result = anna_function_invoke_values(*function_ptr, 0, o_param, stack);
    }
    return result;
}

void anna_string_type_create(anna_stack_frame_t *stack)
{
    anna_member_create(
	string_type, ANNA_MID_STRING_PAYLOAD,  L"!stringPayload", 
	0, null_type);
    int i;
    string_buffer_t sb;
    sb_init(&sb);
    for(i=1; i<(((sizeof(anna_string_t)+1)/sizeof(anna_object_t *))+1);i++)
    {
	sb_clear(&sb);
	sb_printf(&sb, L"!stringPayload%d", i+1);
	anna_member_create(
	    string_type, anna_mid_get(sb_content(&sb)),  sb_content(&sb), 
	    0, null_type);
    }
    
    sb_destroy(&sb);

#if 0
    anna_node_t *i_argv[] = 
	{
	    (anna_node_t *)anna_node_create_identifier(0, L"String"),
	    (anna_node_t *)anna_node_create_identifier(0, L"Int"),
	    (anna_node_t *)anna_node_create_identifier(0, L"Char")
	}
    ;

    wchar_t *i_argn[] =
	{
	    L"this", L"index", L"value"
	}
    ;

    anna_node_t *range_param[] = 
	{
	    (anna_node_t *)anna_node_create_identifier(0, L"lang"),	    
	    (anna_node_t *)anna_node_create_identifier(0, L"Range"),	    
	};
    
    
    anna_node_t *range = (anna_node_t *)anna_node_create_call(
	0,
	(anna_node_t *)anna_node_create_identifier(0, L"__memberGet__"),
	2,
	range_param);
    
    anna_node_t *range_argv[] = 
	{
	    (anna_node_t *)anna_node_create_identifier(0, L"String"),
	    (anna_node_t *)range,
	    (anna_node_t *)anna_node_create_identifier(0, L"String"),
	}
    ;

    wchar_t *range_argn[] =
	{
	    L"this", L"range", L"value"
	}
    ;

    wchar_t *join_argn[] =
	{
	    L"this", L"other"
	}
    ;
    
    anna_node_t *join_argv[] = 
	{
	    (anna_node_t *)anna_node_create_identifier(0, L"String"),
	    (anna_node_t *)anna_node_create_identifier(0, L"String"),
	}
    ;

    anna_node_t *e_method_argv[] = 
	{
	    (anna_node_t *)anna_node_create_identifier(0, L"Int"),
	    (anna_node_t *)anna_node_create_identifier(0, L"Char")
	}
    ;

    wchar_t *e_method_argn[] = 
	{
	    L"index",
	    L"value"
	}
    ;

    anna_node_t *e_argv[] = 
	{
	    (anna_node_t *)anna_node_create_identifier(0, L"String"),
	    anna_node_create_function_declaration(0, (anna_node_t *)anna_node_create_identifier(0, L"Object"), 2, e_method_argv, e_method_argn)
	}
    ;

    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;
    
    string_type = anna_type_native_create(L"String", stack);
    anna_node_call_t *definition = anna_type_definition_get(string_type);
    
    anna_member_add_node(
	definition, ANNA_MID_STRING_PAYLOAD,  L"!stringPayload", 
	0, (anna_node_t *)anna_node_create_identifier(0, L"Null") );
    
    int i;
    string_buffer_t sb;
    sb_init(&sb);
    for(i=1; i<(((sizeof(anna_string_t)+1)/sizeof(anna_object_t *))+1);i++)
    {
	sb_clear(&sb);
	sb_printf(&sb, L"!stringPayload%d", i);
	
	anna_member_add_node(
	    definition, -1,  sb_content(&sb), 
	    0, (anna_node_t *)anna_node_create_identifier(0, L"Null") );	
    }
    
    sb_destroy(&sb);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__init__",
	ANNA_FUNCTION_VARIADIC, 
	(anna_native_t)&anna_string_i_init, 
	(anna_node_t *)anna_node_create_identifier(0, L"Null") , 
	1, i_argv, i_argn);    

    anna_native_method_add_node(
	definition,
	-1,
	L"__get__Int__",
	0, 
	(anna_native_t)&anna_string_i_get_int, 
	(anna_node_t *)anna_node_create_identifier(0, L"Char") , 
	2, 
	i_argv, 
	i_argn);

    anna_native_method_add_node(
	definition,
	-1,
	L"__get__Range__",
	0, 
	(anna_native_t)&anna_string_i_get_range, 
	(anna_node_t *)anna_node_create_identifier(0, L"String") , 
	2,
	range_argv, 
	range_argn);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__set__Range__",
	0, 
	(anna_native_t)&anna_string_i_set_range, 
	(anna_node_t *)anna_node_create_identifier(0, L"String") , 
	3,
	range_argv, 
	range_argn);
    
    anna_native_method_add_node(
	definition, 
	-1,
	L"__set__Int__", 
	0, 
	(anna_native_t)&anna_string_i_set_int, 
	(anna_node_t *)anna_node_create_identifier(0, L"Char"), 
	3,
	i_argv, 
	i_argn);
    
    anna_native_method_add_node(
	definition, 
	-1,
	L"__join__String__", 
	0, 
	(anna_native_t)&anna_string_i_join, 
	(anna_node_t *)anna_node_create_identifier(0, L"String"), 
	2,
	join_argv, 
	join_argn);
    
    anna_native_method_add_node(
	definition, -1, L"__each__", 0, 
	(anna_native_t)&anna_string_i_each, 
	(anna_node_t *)anna_node_create_identifier(0, L"Char"), 
	2, e_argv, e_argn);
  
    anna_native_method_add_node(
	definition, -1, L"getCount", 0, 
	(anna_native_t)&anna_string_i_get_count, 
	(anna_node_t *)anna_node_create_identifier(0, L"Int"), 
	1, i_argv, i_argn);
    
    anna_native_method_add_node(
	definition, -1, L"setCount", 0, 
	(anna_native_t)&anna_string_i_set_count, 
	(anna_node_t *)anna_node_create_identifier(0, L"Int"), 
	2, i_argv, i_argn);

    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_create_property(
	    0,
	    L"count",
	    (anna_node_t *)anna_node_create_identifier(0, L"Int") , 
	    L"getCount",
	    L"setCount"));
#endif	
}
