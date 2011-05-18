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

anna_stack_template_t *anna_lang_load()
{

    anna_stack_template_t *stack_lang = anna_stack_create(stack_global);
    stack_lang->flags |= ANNA_STACK_NAMESPACE;
    
    /*
      Create lowest level stuff. Bits of magic, be careful with
      ordering here. A lot of intricate dependencies going on between
      the various calls. In other words...

      Here be dragons.
    */
    
    type_type = anna_type_native_create(L"Type", stack_lang);
    object_type = anna_type_native_create(L"Object" ,stack_lang);
    null_type = anna_type_native_create(L"Null", stack_lang);
    int_type =anna_type_native_create(L"Int", stack_lang);
    list_type =anna_type_native_create(L"List", stack_lang);
    string_type = anna_type_native_create(L"String", stack_lang);
    float_type = anna_type_native_create(L"Float", stack_lang);
    complex_type = anna_type_native_create(L"Complex", stack_lang);
    char_type = anna_type_native_create(L"Char", stack_lang);
    member_type = anna_type_native_create(L"Member",stack_lang);
    range_type = anna_type_native_create(L"Range", stack_lang);
    hash_type = anna_type_native_create(L"HashMap", stack_lang);
    pair_type = anna_type_native_create(L"Pair",stack_lang);
    
    anna_object_type_create();
    anna_type_type_create(stack_lang);    
    anna_list_type_create(stack_lang);
    anna_type_type_create2(stack_lang);    
    anna_null_type_create();    
    anna_int_type_create(stack_lang);
    anna_string_type_create(stack_lang);
    anna_member_types_create(stack_lang);
    anna_char_type_create(stack_lang);
    anna_float_type_create(stack_lang);
    anna_range_type_create(stack_lang);
    anna_complex_type_create(stack_lang);
    anna_pair_type_create();
    anna_hash_type_create(stack_lang);
    
    int i;
    anna_type_t *types[] = 
	{
	    type_type, int_type, object_type, null_type,
	    list_type, string_type, float_type, complex_type, 
	    char_type, range_type, hash_type, pair_type
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

    anna_stack_populate_wrapper(stack_lang);
    anna_stack_declare(
	stack_global,
	L"lang",
	anna_stack_wrap(stack_lang)->type,
	anna_stack_wrap(stack_lang),
	ANNA_STACK_READONLY);
    
    return stack_lang;
}
