#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_node_wrapper.h"
#include "anna_vm.h"

struct anna_node *anna_macro_invoke(
    anna_function_t *macro, 
    anna_node_call_t *node)
{
    anna_object_t *wrapped_node = anna_node_wrap((anna_node_t *)node);
    anna_object_t *res = anna_vm_run(macro->wrapper, 1, &wrapped_node);
    return anna_node_unwrap(res);
}

