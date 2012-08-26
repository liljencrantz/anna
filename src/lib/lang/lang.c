#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <float.h>
#include <wctype.h>
#include <limits.h>
#include <time.h>

#include "anna/fallback.h"
#include "anna/common.h"
#include "anna/util.h"
#include "anna/base.h"
#include "anna/misc.h"
#include "anna/function.h"
#include "anna/function_type.h"
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
#include "anna/object.h"

#include "anna/lib/lang/int.h"
#include "anna/lib/lang/float.h"
#include "anna/lib/lang/complex.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/lang/char.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/range.h"
#include "anna/lib/lang/hash.h"
#include "anna/lib/lang/pair.h"
#include "anna/lib/lang/object.h"
#include "anna/lib/lang/buffer.h"
#include "anna/lib/parser.h"
#include "anna/lib/clib.h"
#include "anna/lib/lang/string_internal.h"
#include "anna/tt.h"

#include "src/lib/lang/buffer.c"
#include "src/lib/lang/int.c"
#include "src/lib/lang/float.c"
#include "src/lib/lang/complex.c"
#include "src/lib/lang/string.c"
#include "src/lib/lang/char.c"
#include "src/lib/lang/list.c"
#include "src/lib/lang/range.c"
#include "src/lib/lang/hash.c"
#include "src/lib/lang/pair.c"
#include "src/lib/lang/object.c"

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
    *hash_key_type=0,
    *pair_type=0,
    *buffer_type=0,
    *sendable_type=0
    ;

anna_object_t *null_object=0;
anna_member_t *null_member;

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
    { &hash_key_type, L"HashKey" },
    { &pair_type, L"Pair" },
    { &buffer_type, L"Buffer" },
    { &sendable_type, L"Sendable" },
};

static void anna_null_type_create()
{
    int i;
    null_member = calloc(1,sizeof(anna_member_t));
    null_member->type = null_type;
    null_member->offset=0;
    anna_member_set_static(null_member,1);
    anna_member_set_property(null_member,1);
    null_member->name = anna_intern_static(L"!null_member");
    
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
    null_type->static_member[0]= anna_from_ptr(null_function);

    for(i=0; i<null_type->mid_count;i++) {
	null_type->mid_identifier[i] = null_member;
    }

    null_object->type = null_type;

    anna_type_document(
	null_type,
	L"The Null type is the type of the null object. There is only one instance of the null object, and it can be accessed through the <code class='anna-code'>?</code> literal.");  

    anna_type_document(
	null_type,
	L"The null object is a special object. It can be cast to any other type. All members hold the value null. Calling a null value as a function will return null. The null object can be cast as any Object.");  

}


anna_object_t *anna_wrap_method;

ANNA_VM_NATIVE(anna_i_print_internal, 1)
{
    anna_object_t *value = anna_as_obj(param[0]);
    if(value == null_object)
    {
	write(1, "?", 1);	
    }
    else 
    {
	if(value->type == imutable_string_type || value->type == mutable_string_type)
	{
	    anna_string_print(value);
	}	
	else
	{
	    char *msg = "<invalid string type>";	    
	    write(1, msg, strlen(msg));
	}
    }
    return null_entry;
}

ANNA_VM_NATIVE(anna_i_not, 1)
{
    return anna_entry_null(param[0])?anna_from_int(1):null_entry;
}

ANNA_VM_NATIVE(anna_i_gt, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    int vvv = anna_as_int(param[0]);
    return vvv > 0 ? anna_from_int(1) : null_entry;
}

ANNA_VM_NATIVE(anna_i_gte, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    int vvv = anna_as_int(param[0]);
    return vvv >= 0 ? anna_from_int(1) : null_entry;
}

ANNA_VM_NATIVE(anna_i_lt, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    int vvv = anna_as_int(param[0]);
    return vvv < 0 ? anna_from_int(1) : null_entry;
}

ANNA_VM_NATIVE(anna_i_lte, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    int vvv = anna_as_int(param[0]);
    return vvv <= 0 ? anna_from_int(1) : null_entry;
}

ANNA_VM_NATIVE(anna_i_eq, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    int vvv = anna_as_int(param[0]);
    return vvv == 0 ? anna_from_int(1) : null_entry;
}

ANNA_VM_NATIVE(anna_i_neq, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    int vvv = anna_as_int(param[0]);
    return vvv != 0 ? anna_from_int(1) : null_entry;
}

static void anna_i_callcc_callback(anna_context_t *context)
{
    anna_object_t *res = anna_context_pop_object(context);
    anna_context_pop_object(context);
    anna_context_push_object(context, res);
}

static void anna_i_callcc(anna_context_t *context)
{
    context->frame = anna_frame_to_heap(context);
    
    anna_object_t *fun = anna_context_pop_object(context);
    anna_context_pop_object(context);
    anna_object_t *cont = anna_continuation_create(
	&context->stack[0],
	context->top - &context->stack[0],
	context->frame,
	1)->wrapper;
    
    anna_vm_callback_native(
	context, &anna_i_callcc_callback, 0, 0, 
	fun, 1, (anna_entry_t *)&cont);    
}

static void anna_i_wrap_method(anna_context_t *context)
{
    anna_entry_t meth = anna_context_pop_entry(context);
    anna_entry_t obj = anna_context_pop_entry(context);
    anna_context_pop_object(context);
    
    anna_function_t *fun = anna_function_unwrap(anna_as_obj(meth));
    if(fun)
    {
	anna_object_t *res = anna_method_bind(
	    context,
	    fun)->wrapper;
	*anna_entry_get_addr(res, ANNA_MID_THIS) = obj;
	*anna_entry_get_addr(res, ANNA_MID_METHOD) = meth;
	anna_context_push_object(context, res);
    }
    else
    {
	anna_context_push_object(context, null_object);	
    }
    
}

void anna_lang_create_types(anna_stack_template_t *stack_lang)
{
    anna_type_data_create(anna_lang_type_data, stack_lang);    
}

ANNA_VM_NATIVE(anna_reflection_type_of, 1)
{
    anna_object_t *this = anna_as_obj(param[0]);
    return anna_from_obj(anna_type_wrap(this->type));
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
    anna_type_make_sendable(sendable_type);
    anna_type_document(
	sendable_type,
	L"A type that can be sent through a <a path='mp.Channel'>mp.Channel</a>. Sendable types can not be concurrently mutated - they are either immutable or wrap or mutating operations in some kind of locking.");
    
    static wchar_t *p_argn[]={L"object"};

    anna_module_function(
	stack,
	L"printInternal", 
	0, 
	&anna_i_print_internal, 
	object_type, 1, &string_type, 
	p_argn, 0,
	L"Print the specified String to standard output. This is a non-standard internal helper function, do not use it directly. Use the print function instead.");

    anna_module_function(
	stack,
	L"__not__", 0, 
	&anna_i_not, 
	int_type, 
	1, &object_type, p_argn, 0,
	L"Negates the value. Returns 1 of the input is null, null otherwise.");

    anna_module_function(
	stack,
	L"__lteInternal__", 0, 
	&anna_i_lte, 
	int_type, 
	1, &int_type, p_argn, 0,
	L"Returns non-null if the supplied integer is positive, null otherwise. This function is used internally by the __gt__ macro, there should be no reason to ever use it directly.");

    anna_module_function(
	stack,
	L"__eqInternal__", 0, 
	&anna_i_eq, 
	int_type, 
	1, &int_type, p_argn, 0,
	L"Returns non-null if the supplied integer is positive, null otherwise. This function is used internally by the __gt__ macro, there should be no reason to ever use it directly.");

    anna_module_function(
	stack,
	L"__neqInternal__", 0, 
	&anna_i_neq, 
	int_type, 
	1, &int_type, p_argn, 0,
	L"Returns non-null if the supplied integer is positive, null otherwise. This function is used internally by the __gt__ macro, there should be no reason to ever use it directly.");

    anna_module_function(
	stack,
	L"__gtInternal__", 0, 
	&anna_i_gt, 
	int_type, 
	1, &int_type, p_argn, 0,
	L"Returns non-null if the supplied integer is positive, null otherwise. This function is used internally by the __gt__ macro, there should be no reason to ever use it directly.");

    anna_module_function(
	stack,
	L"__gteInternal__", 0, 
	&anna_i_gte, 
	int_type, 
	1, &int_type, p_argn, 0,
	L"Returns non-null if the supplied integer is positive, null otherwise. This function is used internally by the __gt__ macro, there should be no reason to ever use it directly.");

    anna_module_function(
	stack,
	L"__ltInternal__", 0, 
	&anna_i_lt, 
	int_type, 
	1, &int_type, p_argn, 0,
	L"Returns non-null if the supplied integer is positive, null otherwise. This function is used internally by the __gt__ macro, there should be no reason to ever use it directly.");

    anna_module_function(
	stack,
	L"callCC", 0, 
	&anna_i_callcc, 
	object_type, 
	1, &object_type, p_argn, 0,
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

    static wchar_t *type_argn[]={L"type"};

    anna_module_function(
	stack,
	L"type", 0, 
	&anna_reflection_type_of, 
	type_type, 
	1, &object_type, type_argn, 0,
	L"Returns the type of the specified object.");

    anna_type_data_register(anna_lang_type_data, stack);

    anna_stack_document(stack, L"The lang module contains the basic language constructs of Anna.");
    anna_stack_document(stack, L"Basic types like numbers, character strings and data buffers live in the lang module, as do a few nearly universally used functions, like print.");
}
