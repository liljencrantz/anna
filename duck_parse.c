#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "duck_node.h"

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

wchar_t *duck_current_filename;
size_t duck_current_pos;



void duck_error(wchar_t *filename, size_t pos, wchar_t *msg)
{
    wprintf(L"Error in %ls, pos %d: %ls\n", filename, pos, msg);
    
}

duck_node_dummy_t *duck_node_dummy_create(wchar_t *src, size_t src_pos, struct duck_object *val)
{
   duck_node_dummy_t *result = calloc(1,sizeof(duck_node_dummy_t));
   result->node_type = DUCK_NODE_DUMMY;
   result->source_position = src_pos;
   result->source_filename = src;
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;  
}

duck_node_member_get_t *duck_node_member_get_create(wchar_t *src, size_t src_pos, struct duck_node *object, size_t mid, struct duck_type *type, int wrap)
{
   duck_node_member_get_t *result = calloc(1,sizeof(duck_node_member_get_t));
   result->node_type = wrap?DUCK_NODE_MEMBER_GET_WRAP:DUCK_NODE_MEMBER_GET;
   result->source_position = src_pos;
   result->source_filename = src;
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->object=object;
   result->mid=mid;
   result->type=type;
   return result;  
  
}


duck_node_assign_t *duck_node_assign_create(wchar_t *src, size_t src_pos, duck_sid_t sid, struct duck_node *value)
{
   duck_node_assign_t *result = calloc(1,sizeof(duck_node_assign_t));
   result->node_type = DUCK_NODE_ASSIGN;
   result->source_position = src_pos;
   result->source_filename = src;
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->value=value;
   result->sid=sid;
   return result;  
}

duck_node_int_literal_t *duck_node_int_literal_create(wchar_t *src, size_t src_pos, int val)
{
   duck_node_int_literal_t *result = calloc(1,sizeof(duck_node_int_literal_t));
   result->node_type = DUCK_NODE_INT_LITERAL;
   result->source_position = src_pos;
   result->source_filename = src;
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;
}

duck_node_float_literal_t *duck_node_float_literal_create(wchar_t *src, size_t src_pos, double val)
{
   duck_node_float_literal_t *result = calloc(1,sizeof(duck_node_float_literal_t));
   result->node_type = DUCK_NODE_FLOAT_LITERAL;
   result->source_position = src_pos;
   result->source_filename = src;
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;
}

duck_node_char_literal_t *duck_node_char_literal_create(wchar_t *src, size_t src_pos, wchar_t val)
{
   duck_node_char_literal_t *result = calloc(1,sizeof(duck_node_char_literal_t));
   result->node_type = DUCK_NODE_CHAR_LITERAL;
   result->source_position = src_pos;
   result->source_filename = src;
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = val;
   return result;
}

duck_node_string_literal_t *duck_node_string_literal_create(wchar_t *src, size_t src_pos, size_t sz, wchar_t *str)
{
   duck_node_string_literal_t *result = calloc(1,sizeof(duck_node_string_literal_t));
   result->node_type = DUCK_NODE_STRING_LITERAL;
   result->source_position = src_pos;
   result->source_filename = src;
  /*
    FIXME: Create a nice and tidy wrapper
  */
   result->payload = str;
   result->payload_size = sz;
   return result;
}

duck_node_call_t *duck_node_call_create(wchar_t *src, size_t src_pos, duck_node_t *function, size_t argc, duck_node_t **argv)
{
    duck_node_call_t *result = calloc(1,sizeof(duck_node_call_t));
    result->child = calloc(1,sizeof(duck_node_t *)*(argc));
    result->node_type = DUCK_NODE_CALL;
    result->source_position = src_pos;
    result->source_filename = src;
    result->function = function;
    result->child_count = argc;
    result->child_capacity = argc;
    memcpy(result->child, argv, sizeof(duck_node_t *)*(argc));
    /*
      FIXME: Create a nice and tidy wrapper
    */
    return result;
}

duck_node_lookup_t *duck_node_lookup_create(wchar_t *src, size_t src_pos, wchar_t *name)
{
    duck_node_lookup_t *result = calloc(1,sizeof(duck_node_call_t));
    result->node_type = DUCK_NODE_LOOKUP;
    result->source_position = src_pos;
    result->source_filename = src;
    result->name = name;
    /*
      FIXME: Create a nice and tidy wrapper
    */
    return result;
}

duck_node_t *duck_node_null_create(wchar_t *src, size_t src_pos)
{
    duck_node_t *result = calloc(1,sizeof(duck_node_t));
    result->node_type = DUCK_NODE_NULL;
    result->source_position = src_pos;
    result->source_filename = src;
    /*
      FIXME: Create a nice and tidy wrapper
    */
    return result;
}

void duck_node_call_add_child(duck_node_call_t *call, duck_node_t *child)
{
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 8 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(duck_node_t *));
	if(!call->child) 
	{
	    wprintf(L"Out of memory\n");
	    exit(1);
	}	
	call->child_capacity = new_capacity;
    }
//    wprintf(L"LALALA %d %d\n", call->child_capacity, call->child_count);
    
    call->child[call->child_count++] = child;
}

void duck_node_call_set_function(duck_node_call_t *call, duck_node_t *function)
{
    call->function = function;
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

/*
  Like iswalpha, but only allow a-z, not localized chars
*/
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
   
   /*
     FIXME: Check for too big numbers!
   */

   return (duck_node_t *)duck_node_int_literal_create(d->filename, start_pos, res);
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
    return (duck_node_t *)duck_node_lookup_create(d->filename, start_pos, wcsdup((wchar_t *)d->buff.buff));
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

int yyparse ();

duck_node_t *duck_parse(FILE *file, wchar_t *filename) 
{
    duck_current_filename = filename;
    duck_current_pos=0;
    yyparse();
    return duck_yacc_error?0:duck_parse_tree;
    
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
