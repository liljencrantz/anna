#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

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

#include "anna/lib/clib.h"
#include "anna/lib/parser.h"
#include "anna/lib/reflection.h"

#include "src/lib/reflection/function.c"
#include "src/lib/reflection/type.c"
#include "src/lib/reflection/member.c"

anna_type_t *type_type=0, 
    *member_type=0,
    *function_type_base = 0,
    *member_method_type = 0, 
    *member_property_type = 0, 
    *member_variable_type = 0,
    *continuation_type = 0,
    *block_type = 0;

static anna_type_data_t anna_member_type_data[] = 
{
    { &type_type,L"Type" },
    { &function_type_base, L"Function" },
    { &member_type, L"Member" },
    { &member_method_type, L"Method" },
    { &member_property_type, L"Property" },
    { &member_variable_type, L"Variable" },
//    { &continuation_type, L"Continuation" },
}
    ;

void anna_reflection_create_types(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_member_type_data, stack);
}

static void anna_i_cc(anna_context_t *stack)
{
    stack->frame = anna_frame_to_heap(stack->frame);
    
    anna_context_pop_object(stack);
    anna_context_pop_object(stack);
    anna_object_t *cont = anna_continuation_create(
	&stack->stack[0],
	stack->top - &stack->stack[0],
	stack->frame,
	1)->wrapper;
    anna_context_push_object(stack, cont);
}

void anna_reflection_load(anna_stack_template_t *stack)
{
    continuation_type = anna_type_get_function(
	object_type,
	0, 0, 0,
	0,
	ANNA_FUNCTION_CONTINUATION);
    block_type = anna_type_get_function(
	object_type,
	0, 0, 0,
	0,
	0);

    anna_type_load();    
    anna_member_load(stack);
    anna_function_load(stack);
    anna_type_data_register(
	anna_member_type_data, stack);
    anna_stack_declare(
	stack, continuation_type->name,
	type_type, anna_from_obj(anna_type_wrap(continuation_type)), ANNA_STACK_READONLY);
    anna_stack_declare(
	stack, L"Block",
	type_type, anna_from_obj(anna_type_wrap(block_type)), ANNA_STACK_READONLY);
    
    anna_type_t *type = anna_stack_wrap(stack)->type;
    anna_member_create_native_property(
	type, anna_mid_get(L"currentContinuation"),
	continuation_type,
	&anna_i_cc,
	0,
	L"A continuation of the current execution point.");
}
