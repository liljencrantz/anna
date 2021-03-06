#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/fallback.h"
#include "anna/common.h"
#include "anna/base.h"
#include "anna/node.h"
#include "anna/node_create.h"
#include "anna/type.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/int.h"
#include "anna/member.h"
#include "anna/function.h"
#include "anna/function_type.h"
#include "anna/vm.h"
#include "anna/intern.h"
#include "anna/stack.h"
#include "anna/util.h"
#include "anna/node_hash.h"
#include "anna/mid.h"
#include "anna/type_data.h"
#include "anna/module.h"
#include "anna/vm_internal.h"
#include "anna/object.h"

#include "anna/lib/clib.h"
#include "anna/lib/parser.h"
#include "anna/lib/reflection.h"
#include "anna/lib/lang/hash.h"

#include "src/lib/reflection/function.c"
#include "src/lib/reflection/type.c"
#include "src/lib/reflection/member.c"
#include "src/lib/reflection/continuation_member.c"

anna_type_t *type_type=0, 
    *member_type=0,
    *function_type_base = 0,
    *member_method_type = 0, 
    *member_property_type = 0, 
    *member_variable_type = 0,
    *continuation_type = 0,
    *continuation_member_type = 0,
    *block_type = 0;

static anna_type_data_t anna_member_type_data[] = 
{
    { &type_type,L"Type" },
    { &function_type_base, L"Function" },
    { &member_type, L"Member" },
    { &member_method_type, L"Method" },
    { &member_property_type, L"Property" },
    { &member_variable_type, L"Variable" },
    { &continuation_member_type, L"ContinuationMember" },
//    { &continuation_type, L"Continuation" },
}
    ;

void anna_reflection_create_types(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_member_type_data, stack);
}

static void anna_i_cc(anna_context_t *context)
{
    context->frame = anna_frame_to_heap(context);
    
    anna_context_pop_object(context);
    anna_context_pop_object(context);
    anna_object_t *cont = anna_continuation_create(
	&context->stack[0],
	context->top - &context->stack[0],
	context->frame,
	1)->wrapper;
    anna_context_push_object(context, cont);
}

ANNA_VM_NATIVE(anna_reflection_heap_size, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    if(anna_is_obj(param[0]))
    {
	anna_object_t *obj = anna_as_obj(param[0]);
	return anna_from_int(obj->type->object_size);
    }
    return anna_from_int(0);
}

void anna_reflection_load(anna_stack_template_t *stack)
{
    anna_type_load();    
    continuation_type = anna_type_get_function(
	any_type,
	0, 0, 0,
	0,
	ANNA_FUNCTION_CONTINUATION);
    block_type = anna_type_get_function(
	any_type,
	0, 0, 0,
	0,
	0);

    block_type->name = anna_intern(L"Block");

    anna_type_document(
	block_type,
	L"A Block is a Anna function that accepts no parameters.");
    anna_type_document(
	block_type,
	L"There is nothing inherently different or special about blocks compared to other function types.");

    anna_member_load(stack);
    anna_function_load(stack);
    anna_continuation_member_load(stack);
    anna_type_data_register(
	anna_member_type_data, stack);
    anna_stack_declare(
	stack, continuation_type->name,
	type_type, anna_from_obj(anna_type_wrap(continuation_type)), ANNA_STACK_READONLY | ANNA_STACK_ASSIGNED);
    anna_stack_declare(
	stack, L"Block",
	type_type, anna_from_obj(anna_type_wrap(block_type)), ANNA_STACK_READONLY | ANNA_STACK_ASSIGNED);
    
    anna_type_t *type = anna_stack_wrap(stack)->type;
    anna_member_create_native_property(
	type, anna_mid_get(L"currentContinuation"),
	continuation_type,
	&anna_i_cc,
	0,
	L"A continuation of the current execution point.");

    anna_stack_document(stack, L"The reflection module contains tools used for run time introspection of functions and types.");

    static wchar_t *hs_argn[]={L"object"};

    anna_module_function(
	stack,
	L"heapSize", 0, 
	&anna_reflection_heap_size, 
	int_type, 
	1, &any_type, hs_argn, 0,
	L"Returns the amount of heap memory used by the specified object. Note that objects that can be stored directly on the stack (such as integers) may return a size of zero.");

}
