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
#include "anna_function_type.h"
#include "common.h"
#include "anna_vm.h"
#include "anna_mid.h"
#include "anna_type_data.h"

static anna_type_t *member_method_type, *member_property_type, *member_variable_type;

anna_object_t *anna_member_wrap(anna_type_t *type, anna_member_t *result)
{
    if(result->wrapper)
	return result->wrapper;
    
    anna_type_t * m_type;
    if(result->is_bound_method)
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
    memcpy(anna_entry_get_addr(result->wrapper, ANNA_MID_MEMBER_PAYLOAD), &result, sizeof(anna_member_t *));  
    memcpy(anna_entry_get_addr(result->wrapper, ANNA_MID_MEMBER_TYPE_PAYLOAD), &type, sizeof(anna_type_t *));  
    assert(result->wrapper);
    return result->wrapper;
}

anna_member_t *anna_member_unwrap(anna_object_t *wrapper)
{
    return *(anna_member_t **)anna_entry_get_addr(wrapper, ANNA_MID_MEMBER_PAYLOAD);
}

static anna_type_t *anna_member_of(anna_object_t *wrapper)
{
    return *(anna_type_t **)anna_entry_get_addr(wrapper, ANNA_MID_MEMBER_TYPE_PAYLOAD);
}

ANNA_NATIVE(anna_member_i_get_name, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_member_t *m = anna_member_unwrap(this);
    return anna_from_obj( anna_string_create(wcslen(m->name), m->name));
}

ANNA_NATIVE(anna_member_i_get_static, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_member_t *m = anna_member_unwrap(this);
    return m->is_static?anna_from_int(1):null_entry;
}

ANNA_NATIVE(anna_member_i_get_method, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_member_t *m = anna_member_unwrap(this);
    return m->is_bound_method?anna_from_int(1):null_entry;
}

ANNA_NATIVE(anna_member_i_get_property, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_member_t *m = anna_member_unwrap(this);
    return m->is_property?anna_from_int(1):null_entry;
}

ANNA_NATIVE(anna_member_i_get_constant, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_member_t *m = anna_member_unwrap(this);
    anna_type_t *type = anna_member_of(this);
    anna_stack_template_t *frame = type->stack;
    if(!anna_stack_get(frame, m->name))
    {
	return null_entry;
    }
    
    return anna_stack_get_flag(frame, m->name) & ANNA_STACK_READONLY ? anna_from_int(1): null_entry;
}

ANNA_NATIVE(anna_member_i_value, 2)
{
    anna_object_t *memb_obj = anna_as_obj_fast(param[0]);
    anna_object_t *obj = anna_as_obj(param[1]);
    anna_member_t *memb = anna_member_unwrap(memb_obj);
    anna_type_t *type = anna_member_of(memb_obj);
    if(memb->is_static)
    {
	return type->static_member[memb->offset];
    }
    else if(type != obj->type)
    {
	return null_entry;
    }
    else
    {
	return obj->member[memb->offset];	
    }
}

static void anna_member_type_create()
{

    anna_member_create(
	member_type, ANNA_MID_MEMBER_PAYLOAD, 0, null_type);
 
   anna_member_create(
	member_type,
	ANNA_MID_MEMBER_TYPE_PAYLOAD,
	0,
	null_type);
    
    anna_member_create_native_property(
	member_type, anna_mid_get(L"name"),
	string_type, &anna_member_i_get_name, 0);

    anna_member_create_native_property(
	member_type,
	anna_mid_get(L"isStatic"),
	int_type,
	&anna_member_i_get_static,
	0);

    anna_member_create_native_property(
	member_type,
	anna_mid_get(L"isMethod"),
	int_type,
	&anna_member_i_get_method,
	0);

    anna_member_create_native_property(
	member_type,
	anna_mid_get(L"isProperty"),
	int_type,
	&anna_member_i_get_property,
	0);

    anna_member_create_native_property(
	member_type,
	anna_mid_get(L"isConstant"),
	int_type,
	&anna_member_i_get_constant,
	0);
    
    anna_type_t *v_argv[] = 
	{
	    member_type,
	    object_type
	}
    ;

    wchar_t *v_argn[] =
	{
	    L"this", L"object"
	}
    ;
    
    anna_member_create_native_method(
	member_type, anna_mid_get(L"value"),
	0, &anna_member_i_value,
	object_type,
	2,
	v_argv,
	v_argn);
}

#include "anna_member_method.c"
#include "anna_member_property.c"
#include "anna_member_variable.c"

static anna_type_data_t anna_member_type_data[] = 
{
    { &member_type, L"Member" },
    { &member_method_type, L"Method" },
    { &member_property_type, L"Property" },
    { &member_variable_type, L"Variable" },
}
    ;

void anna_member_create_types(anna_stack_template_t *stack)
{    
    anna_type_data_create(anna_member_type_data, stack);
}

void anna_member_load(anna_stack_template_t *stack)
{
    anna_member_type_create();
    anna_member_method_type_create(stack);
    anna_member_property_type_create(stack);
    anna_member_variable_type_create(stack);    
    anna_type_data_register(anna_member_type_data, stack);
}

void anna_member_type_set(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *member_type)
{
    type->mid_identifier[mid]->type = member_type;
}

mid_t anna_member_create(
    anna_type_t *type,
    mid_t mid,
    int storage,
    anna_type_t *member_type)
{
/*
    if(!member_type)
    {
	wprintf(L"Critical: Create a member with unspecified type\n");
	CRASH;
    }
*/
    //wprintf(L"Create member %ls in type %ls at mid %d\n", name, type->name, mid);

    wchar_t *name = anna_mid_get_reverse(mid);
    
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
	
	if(type->flags & ANNA_TYPE_MEMBER_DECLARATION_IN_PROGRESS)
	{
	    return anna_mid_get(name);
	}
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
	    type->static_member[type->static_member_count-1] = null_entry;
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

        
    anna_type_calculate_size(type);

    if(!(storage & ANNA_MEMBER_VIRTUAL))
    {
	type->flags |= ANNA_TYPE_MEMBER_DECLARATION_IN_PROGRESS;
/*
	anna_stack_declare(
	    type->stack,
	    name,
	    member_type,
	    null_entry,
	    0);
*/
	type->flags &= ~ANNA_TYPE_MEMBER_DECLARATION_IN_PROGRESS;
    }
    
    return mid;
}

mid_t anna_member_create_blob(
    anna_type_t *type,
    mid_t mid,
    int storage,
    size_t sz)
{
    mid_t res = anna_member_create(
	type,
	mid,
	storage,
	null_type);
    
    wchar_t *name = anna_mid_get_reverse(mid);
    int i;
    string_buffer_t sb;
    sb_init(&sb);
    
//    wprintf( L"Allocate blob of size %d, uses %d slots\n", sz, (((sz-1)/sizeof(anna_entry_t *))+1));
    
    for(i=1; i<(((sz-1)/sizeof(anna_entry_t *))+1);i++)
    {
	sb_clear(&sb);
	sb_printf(&sb, L"%ls%d", name, i+1);
	anna_member_create(type, anna_mid_get(sb_content(&sb)),
                           storage & ANNA_MEMBER_STATIC, null_type);
    }
    sb_destroy(&sb);
    
    return res;
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
    debug(D_SPAM, L"SEARCH for match to %ls in type %ls\n", anna_mid_get_reverse(mid), type->name);
    int i;
    wchar_t **members = calloc(sizeof(wchar_t *), hash_get_count(&type->name_identifier));
    wchar_t *alias_name = anna_mid_get_reverse(mid);
    anna_type_get_member_names(type, members);    
    wchar_t *match=0;
    int fault_count=0;

    for(i=0; i<hash_get_count(&type->name_identifier); i++)
    {
	anna_member_t *member = anna_member_get(type, anna_mid_get(members[i]));
//	debug(D_ERROR, L"Check %ls %d %d %ls\n", members[i],
//	      member->is_static, member->offset, member->type->name);
	if(member->is_static && member->offset>=0 && member->type != null_type)
	{
	    anna_object_t *mem_val = anna_as_obj(type->static_member[member->offset]);
	    anna_function_t *mem_fun = anna_function_unwrap(mem_val);
	    
	    if(!mem_fun)
	    {
		continue;
	    }
	    
	    anna_function_type_t *mem_fun_type = anna_function_type_unwrap(
		member->type);
	    
	    int has_alias = is_reverse ? anna_function_has_alias_reverse(mem_fun, alias_name):anna_function_has_alias(mem_fun, alias_name);
	    has_alias |= (!is_reverse && wcscmp(members[i], alias_name)==0);
	    
	    if(has_alias)
	    {
		int j;
		int off = !!member->is_bound_method && !(call->access_type & ANNA_NODE_ACCESS_STATIC_MEMBER);
		
		if(mem_fun->input_count != call->child_count+off)
		    continue;	    
		//debug(D_SPAM, L"YAY, right number of arguments (%d)\n", argc);
		
		debug(D_SPAM, L"Check %ls against %ls\n",call->child[0]->return_type->name, mem_fun->input_type[off]->name);
		int my_fault_count = 0;
		int ok1 = anna_node_validate_call_parameters(
		    call, mem_fun_type, off, 0);
		int ok2 = 1;
		
		if(ok1)
		{
		    anna_node_call_t *call_copy = (anna_node_call_t *)anna_node_clone_shallow((anna_node_t *)call);
		    anna_node_call_map(call_copy, mem_fun_type, off);
		    
		    for(j=0; j<call->child_count; j++)
		    {
			if(anna_abides(call_copy->child[j]->return_type, mem_fun->input_type[j+off]))
			{
			    my_fault_count += 
				anna_abides_fault_count(mem_fun->input_type[j+off], call_copy->child[j]->return_type);
			}
			else
			{
			    ok2=0;
			    debug(D_SPAM, L"Argument %d, %ls does not match %ls!\n", j, 
				  call_copy->child[j]->return_type->name, mem_fun->input_type[j+off]->name);
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

size_t anna_member_create_property(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *property_type,
    ssize_t getter_offset,
    ssize_t setter_offset)
{
    mid = anna_member_create(
	type,
	mid,
	ANNA_MEMBER_VIRTUAL,
	property_type);
    anna_member_t *memb = anna_member_get(type, mid);
    
    memb->is_property=1;
    memb->getter_offset = getter_offset;
    memb->setter_offset = setter_offset;
    return mid;
}

size_t anna_member_create_native_property(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *property_type,
    anna_native_t getter,
    anna_native_t setter)
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

    wchar_t *name = anna_mid_get_reverse(mid);

    if(getter)
    {
	sb_printf(&sb, L"!%lsGetter", name);
	
	getter_mid = anna_member_create_native_method(
	    type,
	    anna_mid_get(sb_content(&sb)),
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
	setter_mid = anna_member_create_native_method(
	    type,
	    anna_mid_get(sb_content(&sb)),
	    0,
	    setter,
	    property_type,
	    2,
	    argv,
	    argn);
	anna_member_t *sm = anna_member_get(type, setter_mid);
	setter_offset = sm->offset;
    }
    sb_destroy(&sb);
    
    return anna_member_create_property(
	type, mid, property_type, 
	getter_offset, setter_offset);
}

mid_t anna_member_create_method(
    anna_type_t *type,
    mid_t mid,
    anna_function_t *method)
{
    wchar_t *name = anna_mid_get_reverse(mid);

    if(hash_get(&type->name_identifier, name))
    {
	mid = anna_mid_get(name);
    }
    else
    {
	mid = 
	    anna_member_create(
		type,
		mid,
		1,
		anna_function_wrap(method)->type);
    }
    
    anna_member_t *m = type->mid_identifier[mid];
    //debug(D_SPAM,L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    m->is_bound_method=1;
    type->static_member[m->offset] = 
	anna_from_obj(
	    anna_function_wrap(
		method));
    //wprintf(L"INSERTELISERT!!!!\n");
    
    return mid;
}

size_t anna_member_create_native_method(
    anna_type_t *type,
    mid_t mid,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn)
{
    wchar_t *name = anna_mid_get_reverse(mid);

    if(!flags) 
    {
	if(!result)
	{
	    CRASH;
	}
	
	if(argc) 
	{
	    assert(argv);
	    assert(argn);
	}
    }
    
    mid = anna_member_create(
	type,
	mid,
	1,
	anna_type_for_function(
	    result, 
	    argc, 
	    argv,
	    argn,
	    0,
	    flags));
    anna_member_t *m = type->mid_identifier[mid];
    //debug(D_SPAM,L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    m->is_bound_method=1;
    type->static_member[m->offset] = 
	anna_from_obj(
	    anna_function_wrap(
		anna_native_create(
		    name, flags, func, result, 
		    argc, argv, argn,
		    0)));
    return (size_t)mid;
}

size_t anna_member_create_native_type_method(
    anna_type_t *type,
    mid_t mid,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn)
{
    wchar_t *name = anna_mid_get_reverse(mid);
    if(!flags) 
    {
	if(!result)
	{
	    CRASH;
	}
	
	if(argc) 
	{
	    assert(argv);
	    assert(argn);
	}
    }
    
    mid = anna_member_create(
	type,
	mid,
	1,
	anna_type_for_function(
	    result, 
	    argc, 
	    argv,
	    argn,
	    0,
	    flags));
    anna_member_t *m = type->mid_identifier[mid];
    //debug(D_SPAM,L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    m->is_bound_method=0;
    type->static_member[m->offset] = 
	anna_from_obj(
	    anna_function_wrap(
		anna_native_create(
		    name, flags, func, result, 
		    argc, argv, argn,
		    0)));
    return (size_t)mid;
}

void anna_member_document(
    anna_type_t *type,
    mid_t mid,
    wchar_t *doc)
{
    anna_function_t *fun = anna_function_unwrap(
	anna_as_obj_fast(anna_entry_get_static(type, mid)));
    if(!fun)
    {
	CRASH;
    }
    anna_function_document(fun, doc);
    
}
