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

int anna_error_count=0;

void anna_error(anna_node_t *node, wchar_t *msg, ...)
{    
    va_list va;
    va_start( va, msg );
    if(node && (node->location.filename))
    {
	anna_message(
	    L"Error in file «%ls», on line %d:\n", 
	    node->location.filename, node->location.first_line);
	anna_node_print_code(node);
	anna_message(L"\n");
    }
    else
    {
	anna_message(L"Error: ");
    }
    string_buffer_t sb;
    sb_init(&sb);
    sb_vprintf(&sb, msg, va);
    va_end( va );
    anna_message(L"%ls\n\n", sb_content(&sb));
    sb_destroy(&sb);
    anna_error_count++;
}
