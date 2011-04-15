#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_node_check.h"

int check_node_identifier_name(anna_node_t *n,
			       wchar_t *name)
{
    if(anna_node_is_named(n, name))
    {
	return 1;
    }    
    anna_error((anna_node_t *)n,
	       L"Unexpected argument. Expected an identifier named \"%ls\".", name);
    return 0;
}


int check_node_block(anna_node_t *n)
{
    if(anna_node_is_call_to(n, L"__block__"))
    {
	return 1;
    }    
    anna_error((anna_node_t *)n,
	       L"Unexpected argument type. Expected a block definition.");
    return 0;
}
