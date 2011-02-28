#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "wutil.h"
#include "anna_node.h"

static void anna_indent(int indentation)
{
    int indent;
    for(indent=0; indent<indentation; indent++)
    {
	fwprintf(stderr,L"    ");
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
	     (ct==ANNA_NODE_CLOSURE) ||
	     (ct==ANNA_NODE_MEMBER_CALL) ||
	     (ct==ANNA_NODE_MEMBER_GET) ||
	     (ct==ANNA_NODE_MEMBER_GET_WRAP) ||
	     (ct==ANNA_NODE_MEMBER_SET) ||
	     (ct==ANNA_NODE_IF) ||
	     (ct==ANNA_NODE_NULL)))
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
	    fwprintf(stderr,L"%d", this2->payload);
	    break;
	}
	
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_indent(indentation);
	    anna_node_float_literal_t *this2 = (anna_node_float_literal_t *)this;
	    fwprintf(stderr,L"%f", this2->payload);
	    break;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_indent(indentation);
	    anna_node_string_literal_t *this2 = (anna_node_string_literal_t *)this;
	    int i;
	    
	    fwprintf(stderr,L"\"");
	    for(i=0;i<this2->payload_size;i++)
	    {
		wchar_t c = this2->payload[i];
		if(c<32) 
		{
		    fwprintf(stderr,L"\\x%.2x", c);		    
		}
		else
		{
		    fwprintf(stderr,L"%lc", c);
		}
	    }
	    fwprintf(stderr,L"\"");
	    
	    break;
	}
	
	case ANNA_NODE_CHAR_LITERAL:
	{
	    anna_indent(indentation);
	    anna_node_char_literal_t *this2 = (anna_node_char_literal_t *)this;
	    fwprintf(stderr,L"'%lc'", this2->payload);
	    break;
	}
	
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_indent(indentation);
	    anna_node_identifier_t *this2 = (anna_node_identifier_t *)this;
	    fwprintf(stderr,L"%ls", this2->name);
	    break;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_indent(indentation);
	    anna_node_assign_t *this2 = (anna_node_assign_t *)this;
	    fwprintf(stderr,L"__assign__(");

	    fwprintf(stderr,L"%d:%d", this2->sid.frame, this2->sid.offset);
	    fwprintf(stderr,L";\n");
	    anna_node_print_internal(this2->value, indentation+1);
	    fwprintf(stderr,L")");
	    
	    break;
	}
	
	case ANNA_NODE_DUMMY:
	{
	    anna_indent(indentation);
	    anna_node_dummy_t *this2 = (anna_node_dummy_t *)this;
	    fwprintf(stderr,L"<Dummy>: %ls", this2->payload->type->name);
	    break;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_indent(indentation);
	    anna_node_closure_t *this2 = (anna_node_closure_t *)this;
	    if(this2->payload)
	    {
		fwprintf(stderr,L"*closure(\n");
		anna_node_print_internal((anna_node_t *)this2->payload->body, indentation+1);
		fwprintf(stderr,L")");
	    }
	    else
	    {
		fwprintf(stderr,L"<Closure>: <ERROR>");		
	    }
	    break;
	}
	
	case ANNA_NODE_BLOB:
	{
	    anna_indent(indentation);
	    //anna_node_dummy_t *this2 = (anna_node_dummy_t *)this;
	    fwprintf(stderr,L"<Blob>");
	    break;
	}
	

	case ANNA_NODE_MEMBER_GET:
	{
	    anna_indent(indentation);
	    anna_node_member_get_t *this2 = (anna_node_member_get_t *)this;
	    fwprintf(stderr,L"__memberGet__(\n");
	    anna_node_print_internal(this2->object, indentation+1);
	    fwprintf(stderr,L"; %ls)", anna_mid_get_reverse(this2->mid));
	    break;
	}

	case ANNA_NODE_DECLARE:
	{
	    anna_indent(indentation);
	    anna_node_declare_t *this2 = (anna_node_declare_t *)this;
	    fwprintf(stderr,L"*__var__(\n");
	    anna_indent(indentation+1);
	    fwprintf(stderr,L"%ls;\n", this2->name);
	    anna_node_print_internal(this2->type, indentation+1);
	    fwprintf(stderr,L";\n");
	    anna_node_print_internal(this2->value, indentation+1);
	    fwprintf(stderr,L")");
	    break;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_indent(indentation);
	    anna_node_member_set_t *this2 = (anna_node_member_set_t *)this;
	    fwprintf(stderr,L"__memberSet__(\n");
	    anna_node_print_internal(this2->object, indentation+1);
	    fwprintf(stderr,L";\n");
	    anna_indent(indentation+1);
	    fwprintf(stderr,L"%ls;\n", anna_mid_get_reverse(this2->mid));
	    anna_node_print_internal(this2->value, indentation+1);
	    fwprintf(stderr,L")");

	    break;
	}

	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_indent(indentation);
	    anna_node_member_get_t *this2 = (anna_node_member_get_t *)this;
	    fwprintf(stderr,L"__memberGet__(\n");
	    anna_node_print_internal(this2->object, indentation+1);
	    fwprintf(stderr,L", %ls)", anna_mid_get_reverse(this2->mid));
	    break;
	}
	
	case ANNA_NODE_NULL:
	{
	    anna_indent(indentation);
	    fwprintf(stderr,L"null");
	    break;
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *this2 = (anna_node_call_t *)this;	    
	    int i;
//	    fwprintf(stderr,L"/*%d*/", this2);
	    anna_node_print_internal(this2->function, indentation);
	    /*	    fwprintf(stderr,L"\n");
		    anna_indent(indentation);*/
	    if(this2->child_count == 0)
	    {
		fwprintf(stderr,L"()" );		
	    }
	    else if(is_simple(this2, 5))
	      {
		fwprintf(stderr,L"(");
		
		for(i=0; i<this2->child_count; i++)
		{
		    if(i!=0) 
		    {
			fwprintf(stderr,L";");
		    }
		    anna_node_print_internal(this2->child[i], 0);
		}
		/*	    fwprintf(stderr,L"\n" );
			    anna_indent(indentation);*/
		fwprintf(stderr,L")" );
		
	      }
	    else 
	    {
		fwprintf(stderr,L"(\n");
		
		for(i=0; i<this2->child_count; i++)
		{
		    if(i!=0) 
		    {
			fwprintf(stderr,L";\n");
		    }
		    anna_node_print_internal(this2->child[i], indentation+1);
		}
		/*	    fwprintf(stderr,L"\n" );
			    anna_indent(indentation);*/
		fwprintf(stderr,L")" );
	    }
	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_member_call_t *this2 = (anna_node_member_call_t *)this;	    
	    int i;
	    anna_indent(indentation);
	    fwprintf(stderr,L"*__memberGet__(\n");
	    anna_node_print_internal(this2->object, indentation+1);
	    fwprintf(stderr,L",\n");
	    anna_indent(indentation+1);
	    fwprintf(stderr,L"\"%ls\")", anna_mid_get_reverse(this2->mid));
		
	    if(this2->child_count == 0)
	    {
		fwprintf(stderr,L"()" );		
	    }
	    else
	    {
		fwprintf(stderr,L"(\n");
		
		for(i=0; i<this2->child_count; i++)
		{
		    if(i!=0) 
		    {
			fwprintf(stderr,L";\n");
		    }
		    anna_node_print_internal(this2->child[i], indentation+1);
		}
		fwprintf(stderr,L")" );
	    }
	    break;
	}
	
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *this2 = (anna_node_if_t *)this;	    
	    anna_indent(indentation);
	    fwprintf(stderr,L"*if(\n");
	    anna_node_print_internal(this2->cond, indentation+1);
	    fwprintf(stderr,L",\n");
	    anna_node_print_internal((anna_node_t *)this2->block1, indentation+1);
	    fwprintf(stderr,L",\n");
	    anna_node_print_internal((anna_node_t *)this2->block2, indentation+1);
	    fwprintf(stderr,L")");
		
	    break;
	}
	
	default:
	{
	    fwprintf(stderr,L"<Don't know how to print node of type %d>", this->node_type);
	    break;
	}
    }

}


void anna_node_print(anna_node_t *this)
{
    if(debug_level < 2)
	return;
    
  anna_node_print_internal(this, 0);
  fwprintf(stderr,L"\n");
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
		    fwprintf(stderr, L"\x1b[0m");		
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
		fwprintf(stderr, L"\x1b[0m");		
	    }
	    is_marking=0;
	    fwprintf(stderr, L"%*d: ", 6, current_line);
	    
	}
	
	
	mark = is_after_first && is_before_last;
	if(print && mark != is_marking)
	{
	    if(mark)
	    {
		fwprintf(stderr, L"\x1b[31m");
	    }
	    else 
	    {
		fwprintf(stderr, L"\x1b[0m");		
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
