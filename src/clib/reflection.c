#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "parser.h"
#include "anna_type.h"
#include "anna_string.h"
#include "anna_list.h"
#include "anna_int.h"
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

#include "clib/anna_function_type.c"

void anna_reflection_create_types(anna_stack_template_t *stack)
{
    anna_member_create_types(stack);
    anna_function_type_create_types(stack);
}

void anna_reflection_load(anna_stack_template_t *stack)
{
    anna_member_load(stack);
    anna_function_type_load(stack);
}

