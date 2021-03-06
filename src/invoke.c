#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/fallback.h"
#include "anna/common.h"
#include "anna/util.h"
#include "anna/base.h"
#include "anna/node.h"
#include "anna/node_create.h"
#include "anna/lib/parser.h"
#include "anna/vm.h"

struct anna_node *anna_macro_invoke(
    anna_function_t *macro, 
    anna_node_call_t *node)
{
    anna_entry_t wrapped_node = anna_from_obj(anna_node_wrap((anna_node_t *)node));
    anna_object_t *res = anna_vm_run(macro->wrapper, 1, &wrapped_node);
    anna_node_t *nn = anna_node_unwrap(res);
    
    if(!nn)
    {
	nn = (anna_node_t *)anna_node_create_null(&node->location);
    }
    return nn;
}

