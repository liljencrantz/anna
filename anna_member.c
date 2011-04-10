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
#include "anna_function.h"
#include "common.h"
#include "anna_vm.h"

static anna_type_t *member_method_type, *member_property_type, *member_variable_type;

anna_object_t *anna_member_wrap(anna_type_t *type, anna_member_t *result)
{
    if(likely((long)result->wrapper))
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

static inline anna_object_t *anna_member_i_get_name_i(anna_object_t **param)
{
    anna_member_t *m = anna_member_unwrap(param[0]);
    return anna_string_create(wcslen(m->name), m->name);
}
ANNA_VM_NATIVE(anna_member_i_get_name, 1)

static inline anna_object_t *anna_member_i_get_static_i(anna_object_t **param)
{
    anna_member_t *m = anna_member_unwrap(param[0]);
    return m->is_static?anna_int_one:null_object;
}
ANNA_VM_NATIVE(anna_member_i_get_static, 1)

static inline anna_object_t *anna_member_i_get_method_i(anna_object_t **param)
{
    anna_member_t *m = anna_member_unwrap(param[0]);
    return m->is_method?anna_int_one:null_object;
}
ANNA_VM_NATIVE(anna_member_i_get_method, 1)

static inline anna_object_t *anna_member_i_get_property_i(anna_object_t **param)
{
    anna_member_t *m = anna_member_unwrap(param[0]);
    return m->is_property?anna_int_one:null_object;
}
ANNA_VM_NATIVE(anna_member_i_get_property, 1)

static void anna_member_type_create()
{

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

void anna_member_types_create(anna_stack_template_t *stack)
{
    anna_member_type_create();
    anna_member_method_type_create(stack);
    anna_member_property_type_create(stack);
    anna_member_variable_type_create(stack);    
}

mid_t anna_member_create(
    anna_type_t *type,
    mid_t mid,
    wchar_t *name,
    int storage,
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
	if(
	    mid == ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD ||
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
    member->is_static = !!(storage & ANNA_MEMBER_STATIC);
    if(storage & ANNA_MEMBER_VIRTUAL)
    {
	member->offset = -1;
    }
    else
    {
	if(storage & ANNA_MEMBER_STATIC) {
	    member->offset = anna_type_static_member_allocate(type);
	    type->static_member_blob[type->static_member_count-1] = (storage&ANNA_MEMBER_ALLOC)?ANNA_GC_ALLOC:(member_type == null_type);
	    type->static_member[type->static_member_count-1] = null_object;
	} else {
	    type->member_blob = realloc(
		type->member_blob, 
		sizeof(int)*(type->member_count+1));
	    type->member_blob[type->member_count] = (storage&ANNA_MEMBER_ALLOC)?ANNA_GC_ALLOC:(member_type == null_type);
	    
	    member->offset = type->member_count++;
	}
    }
    
    type->mid_identifier[mid] = member;
    hash_put(&type->name_identifier, member->name, member);
    
//    wprintf(L"Create member named %ls to type %ls\n", name, type->name);
    
    anna_stack_declare(
	type->stack,
	name,
	member_type,
	null_object,
	0);
    return mid;
}

anna_member_t *anna_member_get(anna_type_t *type, mid_t mid)
{
    return type->mid_identifier[mid];
}

anna_member_t *anna_member_method_search(
    anna_type_t *type,
    mid_t mid, 
    anna_node_call_t *call,
    int is_reverse)
{
    debug(D_SPAM, L"\nSEARCH for match to %ls in type %ls\n", anna_mid_get_reverse(mid), type->name);
    int i;
    wchar_t **members = calloc(sizeof(wchar_t *), hash_get_count(&type->name_identifier));
    wchar_t *alias_name = anna_mid_get_reverse(mid);
    anna_type_get_member_names(type, members);    
    wchar_t *match=0;
    int fault_count=0;

    for(i=0; i<hash_get_count(&type->name_identifier); i++)
    {
	debug(D_SPAM, L"Check %ls\n", members[i]);
	anna_member_t *member = anna_member_get(type, anna_mid_get(members[i]));
	if(member->is_static && member->offset>=0 && member->type != null_type)
	{
	    anna_object_t *mem_val = type->static_member[member->offset];
	    anna_function_t *mem_fun = anna_function_unwrap(mem_val);
	    if(!mem_fun)
	    {
		continue;
	    }
	    
	    anna_function_type_t *mem_fun_type = anna_function_unwrap_type(
		member->type);
	    
	    int has_alias = is_reverse ? anna_function_has_alias_reverse(mem_fun, alias_name):anna_function_has_alias(mem_fun, alias_name);
	

	    if(has_alias)
	    {
	    int j;
	    
	    if(mem_fun->input_count != call->child_count+1)
		continue;	    
	    //debug(D_SPAM, L"YAY, right number of arguments (%d)\n", argc);
	    
	    debug(D_SPAM, L"Check %ls against %ls\n",call->child[0]->return_type->name, mem_fun->input_type[1]->name);
	    int my_fault_count = 0;
	    int ok1 = anna_node_call_validate(call, mem_fun_type, 1, 0);
	    int ok2 = 1;
	    
	    if(ok1)
	    {
		anna_node_call_t *call_copy = (anna_node_call_t *)anna_node_clone_shallow(call);
		anna_node_call_map(call_copy, mem_fun_type, 1);
		
		for(j=0; j<call->child_count; j++)
		{
		    if(anna_abides(call_copy->child[j]->return_type, mem_fun->input_type[j+1]))
		    {
			my_fault_count += 
			    anna_abides_fault_count(mem_fun->input_type[j+1], call_copy->child[j]->return_type);
		    }
		    else
		    {
			ok2=0;
			debug(D_SPAM, L"Argument %d, %ls does not match %ls!\n", j, 
			      call_copy->child[j]->return_type->name, mem_fun->input_type[j+1]->name);
		    }
		
		}
	    }
	    
	    if(ok1 && ok2){
		debug(D_SPAM, L"Match!\n");
		
		if(!match || my_fault_count < fault_count)
		{
		    match = members[i];
		    fault_count = my_fault_count;
		}
	    }
	    }
	}
	else
	{
	    debug(D_SPAM, L"Not a function\n");
	}
	
    }

    if(match)
    {
	debug(D_SPAM, L"Match: %ls\n", match);
    }
    free(members);
    
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
	    argn);
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
	ANNA_MEMBER_VIRTUAL,
	property_type);
    anna_member_t *memb = anna_member_get(type, mid);
    
    memb->is_property=1;
    memb->getter_offset = getter_offset;
    memb->setter_offset = setter_offset;
    return mid;
}

/*
mid_t anna_const_property_create(
    anna_type_t *type, mid_t mid, wchar_t *name, anna_object_t *value)
{
    wchar_t *argn[] = 
	{
	    L"this"
	}
    ;
    anna_type_t *argv[] = 
	{
	    type
	}
    ;
    
    size_t getter_mid = -1;
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"!%lsGetter", name);
    
    getter_mid = anna_native_method_create(
	type,
	-1,
	sb_content(&sb),
	0,
	0,
	value->type,
	1,
	argv,
	argn
	);
    sb_destroy(&sb);
    
    anna_node_t *body_param[] = {
	(anna_node_t *)anna_node_create_dummy(0, value, 0)
    };
    
    anna_node_call_t *body = anna_node_create_block2(
	0,
	body_param);
    anna_function_t *fun = anna_function_unwrap(*anna_static_member_addr_get_mid(type, getter_mid));
    fun->body = body;
    fun->stack_template = anna_stack_create(0);
    anna_stack_declare(fun->stack_template, L"this", type, null_object, 0);
    
    anna_function_setup_body(fun);
    
    anna_member_t *gm = anna_member_get(type, getter_mid);
    size_t getter_offset = gm->offset;
    
    mid = anna_member_create(
	type,
	mid,
	name,
	1,
	value->type);
    anna_member_t *memb = anna_member_get(type, mid);
    
    memb->is_property=1;
    memb->getter_offset = getter_offset;
    memb->setter_offset = -1;
    return mid;

}
*/
