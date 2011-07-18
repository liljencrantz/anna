#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "../common.h"
#include "../util.h"
#include "../anna.h"
#include "../anna_util.h"
#include "../anna_function.h"
#include "../anna_stack.h"
#include "../anna_type.h"
#include "../anna_macro.h"
#include "../anna_member.h"
#include "../anna_intern.h"
#include "../anna_vm.h"
#include "../anna_mid.h"
#include "../anna_type_data.h"

#include "anna_int.h"
#include "anna_float.h"
#include "anna_complex.h"
#include "anna_string.h"
#include "anna_char.h"
#include "anna_list.h"
#include "anna_range.h"
#include "anna_hash.h"
#include "anna_pair.h"
#include "anna_function_type.h"
#include "anna_type_type.h"
#include "anna_object_type.h"
#include "anna_buffer.h"
#include "anna_node_wrapper.h"
#include "anna_lang.h"

anna_type_t *type_type=0, 
    *object_type=0,
    *int_type=0, 
    *null_type=0,
    *string_type=0, 
    *imutable_string_type=0, 
    *mutable_string_type=0, 
    *char_type=0,
    *float_type=0,
    *member_type=0,
    *range_type=0,
    *complex_type=0,
    *hash_type=0,
    *pair_type=0,
    *buffer_type=0
    ;

anna_object_t *null_object=0;

const static anna_type_data_t anna_lang_type_data[] = 
{
    { &type_type,L"Type" },
    { &object_type,L"Object" },
    { &null_type,L"Null" },
    { &int_type,L"Int" },
    { &any_list_type,L"List" },
    { &imutable_list_type,L"ImutableList" },
    { &mutable_list_type, L"MutableList" },
    { &string_type, L"String" },
    { &imutable_string_type, L"ImutableString" },
    { &mutable_string_type, L"MutableString" },
    { &float_type, L"Float" },
    { &complex_type, L"Complex" },
    { &char_type, L"Char" },
    { &range_type, L"Range" },
    { &hash_type, L"HashMap" },
    { &pair_type, L"Pair" },
    { &buffer_type, L"Buffer" },
};

static int hash_null_func( void *data )
{
    return 0;
}

static int hash_null_cmp( void *a, void *b )
{
    return 1;
}

static void anna_null_type_create()
{
    int i;
    wchar_t *member_name = L"!null_member";
    anna_member_t *null_member;
    null_member = calloc(1,sizeof(anna_member_t)+(sizeof(wchar_t*)*(1+wcslen(member_name))));
    null_member->type = null_type;
    null_member->offset=0;
    null_member->is_static=1;
    null_member->is_property=1;
    wcscpy(null_member->name, member_name);
    
    anna_type_t *argv[]={null_type};
    wchar_t *argn[]={L"this"};
    anna_type_static_member_allocate(null_type);
    
    //null_type->static_member[0] = null_object;
    
    anna_object_t *null_function = 
	anna_function_wrap(
	    anna_native_create(
		L"!nullFunction", 0, 
		&anna_vm_null_function, 
		null_type, 1, argv, argn,
		0));
    null_type->static_member[0]= (anna_entry_t *)null_function;
    hash_init(&null_type->name_identifier, &hash_null_func, &hash_null_cmp);
    hash_put(&null_type->name_identifier, L"!null_member", null_member);
    
    for(i=0; i<anna_mid_get_count();i++) {
	null_type->mid_identifier[i] = null_member;
    }
    assert(anna_entry_get_static(null_type, 5) == (anna_entry_t *)null_function);    
    null_object->type = null_type;
}

void anna_lang_create_types(anna_stack_template_t *stack_lang)
{
    anna_type_data_create(anna_lang_type_data, stack_lang);    
}

void anna_lang_load(anna_stack_template_t *stack_lang)
{
    anna_object_type_create();
    anna_type_type_create();    
    anna_list_type_create();
    anna_null_type_create();    
    anna_int_type_create();
    anna_string_type_create();
    anna_char_type_create();
    anna_float_type_create();
    anna_range_type_create();
    anna_complex_type_create();
    anna_pair_type_create();
    anna_hash_type_create();
    anna_buffer_type_create();
    
    anna_function_implementation_init(stack_lang);
    anna_type_data_register(anna_lang_type_data, stack_lang);    
}
