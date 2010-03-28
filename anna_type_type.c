#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_type.h"

void anna_type_type_create(anna_stack_frame_t *stack)
{

    type_type = anna_type_create(L"Type", 64, 1);
    anna_type_definition_make(type_type);

    anna_node_call_t *definition = anna_type_definition_get(type_type);

    anna_member_add_node(
	definition,
	ANNA_MID_TYPE_WRAPPER_PAYLOAD,
	L"!typeWrapperPayload",
	0,
	(anna_node_t *)anna_node_identifier_create(0, L"Null"));

    anna_member_create(
	type_type,
	ANNA_MID_TYPE_WRAPPER_PAYLOAD,
	L"!typeWrapperPayload",
	0,
	null_type);
}

