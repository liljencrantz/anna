#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "wutil.h"
#include "anna_node.h"

static void anna_indent(int indentation)
{
    int indent;
    for(indent=0; indent<indentation; indent++)
    {
	wprintf(L"    ");
    }
}

static int is_simple(anna_node_call_t *call, int max_items)
{
    if(call->child_count > max_items)
    {
        return 0;
    }
    int i;

    if(call->child_count == 1 &&call->child[0]->node_type == ANNA_NODE_CALL) 
    {
      return is_simple((anna_node_call_t *)call->child[0], max_items-1);
    }

    for(i=0;i<call->child_count; i++)
    {
        int ct = call->child[i]->node_type;
	if(!((ct==ANNA_NODE_INT_LITERAL) || 
	     (ct==ANNA_NODE_FLOAT_LITERAL) ||
	     (ct==ANNA_NODE_STRING_LITERAL) ||
	     (ct==ANNA_NODE_IDENTIFIER) ||
	     (ct==ANNA_NODE_CHAR_LITERAL) ||
	     (ct==ANNA_NODE_DUMMY) ||
	     (ct==ANNA_NODE_TRAMPOLINE) ||
	     (ct==ANNA_NODE_NULL)) )
	    return 0;	
    }
    return 1;
}


void anna_node_print_internal(anna_node_t *this, int indentation)
{
    switch(this->node_type)
    {
	case ANNA_NODE_INT_LITERAL:
	{
	    anna_indent(indentation);
	    anna_node_int_literal_t *this2 = (anna_node_int_literal_t *)this;
	    wprintf(L"%d", this2->payload);
	    break;
	}
	
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_indent(indentation);
	    anna_node_float_literal_t *this2 = (anna_node_float_literal_t *)this;
	    wprintf(L"%f", this2->payload);
	    break;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_indent(indentation);
	    anna_node_string_literal_t *this2 = (anna_node_string_literal_t *)this;
	    int i;
	    
	    wprintf(L"\"");
	    for(i=0;i<this2->payload_size;i++)
	    {
		wchar_t c = this2->payload[i];
		if(c<32) 
		{
		    wprintf(L"\\x%.2x", c);		    
		}
		else
		{
		    wprintf(L"%lc", c);
		}
	    }
	    wprintf(L"\"");
	    
	    break;
	}
	
	case ANNA_NODE_CHAR_LITERAL:
	{
	    anna_indent(indentation);
	    anna_node_char_literal_t *this2 = (anna_node_char_literal_t *)this;
	    wprintf(L"'%lc'", this2->payload);
	    break;
	}
	
	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_IDENTIFIER_TRAMPOLINE:
	{
	    anna_indent(indentation);
	    anna_node_identifier_t *this2 = (anna_node_identifier_t *)this;
	    wprintf(L"%ls", this2->name);
	    break;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_indent(indentation);
	    anna_node_assign_t *this2 = (anna_node_assign_t *)this;
	    wprintf(L"__assign__(");

	    wprintf(L"%d:%d", this2->sid.frame, this2->sid.offset);
	    wprintf(L";\n");
	    anna_node_print_internal(this2->value, indentation+1);
	    wprintf(L")");
	    
	    break;
	}
	
	case ANNA_NODE_TRAMPOLINE:
	case ANNA_NODE_DUMMY:
	{
	    anna_indent(indentation);
	    anna_node_dummy_t *this2 = (anna_node_dummy_t *)this;
	    wprintf(L"<Dummy>");
	    break;
	}
	

	case ANNA_NODE_MEMBER_GET:
	{
	    anna_indent(indentation);
	    anna_node_member_get_t *this2 = (anna_node_member_get_t *)this;
	    wprintf(L"__memberGet__(\n");
	    anna_node_print_internal(this2->object, indentation+1);
	    wprintf(L"; %ls)", anna_mid_get_reverse(this2->mid));
	    break;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_indent(indentation);
	    anna_node_member_set_t *this2 = (anna_node_member_set_t *)this;
	    wprintf(L"__memberSet__(\n");
	    anna_node_print_internal(this2->object, indentation+1);
	    wprintf(L";\n");
	    anna_indent(indentation+1);
	    wprintf(L"%ls;\n", anna_mid_get_reverse(this2->mid));
	    anna_node_print_internal(this2->value, indentation+1);
	    wprintf(L")");

	    break;
	}

	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_indent(indentation);
	    anna_node_member_get_t *this2 = (anna_node_member_get_t *)this;
	    wprintf(L"__memberGet__(\n");
	    anna_node_print_internal(this2->object, indentation+1);
	    wprintf(L", %ls)", anna_mid_get_reverse(this2->mid));
	    break;
	}
	
	case ANNA_NODE_NULL:
	{
	    anna_indent(indentation);
	    wprintf(L"null");
	    break;
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *this2 = (anna_node_call_t *)this;	    
	    int i;
//	    wprintf(L"/*%d*/", this2);
	    anna_node_print_internal(this2->function, indentation);
	    /*	    wprintf(L"\n");
		    anna_indent(indentation);*/
	    if(this2->child_count == 0)
	    {
		wprintf(L"()" );		
	    }
	    else if(is_simple(this2, 5))
	      {
		wprintf(L"(");
		
		for(i=0; i<this2->child_count; i++)
		{
		    if(i!=0) 
		    {
			wprintf(L";");
		    }
		    anna_node_print_internal(this2->child[i], 0);
		}
		/*	    wprintf(L"\n" );
			    anna_indent(indentation);*/
		wprintf(L")" );
		
	      }
	    else 
	    {
		wprintf(L"(\n");
		
		for(i=0; i<this2->child_count; i++)
		{
		    if(i!=0) 
		    {
			wprintf(L";\n");
		    }
		    anna_node_print_internal(this2->child[i], indentation+1);
		}
		/*	    wprintf(L"\n" );
			    anna_indent(indentation);*/
		wprintf(L")" );
	    }
	    break;
	}
	
	default:
	{
	    wprintf(L"<Don't know how to print node of type %d>", this->node_type);
	    break;
	}
    }

}


void anna_node_print(anna_node_t *this)
{
  anna_node_print_internal(this, 0);
  wprintf(L"\n");
}

void anna_node_print_code(anna_node_t *node)
{
    int current_line=1;
    int current_column=0;
    int print=0;
    int mark=0;
    int is_after_first;
    int is_before_last;
    int is_marking=0;
    
    if(!node->location.filename)
	return;
    
    
    FILE *file = wfopen(node->location.filename, "r");
    if(!file)
    {
	fwprintf(stderr, L"Error: %ls: Not found\n", node->location.filename);
	return;
    }    
    while(1)
    {
	wint_t res = fgetwc(file);
	switch(res)
	{
	    case WEOF:
		if(is_marking)
		{
		    fwprintf(stderr, L"\e[0m");		
		}
		return;
	}
	
	print = (current_line >=(node->location.first_line-1)) && (current_line <= node->location.last_line+1);

	is_after_first  = (current_line >node->location.first_line) || (current_line == node->location.first_line && current_column >= node->location.first_column);
	is_before_last  = (current_line <node->location.last_line) || (current_line == node->location.last_line && current_column < node->location.last_column);
	
	if(current_column == 0 && print)
	{
	    if(is_marking)
	    {
		fwprintf(stderr, L"\e[0m");		
	    }
	    is_marking=0;
	    fwprintf(stderr, L"%*d: ", 6, current_line);
	    
	}
	
	
	mark = is_after_first && is_before_last;
	if(print && mark != is_marking)
	{
	    if(mark)
	    {
		fwprintf(stderr, L"\e[31m");
	    }
	    else 
	    {
		fwprintf(stderr, L"\e[0m");		
	    }
	    
	}
	is_marking = mark;
	if(print)
	{
	    fputwc(res,stderr);
	}

	switch(res)
	{
	    case L'\n':
		current_line++;
		current_column=0;
		break;
	    default:
		current_column++;
		break;
	}	
    }    
}
