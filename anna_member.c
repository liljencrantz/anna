#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_member.h"
#include "anna_int.h"
#include "anna_char.h"
#include "anna_type.h"
#include "anna_string.h"

static anna_type_t *member_method_type, *member_property_type, *member_variable_type;

anna_object_t *anna_member_wrap(anna_type_t *type, anna_member_t *result)
{
    if(likely(result->wrapper))
	return result->wrapper;

    anna_type_t * m_type;
    if(result->is_method)
    {
	m_type = member_method_type;
    }
    else if(result->is_property)
    {
	m_type = member_property_type;
    }
    else
    {
	m_type=member_variable_type;
    }
    
    result->wrapper = anna_object_create(m_type);
    memcpy(anna_member_addr_get_mid(result->wrapper, ANNA_MID_MEMBER_PAYLOAD), &result, sizeof(anna_member_t *));  
    memcpy(anna_member_addr_get_mid(result->wrapper, ANNA_MID_MEMBER_TYPE_PAYLOAD), &type, sizeof(anna_type_t *));  
    return result->wrapper;
}

anna_member_t *anna_member_unwrap(anna_object_t *wrapper)
{
    return *(anna_member_t **)anna_member_addr_get_mid(wrapper, ANNA_MID_MEMBER_PAYLOAD);
}

static anna_object_t *anna_member_i_get_name(anna_object_t **param)
{
    anna_member_t *m = anna_member_unwrap(param[0]);
    return anna_string_create(wcslen(m->name), m->name);
}


static anna_object_t *anna_member_i_get_static(anna_object_t **param)
{
    anna_member_t *m = anna_member_unwrap(param[0]);
    return m->is_static?anna_int_one:null_object;
}

static anna_object_t *anna_member_i_get_method(anna_object_t **param)
{
    anna_member_t *m = anna_member_unwrap(param[0]);
    return m->is_method?anna_int_one:null_object;
}

static anna_object_t *anna_member_i_get_property(anna_object_t **param)
{
    anna_member_t *m = anna_member_unwrap(param[0]);
    return m->is_property?anna_int_one:null_object;
}

static void anna_member_type_create(anna_stack_frame_t *stack)
{
    anna_node_t *argv[] = 
	{
	    (anna_node_t *)anna_node_create_identifier(0, L"Member"),
	}
    ;

    wchar_t *argn[] =
	{
	    L"this"
	}
    ;
    
    member_type = 
	anna_type_native_create(
	    L"Member",
	    stack);
    
    anna_member_create(
	member_type, 
	ANNA_MID_MEMBER_PAYLOAD, 
	L"!memberPayload", 
	0,
	null_type);
    
    anna_member_create(
	member_type, 
	ANNA_MID_MEMBER_TYPE_PAYLOAD, 
	L"!memberTypePayload", 
	0,
	null_type);
    
    anna_native_property_create(
	member_type,
	-1,
	L"name",
	string_type,
	&anna_member_i_get_name, 
	0);
    
    anna_native_property_create(
	member_type,
	-1,
	L"isStatic",
	int_type,
	&anna_member_i_get_static,
	0);
    
    anna_native_property_create(
	member_type,
	-1,
	L"isMethod",
	int_type,
	&anna_member_i_get_method,
	0);
    
    anna_native_property_create(
	member_type,
	-1,
	L"isProperty",
	int_type,
	&anna_member_i_get_property,
	0);
}

#include "anna_member_method.c"
#include "anna_member_property.c"
#include "anna_member_variable.c"

void anna_member_types_create(anna_stack_frame_t *stack)
{
    anna_member_type_create(stack);
    anna_member_method_type_create(stack);
    anna_member_property_type_create(stack);
    anna_member_variable_type_create(stack);
    
}

size_t anna_member_create(
    anna_type_t *type,
    mid_t mid,
    wchar_t *name,
    int is_static,
    anna_type_t *member_type)
{
    if(!member_type)
    {
	wprintf(L"Critical: Create a member with unspecified type\n");
	CRASH;
    }
    //wprintf(L"Create member %ls in type %ls at mid %d\n", name, type->name, mid);
    
    if(hash_get(&type->name_identifier, name))
    {
	if(type == type_type && wcscmp(name, L"!typeWrapperPayload")==0)
	    return mid;
	if(mid == ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD ||
	   mid == ANNA_MID_FUNCTION_WRAPPER_PAYLOAD ||
	   mid == ANNA_MID_FUNCTION_WRAPPER_STACK ||
	   mid == ANNA_MID_STACK_TYPE_PAYLOAD)
	    return mid;
	
	wprintf(L"Critical: Redeclaring member %ls of type %ls\n",
		name, type->name);
	CRASH;	
    }
    
    anna_member_t * member = calloc(1,sizeof(anna_member_t) + sizeof(wchar_t) * (wcslen(name)+1));
    
    wcscpy(member->name, name);
    if (mid == -1) {
	mid = anna_mid_get(name);
    }
    else 
    {
	if(mid != anna_mid_get(name))
	{
	    wprintf(
		L"Critical: Multiple mids for name %ls: %d and %d\n", 
		name, mid, anna_mid_get(name));
	    CRASH;
	}
    }
    
    member->type = member_type;
    member->is_static = is_static;
    if(is_static) {
	member->offset = anna_type_static_member_allocate(type);
    } else {
	member->offset = type->member_count++;
    }
//  wprintf(L"Add member with mid %d\n", mid);
    
    type->mid_identifier[mid] = member;
    hash_put(&type->name_identifier, member->name, member);
    return mid;
}

anna_member_t *anna_member_get(anna_type_t *type, mid_t mid)
{
    return type->mid_identifier[mid];
}

anna_member_t *anna_member_method_search(
    anna_type_t *type,
    mid_t mid, 
    size_t argc, anna_type_t **argv)
{
//    wprintf(L"\nSEARCH for match to %ls\n", anna_mid_get_reverse(mid));
    int i;
    wchar_t **members = calloc(sizeof(wchar_t *), anna_type_member_count(type));
    wchar_t *prefix = anna_mid_get_reverse(mid);
    anna_type_get_member_names(type, members);    
    wchar_t *match=0;
    int fault_count=0;

    for(i=0; i<anna_type_member_count(type); i++)
    {
//	wprintf(L"Check %ls\n", members[i]);
	if(wcsncmp(prefix, members[i], wcslen(prefix)) != 0)
	    continue;
//	wprintf(L"%ls matches, name-wise\n", members[i]);
	
	anna_member_t *member = anna_member_get(type, anna_mid_get(members[i]));
	anna_type_t *mem_type = member->type;
//	wprintf(L"Is of type %ls\n", mem_type->name);
	anna_function_type_key_t *mem_fun = anna_function_unwrap_type(mem_type);
	if(mem_fun)
	{
//	    wprintf(L"YAY, it's a function (%d arguments)\n", mem_fun->argc);
	    int j;
	    
	    if(mem_fun->argc != argc+1)
		continue;	    
	    //wprintf(L"YAY, right number of arguments (%d)\n", argc);
	    
//	    wprintf(L"Check %ls against %ls\n",argv[0]->name, mem_fun->argv[1]->name);
	    int my_fault_count = 0;
	    int ok = 1;
	    
	    for(j=0; j<argc; j++)
	    {
		if(anna_abides(argv[j], mem_fun->argv[j+1]))
		{
		    my_fault_count += 
			anna_abides_fault_count(mem_fun->argv[j+1], argv[j]);
		}
		else
		{
		    ok=0;
		}
		
	    }
	    
	    if(ok){
		if(!match || my_fault_count < fault_count)
		{
		    match = members[i];
		    fault_count = my_fault_count;
		}
	    }
	}
	else
	{
	    wprintf(L"Not a function\n");
	}
	
    }
/*
    if(match)
    {
	wprintf(L"Match: %ls\n", match);
    }
*/  
    return match ? anna_member_get(type, anna_mid_get(match)):0;
    
}

size_t anna_native_property_create(
    anna_type_t *type,
    mid_t mid,
    wchar_t *name,
    anna_type_t *property_type,
    anna_native_function_t getter,
    anna_native_function_t setter)
{
    wchar_t *argn[] = 
	{
	    L"this", L"value"
	}
    ;
    anna_type_t *argv[] = 
	{
	    type,
	    property_type
	}
    ;

    size_t getter_mid = -1;
    size_t setter_mid = -1;
    ssize_t getter_offset=-1;
    ssize_t setter_offset=-1;
    string_buffer_t sb;
    sb_init(&sb);
    

    if(getter)
    {
	sb_printf(&sb, L"!%lsGetter", name);
	
	getter_mid = anna_native_method_create(
	    type,
	    -1,
	    sb_content(&sb),
	    0,
	    getter,
	    property_type,
	    1,
	    argv,
	    argn
	    );
	anna_member_t *gm = anna_member_get(type, getter_mid);
	getter_offset = gm->offset;
    }
    
    if(setter)
    {
	sb_clear(&sb);
	sb_printf(&sb, L"!%lsSetter", name);
	setter_mid = anna_native_method_create(
	    type,
	    -1,
	    sb_content(&sb),
	    0,
	    setter,
	    property_type,
	    2,
	    argv,
	    argn
	    );
	anna_member_t *sm = anna_member_get(type, setter_mid);
	setter_offset = sm->offset;
    }
    sb_destroy(&sb);
    
    mid = anna_member_create(
	type,
	mid,
	name,
	1,
	property_type);
    anna_member_t *memb = anna_member_get(type, mid);
    
    memb->is_property=1;
    memb->getter_offset = getter_offset;
    memb->setter_offset = setter_offset;
    return mid;
}


