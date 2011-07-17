#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_type.h"
#include "anna_type_type.h"
#include "anna_string.h"
#include "anna_list.h"
#include "anna_member.h"
#include "anna_node_create.h"
#include "anna_node_wrapper.h"
#include "anna_vm.h"
#include "anna_int.h"
#include "anna_mid.h"

ANNA_VM_NATIVE(anna_type_to_string, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *type = anna_type_unwrap(this);
    return anna_from_obj(anna_string_create(wcslen(type->name), type->name));
}

ANNA_VM_NATIVE(anna_type_i_get_member, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *lst = anna_list_create_imutable(member_type);
    int i;
    anna_type_t *type = anna_type_unwrap(this);

    wchar_t **member_name = malloc(sizeof(wchar_t *)*hash_get_count(&type->name_identifier));
    anna_type_get_member_names(type, member_name);
    for(i=0;i<hash_get_count(&type->name_identifier); i++)
    {
	anna_list_add(
	    lst,
	    anna_from_obj(anna_member_wrap(
		type,
		anna_type_member_info_get(
		    type,
		    member_name[i]))));	
    }
    
    return anna_from_obj(lst);
}

static anna_vmstack_t *anna_type_cmp(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *type1 = anna_type_unwrap(this);
    anna_type_t *type2 = anna_type_unwrap(anna_as_obj(param[1]));
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_object(stack, anna_int_create(type1-type2));
    return stack;
}

static anna_vmstack_t *anna_type_abides(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *type1 = anna_type_unwrap(this);
    anna_type_t *type2 = anna_type_unwrap(anna_as_obj(param[1]));
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_entry(stack, anna_abides(type1, type2)?anna_from_int(1):null_entry);
    return stack;
}

static anna_vmstack_t *anna_type_hash(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_vmstack_drop(stack, 2);
    anna_type_t *type = anna_type_unwrap(this);
    anna_vmstack_push_object(stack, anna_int_create(hash_ptr_func(type)));
    return stack;
}

ANNA_VM_NATIVE(anna_type_i_get_attribute, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *f = anna_type_unwrap(this);
    return f->attribute ? anna_from_obj(anna_node_wrap((anna_node_t *)f->attribute)): null_entry;
}

void anna_type_type_create()
{
    anna_member_create(type_type, ANNA_MID_TYPE_WRAPPER_PAYLOAD, 0, null_type);

    anna_type_t *argv[] = 
	{
	    type_type, type_type
	}
    ;
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;

    anna_type_document(
	type_type,
	L"The Type type represents an Anna Type. It is mostly used for object introspection.");
    
    anna_member_create_native_property(
	type_type, anna_mid_get(L"__name__"),
	string_type, &anna_type_to_string, 0, L"The name of this type.");
    anna_member_create_native_property(
	type_type,
	anna_mid_get(L"__member__"),
	anna_list_type_get_imutable(member_type),
	&anna_type_i_get_member,
	0, L"A list of all members of this type.");
    
    anna_member_create_native_method(
	type_type, ANNA_MID_HASH_CODE, 0,
	&anna_type_hash, int_type, 1, argv, argn);
    
    anna_member_create_native_method(
	type_type,
	ANNA_MID_CMP,
	0,
	&anna_type_cmp,
	int_type,
	2,
	argv,
	argn);
    anna_member_create_native_method(
	type_type,
	ANNA_MID_TO_STRING,
	0,
	&anna_type_to_string,
	string_type,
	1,
	argv,
	argn);    
    
    anna_member_create_native_method(
	type_type, anna_mid_get(L"__abides__"), 0,
	&anna_type_abides, object_type, 2, argv,
	argn);   

    anna_member_create_native_property(
	type_type, anna_mid_get(L"__attribute__"),
	node_call_wrapper_type,
	&anna_type_i_get_attribute,
	0,
	L"All attributes specified for this type.");


}