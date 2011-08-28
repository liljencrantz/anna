#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_type.h"
#include "lang/string.h"
#include "lang/list.h"
#include "lang/int.h"
#include "anna_member.h"
#include "anna_function.h"
#include "anna_function_type.h"
#include "anna_vm.h"
#include "anna_intern.h"
#include "anna_stack.h"
#include "anna_util.h"
#include "anna_node_hash.h"
#include "anna_mid.h"
#include "anna_type_data.h"
#include "anna_module.h"
#include "anna_vm_internal.h"

#include "clib/clib.h"
#include "clib/parser.h"
#include "clib/reflection/function.c"
#include "clib/reflection/member.c"
#include "clib/reflection/type.c"
#include "clib/reflection/continuation.c"

anna_type_t *type_type=0, 
    *member_type=0,
    *function_type_base = 0,
    *member_method_type = 0, 
    *member_property_type = 0, 
    *member_variable_type = 0,
    *continuation_type = 0;

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

static anna_vmstack_t *anna_i_cc(anna_vmstack_t *stack, anna_object_t *me)
{
    stack = anna_frame_to_heap(stack);
    
    anna_vmstack_pop_object(stack);
    anna_object_t *cont = anna_continuation_create(
	stack,
	object_type)->wrapper;
    *anna_entry_get_addr(cont, ANNA_MID_CONTINUATION_STACK) = (anna_entry_t *)stack;
    *anna_entry_get_addr(cont, ANNA_MID_CONTINUATION_CODE_POS) = (anna_entry_t *)stack->code;
    anna_vmstack_push_object(stack, cont);
    return stack;
}

void anna_reflection_load(anna_stack_template_t *stack)
{
    wchar_t *argn[] = 
	{
	    L"value"
	}
    ;
    
    continuation_type = anna_type_for_function(
	object_type,
	0, 0, 0,
	0,
	ANNA_FUNCTION_CONTINUATION);

    anna_type_load();    
    anna_member_load(stack);
    anna_function_load(stack);
    anna_vmstack_load(stack);
    anna_type_data_register(anna_member_type_data, stack);
    anna_stack_declare(
	stack, continuation_type->name,
	type_type, anna_from_obj(anna_type_wrap(continuation_type)), ANNA_STACK_READONLY);

    anna_type_t *type = anna_stack_wrap(stack)->type;
    anna_member_create_native_property(
	type, anna_mid_get(L"currentContinuation"),
	continuation_type,
	&anna_i_cc,
	0,
	L"A continuation of the current execution point.");
}
