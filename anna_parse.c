#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "wutil.h"
#include "anna_node.h"
#include "anna_lex.h"
#include "anna_yacc.h"

struct parse_data 
{
    FILE *file;
    wint_t readahead;
    wchar_t *filename;
    size_t pos;
    string_buffer_t buff;
}
   ;
typedef struct parse_data parse_data_t;

int anna_yacc_parse(yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr);
void anna_yacc_init();

/*
void anna_error(wchar_t *filename, size_t pos, wchar_t *msg)
{
    wprintf(L"Error in %ls, pos %d: %ls\n", filename, pos, msg);
    
}

static void parse_data_init(parse_data_t *d, FILE *f, wchar_t *filename)
{
   d->file = f;
   d->filename = filename;
   d->readahead = WEOF;
   d->pos=0;
   sb_init(&d->buff);   
}

static void parse_data_destroy(parse_data_t *d)
{
   sb_destroy(&d->buff);
}

static wint_t dp_read(parse_data_t *d)
{
   wint_t res;
   d->pos++;
   
   if(d->readahead != WEOF) 
   {
      res = d->readahead;
      d->readahead = WEOF;
   }
   else 
   {
      res = fgetwc(d->file);
   }
   return res;   
}

static wint_t dp_peek(parse_data_t *d)
{
   wint_t res = dp_read(d);
   d->pos--;
   d->readahead = res;
   return res;
}

//  Like iswalpha, but only allow a-z, not localized chars

static anna_isalpha(wchar_t ch)
{
    return (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z');
}

static anna_isspace(wchar_t ch)
{
    return iswspace(ch);
    
}

static anna_isdigit(wchar_t ch)
{
    return (ch >= L'0' && ch <= L'9');
}

static anna_isalnum(wchar_t ch)
{
    return anna_isalpha(ch) || anna_isdigit(ch);
}

static anna_node_t *parse_int_literal(parse_data_t *d) 
{
   long long res = 0;
   int minus=0;
   int count = 0;
   
   int start_pos = d->pos;
   
   wint_t ch = dp_peek(d);
   if(ch =='-') 
   {
      minus = 1;
      dp_read(d);
   }

   while(1)
   {
      wint_t ch = dp_peek(d);
      if(ch < '0' || ch > '9' || count > 12)
      {
	 break;
      }
      dp_read(d);
            
      count++;
      res = res*10 + (ch-'0');
   }
   
   if(minus)
      res = -res;
   
   if(count == 0 || count > 12) 
   {
      anna_error(d->filename, start_pos, L"Invalid number");
      return 0;
   }
   
   //FIXME: Check for too big numbers!
   

   return (anna_node_t *)anna_node_int_literal_create(0, res);
}


static anna_node_t *parse_identifier(parse_data_t *d) 
{
    int start_pos = d->pos;
    sb_clear(&d->buff);
    while(1)
    {
	wint_t ch = dp_peek(d);
	if(!anna_isalnum(ch) && ch!=L'_')
	{
	    break;
	}
	dp_read(d);
	sb_append_char(&d->buff, ch);
    }
    return (anna_node_t *)anna_node_identifier_create(0, wcsdup((wchar_t *)d->buff.buff));
}

static anna_node_t *parse_string_literal(parse_data_t *d) 
{
    int start_pos = d->pos;
    wchar_t *res;
    
    sb_clear(&d->buff);
    dp_read(d);
    while(1)
    {
	wint_t ch = dp_read(d);
	if(ch == L'"')
	{
	    break;
	}
	else if(ch == L'\\') 
	{
	    ch = dp_read(d);
	    switch(ch)
	    {
		case L'\\':
		case L'\"':
		case L'\'':
		    sb_append_char(&d->buff, ch);
		    break;
		    
		case L'n':
		    sb_append_char(&d->buff, L'\n');
		    break;
		    
		case L't':
		    sb_append_char(&d->buff, L'\t');
		    break;
		    
		case L'r':
		    sb_append_char(&d->buff, L'\r');
		    break;
		default:
		    anna_error(d->filename, start_pos, L"Invalid string literal");
		    return 0;		    
	    }
	    
	}
	else
	{
	    sb_append_char(&d->buff, ch);
	}
	
    }
    res = malloc(sizeof(wchar_t) * sb_length(&d->buff));
    memcpy(res, (wchar_t *)d->buff.buff, sizeof(wchar_t) * sb_length(&d->buff));
    return (anna_node_t *)anna_node_string_literal_create(d->filename, start_pos, sb_length(&d->buff), res);
}

static anna_node_t *parse_char_literal(parse_data_t *d) 
{
    int start_pos = d->pos;
    wchar_t res;
    
    dp_read(d);
    wint_t ch = dp_read(d);
    if(ch == L'\'')
    {
	anna_error(d->filename, start_pos, L"Invalid char literal");
	return 0;
    }
    else if(ch == L'\\') 
    {
	ch = dp_read(d);
	switch(ch)
	{
	    case L'\\':
	    case L'\"':
	    case L'\'':
		res = ch;
	    break;
	    
	    case L'n':
		res = L'\n';
		break;
		
	    case L't':
		res = L'\t';
		break;
		
	    case L'r':
		res = L'\r';
		break;
	    default:
		anna_error(d->filename, start_pos, L"Invalid char literal");
		return 0;		    
	}
	    
    }
    else
    {
	res = ch;
    }	
    ch = dp_read(d);
    if(ch != L'\'') 
    {
	anna_error(d->filename, start_pos, L"Invalid char literal");
	return 0;	
    }
    
    return (anna_node_t *)anna_node_char_literal_create(d->filename, start_pos, res);
}

static void parse_skip_space(parse_data_t *d)
{
    while(1)
    {
	wint_t ch = dp_peek(d);
	if(!anna_isspace(ch))
	    return;
	dp_read(d);
    }
}


static anna_node_t *parse(parse_data_t *d)
{
    parse_skip_space(d);
    int start_pos = d->pos;
    wint_t ch = dp_peek(d);
    anna_node_t *result = 0;
    
    if(ch == WEOF)
    {
	return 0;
    }
    
    if((anna_isdigit(ch)) || ch=='-')
    {
	result = parse_int_literal(d);      
    }
    else if((anna_isalpha(ch)) || ch=='_')
    {
	result = parse_identifier(d);      
    }
    else if(ch == L'"')
    {
	result = parse_string_literal(d);      
    }
    else if(ch == L'\'')
    {
	result = parse_char_literal(d);      
    }
    else 
    {
	anna_error(d->filename, start_pos, L"Unknown token type");
	return 0;
    }
    if(!result)
    {
	return 0;	
    }
    
    
    while(1)
    {
	parse_skip_space(d);
	ch = dp_peek(d);
	if(ch == L'(') 
	{
	    anna_node_t *function = result;
	    array_list_t child;
	    int i;
	    
	    
	    al_init(&child);
	    
	    dp_read(d);
	    while(1) 
	    {
		parse_skip_space(d);
		ch = dp_peek(d);
		if(ch == WEOF) 
		{
		    al_destroy(&child);
		    anna_error(d->filename, start_pos, L"Unexpected EOF");
		    return 0;
		}
		if(ch == L')') 
		{
		    dp_read(d);
		    break;
		}
//		wprintf(L"Got character %lc\n", ch);
		anna_node_t *next_child = parse(d);
		if(!next_child)
		{
		    al_destroy(&child);
		    return 0;
		}
		al_push(&child, next_child);
		parse_skip_space(d);

		ch = dp_peek(d);
		if(ch == L';') 
		{
		    dp_read(d);
		}
		
	    }
	    anna_node_t **child_arr = malloc(sizeof(anna_node_t *) * al_get_count(&child));
	    for(i=0; i<al_get_count(&child); i++) 
	    {
		child_arr[i] = (anna_node_t *)al_get( &child, i );
	    }
	    result = (anna_node_t *)anna_node_call_create(d->filename, start_pos, function, al_get_count(&child), child_arr);
	    al_destroy(&child);
	}
	else 
	{
	    break;
	}
	
    }
    
    return result;
   
}
*/

anna_node_t *anna_parse(wchar_t *filename) 
{
  yyscan_t scanner;

  FILE *file = wfopen(filename, "r");
  anna_node_t *parse_tree;

  anna_yacc_init();
  anna_lex_lex_init(&scanner);

  anna_lex_set_in( file, scanner);
  /*
  YYLTYPE lloc;
  
  lloc.first_line=1;
  lloc.last_line=1;
  lloc.first_column=0;
  lloc.last_column=0;
  lloc.filename = filename;
  */
  //  wprintf(L"Todelo, scanner is %d\n", &scanner);
  anna_yacc_parse( scanner, filename, &parse_tree );
  anna_lex_lex_destroy(scanner);
  fclose(file);
  
  return anna_yacc_error_count?0:parse_tree;
    
/*    
    parse_data_t data;
    parse_data_init(&data, file, filename);

    anna_node_t *result = parse(&data);
    parse_data_destroy(&data);
    return result;
*/ 
   /*
   anna_node_t *param[1];
   param[0] = (anna_node_t *)anna_node_int_literal_create(L"FOO",0,7);
   
   anna_node_t *print_identifier = (anna_node_t *)anna_node_identifier_create(L"FOO",0,L"print");
   
   anna_node_t *program = (anna_node_t *)anna_node_call_create(L"FOO",0, print_identifier, 1, param);
   
   return program;
   */
}
