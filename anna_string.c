#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_string.h"
#include "anna_int.h"
#include "anna_char.h"
#include "anna_type.h"
#include "anna_string_internal.h"


static anna_string_t *as_unwrap(anna_object_t *obj)
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

    asi_init_from_ptr(as_unwrap(obj),  data, sz);
    return obj;
}

size_t anna_string_get_count(anna_object_t *this)
{
    return asi_get_length(as_unwrap(this));
}

static anna_object_t *anna_string_set_int(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    wchar_t ch = anna_char_get(param[2]);
    asi_set_char(as_unwrap(param[0]), anna_int_get(param[1]), ch);
    return param[2];
}

static anna_object_t *anna_string_get_int(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    return anna_char_create(asi_get_char(as_unwrap(param[0]), anna_int_get(param[1])));
}

static anna_object_t *anna_string_init(anna_object_t **param)
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


void anna_string_type_create(anna_stack_frame_t *stack)
{
    anna_node_t *i_argv[] = 
	{
	  (anna_node_t *)anna_node_identifier_create(0, L"String"),
	  (anna_node_t *)anna_node_identifier_create(0, L"Int"),
	  (anna_node_t *)anna_node_identifier_create(0, L"Char")
	}
    ;
    wchar_t *i_argn[]=
	{
	    L"this", L"index", L"value"
	}
    ;

    string_type = anna_type_native_create(L"String", stack);
    anna_node_call_t *definition = anna_type_definition_get(string_type);
    
    anna_member_add_node(
	definition, ANNA_MID_STRING_PAYLOAD,  L"!stringPayload", 
	0, (anna_node_t *)anna_node_identifier_create(0, L"Null") );
    
    int i;
    string_buffer_t sb;
    sb_init(&sb);
    for(i=1; i<(((sizeof(anna_string_t)+1)/sizeof(anna_object_t *))+1);i++)
    {
	sb_clear(&sb);
	sb_printf(&sb, L"!stringPayload%d", i);
	
	anna_member_add_node(
	    definition, -1,  sb_content(&sb), 
	    0, (anna_node_t *)anna_node_identifier_create(0, L"Null") );	
	//wprintf(L"LALALALALA %d\n", i);
    }
    
    sb_destroy(&sb);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__init__",
	ANNA_FUNCTION_VARIADIC, 
	(anna_native_t)&anna_string_init, 
	(anna_node_t *)anna_node_identifier_create(0, L"Null") , 
	1, i_argv, i_argn);    

    anna_native_method_add_node(
	definition,
	-1,
	L"__get__Int__",
	0, 
	(anna_native_t)&anna_string_get_int, 
	(anna_node_t *)anna_node_identifier_create(0, L"Char") , 
	2, 
	i_argv, 
	i_argn);
    
    anna_native_method_add_node(
	definition, 
	-1,
	L"__set__Int__", 
	0, 
	(anna_native_t)&anna_string_set_int, 
	(anna_node_t *)anna_node_identifier_create(0, L"Char"), 
	3,
	i_argv, 
	i_argn);
    
    anna_native_method_add_node(
	definition, -1, L"getCount", 0, 
	(anna_native_t)&anna_string_i_get_count, 
	(anna_node_t *)anna_node_identifier_create(0, L"Int"), 
	1, i_argv, i_argn);
    
    anna_native_method_add_node(
	definition, -1, L"setCount", 0, 
	(anna_native_t)&anna_string_i_set_count, 
	(anna_node_t *)anna_node_identifier_create(0, L"Int"), 
	2, i_argv, i_argn);

    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    -1,
	    L"count",
	    (anna_node_t *)anna_node_identifier_create(0, L"Int") , 
	    L"getCount",
	    L"setCount"));
	
    anna_type_native_setup(string_type, stack);
    anna_type_print(string_type);
}
