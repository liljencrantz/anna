#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "anna_node_hash.h"
#include "anna_node.h"


static void anna_node_hash_func_step(
    anna_node_t *this, void *aux)
{
    int *res = (int *)aux;
    *res = (*res << 3) | (*res >> 29);
    *res += this->node_type;
    switch(this->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *i = (anna_node_identifier_t *)this;
	    *res += wcslen(i->name);
	}

	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *i = (anna_node_call_t *)this;
	    *res += i->child_count;
	}
    }    
}

int anna_node_hash_func( void *data )
{
    int res = 0xDEADBEEF;
    anna_node_each(
	(anna_node_t *)data,
	anna_node_hash_func_step, &res);
    return res;
}

int anna_node_hash_cmp( 
    void *a,
    void *b )
{
    anna_node_t *na = (anna_node_t *)a;
    anna_node_t *nb = (anna_node_t *)b;
    return anna_node_compare(na, nb) == 0;
}


