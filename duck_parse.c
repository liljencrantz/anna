#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "duck_node.h"
#include "duck_lex.h"

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

/*
void duck_error(wchar_t *filename, size_t pos, wchar_t *msg)
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

static duck_isalpha(wchar_t ch)
{
    return (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z');
}

static duck_isspace(wchar_t ch)
{
    return iswspace(ch);
    
}

static duck_isdigit(wchar_t ch)
{
    return (ch >= L'0' && ch <= L'9');
}

static duck_isalnum(wchar_t ch)
{
    return duck_isalpha(ch) || duck_isdigit(ch);
}

static duck_node_t *parse_int_literal(parse_data_t *d) 
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
      duck_error(d->filename, start_pos, L"Invalid number");
      return 0;
   }
   
   //FIXME: Check for too big numbers!
   

   return (duck_node_t *)duck_node_int_literal_create(0, res);
}


static duck_node_t *parse_lookup(parse_data_t *d) 
{
    int start_pos = d->pos;
    sb_clear(&d->buff);
    while(1)
    {
	wint_t ch = dp_peek(d);
	if(!duck_isalnum(ch) && ch!=L'_')
	{
	    break;
	}
	dp_read(d);
	sb_append_char(&d->buff, ch);
    }
    return (duck_node_t *)duck_node_lookup_create(0, wcsdup((wchar_t *)d->buff.buff));
}

static duck_node_t *parse_string_literal(parse_data_t *d) 
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
		    duck_error(d->filename, start_pos, L"Invalid string literal");
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
    return (duck_node_t *)duck_node_string_literal_create(d->filename, start_pos, sb_length(&d->buff), res);
}

static duck_node_t *parse_char_literal(parse_data_t *d) 
{
    int start_pos = d->pos;
    wchar_t res;
    
    dp_read(d);
    wint_t ch = dp_read(d);
    if(ch == L'\'')
    {
	duck_error(d->filename, start_pos, L"Invalid char literal");
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
		duck_error(d->filename, start_pos, L"Invalid char literal");
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
	duck_error(d->filename, start_pos, L"Invalid char literal");
	return 0;	
    }
    
    return (duck_node_t *)duck_node_char_literal_create(d->filename, start_pos, res);
}

static void parse_skip_space(parse_data_t *d)
{
    while(1)
    {
	wint_t ch = dp_peek(d);
	if(!duck_isspace(ch))
	    return;
	dp_read(d);
    }
}


static duck_node_t *parse(parse_data_t *d)
{
    parse_skip_space(d);
    int start_pos = d->pos;
    wint_t ch = dp_peek(d);
    duck_node_t *result = 0;
    
    if(ch == WEOF)
    {
	return 0;
    }
    
    if((duck_isdigit(ch)) || ch=='-')
    {
	result = parse_int_literal(d);      
    }
    else if((duck_isalpha(ch)) || ch=='_')
    {
	result = parse_lookup(d);      
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
	duck_error(d->filename, start_pos, L"Unknown token type");
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
	    duck_node_t *function = result;
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
		    duck_error(d->filename, start_pos, L"Unexpected EOF");
		    return 0;
		}
		if(ch == L')') 
		{
		    dp_read(d);
		    break;
		}
//		wprintf(L"Got character %lc\n", ch);
		duck_node_t *next_child = parse(d);
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
	    duck_node_t **child_arr = malloc(sizeof(duck_node_t *) * al_get_count(&child));
	    for(i=0; i<al_get_count(&child); i++) 
	    {
		child_arr[i] = (duck_node_t *)al_get( &child, i );
	    }
	    result = (duck_node_t *)duck_node_call_create(d->filename, start_pos, function, al_get_count(&child), child_arr);
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

duck_node_t *duck_parse(wchar_t *filename) 
{
  yyscan_t scanner;

  FILE *file = wfopen(filename, "r");
  duck_node_t *parse_tree;
  
  duck_lex_lex_init(&scanner);
  duck_lex_set_in( file, scanner);
  /*
  YYLTYPE lloc;
  
  lloc.first_line=1;
  lloc.last_line=1;
  lloc.first_column=0;
  lloc.last_column=0;
  lloc.filename = filename;
  */
  //  wprintf(L"Todelo, scanner is %d\n", &scanner);
  duck_yacc_parse( scanner, filename, &parse_tree );
  duck_lex_lex_destroy(scanner);
  fclose(file);
  
  return duck_yacc_error_count?0:parse_tree;
    
/*    
    parse_data_t data;
    parse_data_init(&data, file, filename);

    duck_node_t *result = parse(&data);
    parse_data_destroy(&data);
    return result;
*/ 
   /*
   duck_node_t *param[1];
   param[0] = (duck_node_t *)duck_node_int_literal_create(L"FOO",0,7);
   
   duck_node_t *print_lookup = (duck_node_t *)duck_node_lookup_create(L"FOO",0,L"print");
   
   duck_node_t *program = (duck_node_t *)duck_node_call_create(L"FOO",0, print_lookup, 1, param);
   
   return program;
   */
}
