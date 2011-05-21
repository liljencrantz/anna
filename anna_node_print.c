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

static int byte_count(wchar_t ch)
{
    char bytes[8];
    return wctomb(bytes, ch);
}

static void anna_indent(string_buffer_t *sb, int indentation)
{
    int indent;
    for(indent=0; indent<indentation; indent++)
    {
	sb_printf(sb,L"    ");
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


static void anna_node_print_internal(
    string_buffer_t *sb, anna_node_t *this, int indentation)
{
    if(!this)
    {
	anna_indent(sb,indentation);
	sb_printf(sb,L"<null>");
	return;
    }
    
    switch(this->node_type)
    {
	case ANNA_NODE_INT_LITERAL:
	{
	    anna_indent(sb,indentation);
	    anna_node_int_literal_t *this2 = (anna_node_int_literal_t *)this;
	    char *nstr = mpz_get_str(0, 10, this2->payload);
	    sb_printf(sb,L"%s", nstr);
	    free(nstr);
	    break;
	}
	
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_indent(sb,indentation);
	    anna_node_float_literal_t *this2 = (anna_node_float_literal_t *)this;

	    string_buffer_t tmp;
	    sb_init(&tmp);
	    sb_printf(&tmp,L"%f", this2->payload);
	    wchar_t *buff = sb_content(&tmp);
	    wchar_t *comma = wcschr(buff, ',');
	    if(comma)
	    {
		*comma = '.';
	    }
	    sb_printf(sb, L"%ls", buff);
	    sb_destroy(&tmp);
	    
	    break;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_indent(sb,indentation);
	    anna_node_string_literal_t *this2 = (anna_node_string_literal_t *)this;
	    int i;
	    
	    sb_printf(sb,L"\"");
	    for(i=0;i<this2->payload_size;i++)
	    {
		wchar_t c = this2->payload[i];
		if(c<32) 
		{
		    sb_printf(sb,L"\\x%.2x", c);		    
		}
		else
		{
		    sb_printf(sb,L"%lc", c);
		}
	    }
	    sb_printf(sb,L"\"");
	    
	    break;
	}
	
	case ANNA_NODE_CHAR_LITERAL:
	{
	    anna_indent(sb,indentation);
	    anna_node_char_literal_t *this2 = (anna_node_char_literal_t *)this;
	    sb_printf(sb,L"'%lc'", this2->payload);
	    break;
	}
	
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_indent(sb,indentation);
	    anna_node_identifier_t *this2 = (anna_node_identifier_t *)this;
	    sb_printf(sb,L"%ls", this2->name);
	    break;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_indent(sb,indentation);
	    anna_node_assign_t *this2 = (anna_node_assign_t *)this;
	    sb_printf(sb,L"__assign__(");

	    sb_printf(sb,L"%d:%d", this2->sid.frame, this2->sid.offset);
	    sb_printf(sb,L";\n");
	    anna_node_print_internal(sb,this2->value, indentation+1);
	    sb_printf(sb,L")");
	    
	    break;
	}
	
	case ANNA_NODE_DUMMY:
	{
	    anna_indent(sb,indentation);
	    anna_node_dummy_t *this2 = (anna_node_dummy_t *)this;
	    sb_printf(sb,L"<Dummy>: %ls", this2->payload->type->name);
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_indent(sb,indentation);
	    anna_node_type_t *this2 = (anna_node_type_t *)this;
	    sb_printf(sb,L"%ls", this2->payload->name);
	    break;
	}
	
	case ANNA_NODE_TYPE_LOOKUP:
	case ANNA_NODE_TYPE_LOOKUP_RETURN:
	{
	    anna_indent(sb,indentation);
	    sb_printf(sb,this->node_type==ANNA_NODE_TYPE_LOOKUP?L"__typeOf__":L"__typeOfReturn__(\n");
	    anna_node_t *chld = anna_node_type_lookup_get_payload(this);
	    anna_node_print_internal(
		sb, chld, indentation+1);
	    sb_printf(sb,L")");
	    break;
	}
	
	case ANNA_NODE_RETURN:
	{
	    anna_indent(sb,indentation);
	    sb_printf(sb, L"return(\n");
	    anna_node_print_internal(
		sb, ((anna_node_wrapper_t *)this)->payload, indentation+1);
	    sb_printf(sb,L")");
	    break;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_indent(sb,indentation);
	    anna_node_closure_t *this2 = (anna_node_closure_t *)this;
	    if(this2->payload)
	    {
		sb_printf(sb,L"*closure(\n");
		if(this2->payload->body)
		    anna_node_print_internal(sb,(anna_node_t *)this2->payload->body, indentation+1);
		sb_printf(sb,L")");
	    }
	    else
	    {
		sb_printf(sb,L"<Closure>: <ERROR>");		
	    }
	    break;
	}
	
	case ANNA_NODE_MEMBER_GET:
	{
	    anna_indent(sb,indentation);
	    anna_node_member_access_t *this2 = (anna_node_member_access_t *)this;
	    sb_printf(sb,L"*__memberGet__(\n");
	    anna_node_print_internal(sb,this2->object, indentation+1);
	    sb_printf(sb,L"; %ls)", anna_mid_get_reverse(this2->mid));
	    break;
	}

	case ANNA_NODE_DECLARE:
	{
	    anna_indent(sb,indentation);
	    anna_node_declare_t *this2 = (anna_node_declare_t *)this;
	    sb_printf(sb,L"*__var__(\n");
	    anna_indent(sb,indentation+1);
	    sb_printf(sb,L"%ls;\n", this2->name);
	    anna_node_print_internal(sb,this2->type, indentation+1);
	    sb_printf(sb,L";\n");
	    anna_node_print_internal(sb,this2->value, indentation+1);
	    sb_printf(sb,L")");
	    break;
	}

	case ANNA_NODE_CONST:
	{
	    anna_indent(sb,indentation);
	    anna_node_declare_t *this2 = (anna_node_declare_t *)this;
	    sb_printf(sb,L"*__const__(\n");
	    anna_indent(sb,indentation+1);
	    sb_printf(sb,L"%ls;\n", this2->name);
	    anna_node_print_internal(sb,this2->type, indentation+1);
	    sb_printf(sb,L";\n");
	    anna_node_print_internal(sb,this2->value, indentation+1);
	    sb_printf(sb,L")");
	    break;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_indent(sb,indentation);
	    anna_node_member_access_t *this2 = (anna_node_member_access_t *)this;
	    sb_printf(sb,L"__memberSet__(\n");
	    anna_node_print_internal(sb,this2->object, indentation+1);
	    sb_printf(sb,L";\n");
	    anna_indent(sb,indentation+1);
	    sb_printf(sb,L"%ls;\n", anna_mid_get_reverse(this2->mid));
	    anna_node_print_internal(sb,this2->value, indentation+1);
	    sb_printf(sb,L")");

	    break;
	}

	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_indent(sb,indentation);
	    anna_node_member_access_t *this2 = (anna_node_member_access_t *)this;
	    sb_printf(sb,L"*__memberGet__(\n");
	    anna_node_print_internal(sb,this2->object, indentation+1);
	    sb_printf(sb,L", %ls)", anna_mid_get_reverse(this2->mid));
	    break;
	}
	
	case ANNA_NODE_NULL:
	{
	    anna_indent(sb,indentation);
	    sb_printf(sb,L"null");
	    break;
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *this2 = (anna_node_call_t *)this;	    
	    int i;
//	    sb_printf(sb,L"/*%d*/", this2);
	    anna_node_print_internal(sb,this2->function, indentation);
	    /*	    sb_printf(sb,L"\n");
		    anna_indent(sb,indentation);*/
	    if(this2->child_count == 0)
	    {
		sb_printf(sb,L"()" );		
	    }
	    else if(is_simple(this2, 5))
	      {
		sb_printf(sb,L"(");
		
		for(i=0; i<this2->child_count; i++)
		{
		    if(i!=0) 
		    {
			sb_printf(sb,L";");
		    }
		    anna_node_print_internal(sb,this2->child[i], 0);
		}
		/*	    sb_printf(sb,L"\n" );
			    anna_indent(sb,indentation);*/
		sb_printf(sb,L")" );
		
	      }
	    else 
	    {
		sb_printf(sb,L"(\n");
		
		for(i=0; i<this2->child_count; i++)
		{
		    if(i!=0) 
		    {
			sb_printf(sb,L";\n");
		    }
		    anna_node_print_internal(sb,this2->child[i], indentation+1);
		}
		/*	    sb_printf(sb,L"\n" );
			    anna_indent(sb,indentation);*/
		sb_printf(sb,L")" );
	    }
	    break;
	}

	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *this2 = (anna_node_call_t *)this;	    
	    anna_indent(sb,indentation);
	    sb_printf(sb,L"__cast__(\n");		
	    anna_node_print_internal(sb,this2->child[0], indentation+1);
	    sb_printf(sb,L";\n");		
	    anna_node_print_internal(sb,this2->child[1], indentation+1);
	    sb_printf(sb,L")");
	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_call_t *this2 = (anna_node_call_t *)this;	    
	    int i;
	    anna_indent(sb,indentation);
	    sb_printf(sb,L"*__memberGet__(\n");
	    anna_node_print_internal(sb,this2->object, indentation+1);
	    sb_printf(sb,L",\n");
	    anna_indent(sb,indentation+1);
	    sb_printf(sb,L"\"%ls\")", anna_mid_get_reverse(this2->mid));
		
	    if(this2->child_count == 0)
	    {
		sb_printf(sb,L"()" );		
	    }
	    else
	    {
		sb_printf(sb,L"(\n");
		
		for(i=0; i<this2->child_count; i++)
		{
		    if(i!=0) 
		    {
			sb_printf(sb,L";\n");
		    }
		    anna_node_print_internal(sb,this2->child[i], indentation+1);
		}
		sb_printf(sb,L")" );
	    }
	    break;
	}
	
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *this2 = (anna_node_if_t *)this;	    
	    anna_indent(sb,indentation);
	    sb_printf(sb,L"*if(\n");
	    anna_node_print_internal(sb,this2->cond, indentation+1);
	    sb_printf(sb,L",\n");
	    anna_node_print_internal(sb,(anna_node_t *)this2->block1, indentation+1);
	    sb_printf(sb,L",\n");
	    anna_node_print_internal(sb,(anna_node_t *)this2->block2, indentation+1);
	    sb_printf(sb,L")");
		
	    break;
	}
	
	case ANNA_NODE_SPECIALIZE:
	{
	    anna_node_call_t *this2 = (anna_node_call_t *)this;	    
	    int i;
//	    sb_printf(sb,L"/*%d*/", this2);
	    anna_indent(sb,indentation);
	    sb_printf(sb,L"*__specialize__(");
	    anna_node_print_internal(sb,this2->function, 0);
	    /*	    sb_printf(sb,L"\n");
		    anna_indent(sb,indentation);*/
	    if(this2->child_count == 0)
	    {
		sb_printf(sb,L"()" );		
	    }
	    else
	    {
		sb_printf(sb,L"(");
		
		for(i=0; i<this2->child_count; i++)
		{
		    if(i!=0) 
		    {
			sb_printf(sb,L";");
		    }
		    anna_node_print_internal(sb,this2->child[i], 0);
		}
		/*	    sb_printf(sb,L"\n" );
			    anna_indent(sb,indentation);*/
		sb_printf(sb,L")" );
		
	    }
	    sb_printf(sb,L")");
	    break;
	}
	
	default:
	{
	    sb_printf(sb,L"<Don't know how to print node of type %d>", this->node_type);
	    break;
	}
    }

}


void anna_node_print(int level, anna_node_t *this)
{
    if( level < debug_level )
	return;
    string_buffer_t sb;
    sb_init(&sb);
    anna_node_print_internal(&sb, this, 0);
    fwprintf(stderr,L"%ls\n", sb_content(&sb));
    sb_destroy(&sb);
}

wchar_t *anna_node_string(anna_node_t *this)
{
    string_buffer_t sb;
    sb_init(&sb);
    anna_node_print_internal(&sb, this, 0);
    return sb_content(&sb);
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
		current_column+=byte_count(res);
		break;
	}
    }
}
