#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/common.h"
#include "util.h"
#include "anna.h"
#include "anna/node.h"

int anna_error_count=0;

void anna_error(anna_node_t *node, wchar_t *msg, ...)
{    
    va_list va;
    va_start( va, msg );
    if(node && (node->location.filename))
    {
	fwprintf(
	    stderr,L"Error in file «%ls», on line %d:\n", 
	    node->location.filename, node->location.first_line);
	anna_node_print_code(node);
	fwprintf(stderr,L"\n");
    }
    else
    {
	fwprintf(stderr,L"Error: ");
    }
    
    vfwprintf(stderr, msg, va);
    va_end( va );
    fwprintf(stderr, L"\n\n");
    anna_error_count++;
}

