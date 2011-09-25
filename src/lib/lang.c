#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <float.h>
#include <wctype.h>

#include "anna/common.h"
#include "util.h"
#include "anna.h"
#include "anna/util.h"
#include "anna/function.h"
#include "anna/stack.h"
#include "anna/type.h"
#include "anna/macro.h"
#include "anna/member.h"
#include "anna/intern.h"
#include "anna/vm.h"
#include "anna/mid.h"
#include "anna/type_data.h"
#include "anna/vm_internal.h"
#include "anna/module.h"
#include "anna/node_create.h"

#include "anna/lib/lang/int.h"
#include "anna/lib/lang/float.h"
#include "anna/lib/lang/complex.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/lang/char.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/range.h"
#include "anna/lib/lang/hash.h"
#include "anna/lib/lang/pair.h"
#include "anna/lib/anna_function_type.h"
#include "anna/lib/lang/object.h"
#include "anna/lib/lang/buffer.h"
#include "anna/lib/parser.h"
#include "anna/lib/clib.h"
#include "anna/lib/lang/anna_string_internal.h"
#include "anna/tt.h"

#include "anna/lib/lang/buffer.c"
#include "anna/lib/lang/int.c"
#include "anna/lib/lang/float.c"
#include "anna/lib/lang/complex.c"
#include "anna/lib/lang/string.c"
#include "anna/lib/lang/char.c"
#include "anna/lib/lang/list.c"
#include "anna/lib/lang/range.c"
#include "anna/lib/lang/hash.c"
#include "anna/lib/lang/pair.c"
#include "anna/lib/lang/object.c"

anna_type_t *object_type=0,
    *int_type=0, 
    *null_type=0,
    *string_type=0, 
    *imutable_string_type=0, 
    *mutable_string_type=0, 
    *char_type=0,
    *float_type=0,
    *range_type=0,
    *complex_type=0,
    *hash_type=0,
    *pair_type=0,
    *buffer_type=0
    ;

anna_object_t *null_object=0;
anna_function_t *anna_lang_nothing;

const static anna_type_data_t anna_lang_type_data[] = 
{
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
		null_type, 1, argv, argn, 0, 0));
    null_type->static_member[0]= (anna_entry_t *)null_function;
    hash_init(&null_type->name_identifier, &hash_null_func, &hash_null_cmp);
    hash_put(&null_type->name_identifier, L"!null_member", null_member);
    
    for(i=0; i<anna_mid_get_count();i++) {
	null_type->mid_identifier[i] = null_member;
    }
    assert(anna_entry_get_static(null_type, 5) == (anna_entry_t *)null_function);    
    null_object->type = null_type;
}


anna_object_t *anna_wrap_method;

static int print_direct(anna_entry_t *o)
{
    if(anna_is_obj(o))
    {
	anna_object_t *o2 = anna_as_obj(o);
	if(o2->type == string_type)
	{
	    anna_string_print(o2);
	    return 1;
	}
    }
    else
    {
	if(anna_is_float(o))
	{
	    wchar_t buff[32];
	    
	    swprintf(buff, 32, L"%f", anna_as_float(o));
	    wchar_t *comma = wcschr(buff, ',');
	    if(comma)
	    {
		*comma = '.';
	    }
	    wprintf(L"%ls", buff);
	}
	else if(anna_is_char(o))
	{
	    wprintf(L"%lc", anna_as_char(o));
	}
	else if(anna_is_int_small(o))
	{
	    wprintf(L"%d", anna_as_int(o));
	}
	return 1;
    }
    
    return 0;
}

static int print_direct_loop(anna_object_t *list, int idx)
{
    int ls = anna_list_get_count(list);
    while(1)
    {
	if(ls == idx)
	{
	    break;
	}
	anna_entry_t *o = anna_list_get(list, idx);
	if(!print_direct(o))
	{
	    break;
	}
	idx++;	
    }
    return idx;
}

static void anna_print_callback(anna_context_t *stack)
{
    anna_object_t *value = anna_context_pop_object(stack);
    anna_entry_t **param = stack->top - 2;
    anna_object_t *list = anna_as_obj_fast(param[0]);
    int idx = anna_as_int(param[1]);
    int ls = anna_list_get_count(list);
    if(value == null_object) 
    {
	wprintf(L"null");
    }
    else 
    {
	if(value->type == imutable_string_type || value->type == mutable_string_type)
	{
	    anna_string_print(value);
	}	
	else
	{
	    wprintf(L"<invalid toString method>");
	}
    }    
    
    idx = print_direct_loop(list, idx);

    if(ls > idx)
    {
	anna_object_t *o = anna_as_obj(anna_list_get(list, idx));
	param[1] = anna_from_int(idx+1);
	anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	anna_object_t *meth = anna_as_obj_fast(o->type->static_member[tos_mem->offset]);
	anna_vm_callback_reset(stack, meth, 1, (anna_entry_t **)&o);
    }
    else
    {
	anna_context_drop(stack, 3);
	anna_context_push_object(stack, list);
    }
}

static void anna_i_print(anna_context_t *stack)
{
//    wprintf(L"whoa, print\n");

    anna_object_t *list = anna_context_pop_object(stack);
    anna_context_pop_object(stack);
    int idx = print_direct_loop(list, 0);
    if(anna_list_get_count(list) > idx)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_int(idx+1)
	    }
	;
	
	anna_object_t *o = anna_as_obj(anna_list_get(list, idx));
	anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	anna_object_t *meth = anna_as_obj_fast(o->type->static_member[tos_mem->offset]);
	anna_vm_callback_native(
	    stack,
	    anna_print_callback, 2, callback_param,
	    meth, 1, (anna_entry_t **)&o
	    );
    }
    else
    {
	anna_context_push_object(stack, list);
    }
}

ANNA_VM_NATIVE(anna_i_nothing, 1)
{
    anna_object_t *list = anna_as_obj(param[0]);
    size_t count = anna_list_get_count(list);
    if(count)
    {
	return anna_list_get(list, count-1);
    }
    return null_entry;
}

ANNA_VM_NATIVE(anna_i_not, 1)
{
    return anna_entry_null(param[0])?anna_from_int(1):null_entry;
}

static void anna_i_callcc_callback(anna_context_t *stack)
{
    anna_object_t *res = anna_context_pop_object(stack);
    anna_context_pop_object(stack);
    anna_context_push_object(stack, res);
}

static void anna_i_callcc(anna_context_t *stack)
{
    stack->frame = anna_frame_to_heap(stack->frame);
    
    anna_object_t *fun = anna_context_pop_object(stack);
    anna_context_pop_object(stack);
    anna_object_t *cont = anna_continuation_create(
	&stack->stack[0],
	stack->top - &stack->stack[0],
	stack->frame,
	1)->wrapper;
    
    anna_vm_callback_native(
	stack, &anna_i_callcc_callback, 0, 0, 
	fun, 1, (anna_entry_t **)&cont);    
}

static void anna_i_wrap_method(anna_context_t *stack)
{
    anna_entry_t *meth = anna_context_pop_entry(stack);
    anna_entry_t *obj = anna_context_pop_entry(stack);
    anna_context_pop_object(stack);
    
    anna_object_t *res = anna_method_bind(
	stack,
	anna_function_unwrap(anna_as_obj(meth)))->wrapper;
    *anna_entry_get_addr(res, ANNA_MID_THIS) = obj;
    *anna_entry_get_addr(res, ANNA_MID_METHOD) = meth;
    anna_context_push_object(stack, res);
}

void anna_lang_create_types(anna_stack_template_t *stack_lang)
{
    anna_type_data_create(anna_lang_type_data, stack_lang);    
}

void anna_lang_load(anna_stack_template_t *stack)
{
    anna_object_type_create();
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
    
    static wchar_t *p_argn[]={L"object"};

    anna_module_function(
	stack,
	L"print", 
	ANNA_FUNCTION_VARIADIC, 
	&anna_i_print, 
	imutable_list_type, 1, &object_type, 
	p_argn, 
	L"Print all the supplied arguments to standard output");

    anna_lang_nothing = anna_module_function(
	stack,
	L"nothing", 
	ANNA_FUNCTION_VARIADIC, 
	&anna_i_nothing, 
	imutable_list_type, 1, &object_type, 
	p_argn, 
	L"Returns the last argument in the parameter list, or null");
    
    anna_module_function(
	stack,
	L"__not__", 0, 
	&anna_i_not, 
	int_type, 
	1, &object_type, p_argn, 
	L"Negates the value. Returns 1 of the input is null, null otherwise.");

    anna_module_function(
	stack,
	L"callCC", 0, 
	&anna_i_callcc, 
	object_type, 
	1, &object_type, p_argn, 
	L"Call with current continuation.");
    
    static wchar_t *wrap_argn[]={L"object",L"method"};
    anna_type_t *wrap_argv[]={object_type, object_type};
    
    anna_function_t *wrap = anna_native_create(
	L"wrapMethod", 0,
	&anna_i_wrap_method,
	object_type,
	2,
	wrap_argv, wrap_argn, 0, stack);
    anna_wrap_method = wrap->wrapper;

    anna_type_data_register(anna_lang_type_data, stack);
}
