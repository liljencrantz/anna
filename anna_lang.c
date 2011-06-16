#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna.h"
#include "anna_util.h"
#include "anna_function.h"
#include "anna_stack.h"
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
#include "anna_type.h"
#include "anna_macro.h"
#include "anna_member.h"
#include "anna_node_wrapper.h"
#include "anna_intern.h"
#include "anna_lang.h"
#include "anna_vm.h"
#include "anna_mid.h"

static int hash_null_func( void *data )
{
    return 0;
}

static int hash_null_cmp( void *a, 
		   void *b )
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
}

void anna_lang_load(anna_stack_template_t *stack_lang)
{
    stack_lang->flags |= ANNA_STACK_NAMESPACE;
    
    /*
      Create lowest level stuff. Bits of magic, be careful with
      ordering here. A lot of intricate dependencies going on between
      the various calls. In other words...

      Here be dragons.
    */

    typedef struct 
    {
	anna_type_t **addr;
	wchar_t *name;
    }
    type_data_t;
    
    type_data_t type_data[] = 
	{
	    { &type_type,L"Type" },
	    { &object_type,L"Object" },
	    { &null_type,L"Null" },
	    { &int_type,L"Int" },
	    { &any_list_type,L"List" },
	    { &imutable_list_type,L"ImutableList" },
	    { &mutable_list_type, L"MutableList" },
	    { &string_type, L"String" },
	    { &float_type, L"Float" },
	    { &complex_type, L"Complex" },
	    { &char_type, L"Char" },
	    { &member_type, L"Member" },
	    { &range_type, L"Range" },
	    { &hash_type, L"HashMap" },
	    { &pair_type, L"Pair" },
	    { &node_call_wrapper_type, L"Call" },
	    { &node_wrapper_type, L"Node" },
	    { &node_identifier_wrapper_type, L"Identifier" },
	}
    ;
    	    
    int i;
    for(i=0; i<(sizeof(type_data)/sizeof(*type_data)); i++)
    {
	*(type_data[i].addr) = anna_type_native_create(type_data[i].name, stack_lang);
    }
    
    anna_object_type_create();
    anna_type_type_create(stack_lang);    
    anna_list_type_create(stack_lang);
    anna_type_type_create2(stack_lang);    
    anna_null_type_create();    
    anna_int_type_create(stack_lang);
    anna_string_type_create(stack_lang);
    anna_char_type_create(stack_lang);
    anna_float_type_create(stack_lang);
    anna_range_type_create(stack_lang);
    anna_complex_type_create(stack_lang);
    anna_pair_type_create();
    anna_hash_type_create(stack_lang);
    
    anna_type_t *types[] = 
	{
	    type_type, int_type, object_type, null_type,
	    mutable_list_type, any_list_type, imutable_list_type, string_type, 
	    float_type, complex_type, char_type, range_type, 
	    hash_type, pair_type
	};

    for(i=0; i<(sizeof(types)/sizeof(*types)); i++)
    {
	if((types[i] != object_type) && (types[i] != null_type))
	{
	    anna_type_copy_object(types[i]);
	}
	anna_stack_declare(
	    stack_lang, types[i]->name, 
	    type_type, anna_type_wrap(types[i]), ANNA_STACK_READONLY); 
    }

    anna_function_implementation_init(stack_lang);

    anna_stack_declare(
	stack_global,
	L"lang",
	anna_stack_wrap(stack_lang)->type,
	anna_stack_wrap(stack_lang),
	ANNA_STACK_READONLY);
}
