%lex-param   {yyscan_t scanner}
%lex-param   {wchar_t *filename}
%parse-param   {yyscan_t scanner}
%parse-param   {wchar_t *filename}
%parse-param   {anna_node_t **parse_tree_ptr}

%{

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <wchar.h>
#include <string.h>
#include <math.h>

#include "anna/common.h"
#include "anna/node.h"
#include "anna/node_create.h"
#include "autogen/yacc.h"
#include "autogen/lex.h"
#include "anna/intern.h"

void anna_yacc_error(YYLTYPE *llocp, yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr, char *s);
int anna_yacc_parse(yyscan_t scanner, wchar_t *filename, anna_node_t **parse_tree_ptr);
int anna_yacc_lex (YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner, wchar_t *filename);
int was_end_brace=0;
int peek_token = -1;
size_t last_length = 0;

int yylex();

int yylex_val;
int anna_yacc_error_count=0; 

int anna_yacc_do_init = 0;

static string_buffer_t anna_yacc_str_buff;


# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do									\
    {									\
      (Current).filename = filename;					\
	if (N)								\
        {                                                               \
	    (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;	\
	    (Current).first_column = YYRHSLOC(Rhs, 1).first_column;	\
	    (Current).last_line    = YYRHSLOC(Rhs, N).last_line;	\
	    (Current).last_column  = YYRHSLOC(Rhs, N).last_column;	\
        }                                                               \
	else								\
        {                                                               \
	    (Current).first_line   = (Current).last_line   =		\
		YYRHSLOC(Rhs, 0).last_line;				\
	    (Current).first_column = (Current).last_column =		\
		YYRHSLOC(Rhs, 0).last_column;				\
        }								\
    }									\
    while (0)

#define anna_yacc_string( ... ) anna_yacc_string_internal( __VA_ARGS__, (char *)0 )
__sentinel static wchar_t *anna_yacc_string_internal(char *in, ...)

{
    va_list va;
    char *arg;
    
    va_start( va, in );
    sb_clear(&anna_yacc_str_buff);
    sb_printf(&anna_yacc_str_buff, L"%s", in);
    
    while( (arg=va_arg(va, char *) )!= 0 ) 
    {
	sb_printf(&anna_yacc_str_buff, L"%s", arg);
    }
    va_end( va );

    return sb_content(&anna_yacc_str_buff);
}

static anna_node_t *anna_atoi(YYLTYPE *llocp, char *c, int base)
{
    mpz_t res;
    mpz_t mpval;
    mpz_t mpbase;
    mpz_init(res);
    mpz_init(mpval);
    mpz_init(mpbase);
    mpz_set_si(res, 0);
    mpz_set_si(mpbase, base);
    
    while(1)
    {
	char ch = *(c++);
	if(ch == '_')
	    continue;

	int val;

	if( (ch >= '0') && (ch <= '9'))
	{
	    val = ch - '0';
	}
	else if( (ch >= 'a') && (ch <= 'f'))
	{
	    val = ch - 'a' + 10;
	}
	else if( (ch >= 'A') && (ch <= 'F'))
	{
	    val = ch - 'A' + 10;
	}
	else
	{
	    break;
	}
	if(val >= base)
	{
	    break;
	}

	mpz_set_si(mpval, val);

	mpz_mul(res, mpbase, res);
	mpz_add(res, res, mpval);
    }


    anna_node_t *node = (anna_node_t *)anna_node_create_int_literal(llocp, res);
    mpz_clear(res);
    mpz_clear(mpbase);
    mpz_clear(mpval);

    if(*--c)
    {
	node = (anna_node_t *)
	    anna_node_create_call2(
		llocp,
		(anna_node_t *)anna_node_create_identifier(
		    llocp,
		    anna_yacc_string(c, "IntLiteral")),
		node);
    }

    return node;
}


static anna_node_t *anna_atof(YYLTYPE *llocp, char *c)
{
    char *cpy = malloc(strlen(c)+1);
    char *ptr = cpy;
    while(*c)
    {
	if(*c != '_')
	    *(ptr++) = *c;
	c++;
    }
    *ptr=0;
    char *end;
    int tmp = errno;
    errno = 0;
    double res = strtod(cpy, &end);
    if(errno)
    {
	anna_message(
	    L"Error in %ls, on line %d:\n", 
	    llocp->filename,
	    llocp->first_line);
	anna_node_print_code((anna_node_t *)anna_node_create_dummy(llocp, 0));
	
	anna_message(L"Invalid value for Float literal\n");
	anna_yacc_error_count++;
    }

    errno = tmp;
    anna_node_t *node = (anna_node_t *)anna_node_create_float_literal(llocp, res);
    
    if(*end)
    {
	node = (anna_node_t *)
	    anna_node_create_call2(
		llocp,
		(anna_node_t *)anna_node_create_identifier(
		    llocp,
		    anna_yacc_string(end, "FloatLiteral")),
		node);
    }
    
    free(cpy);

    return node;
}

static anna_node_t *anna_text_as_id(anna_location_t *loc, yyscan_t *scanner)
{
    return (anna_node_t *)anna_node_create_identifier(
	loc,anna_yacc_string(anna_lex_get_text(scanner)));
}

enum 
{
    ANNA_LIT_BASE,
    ANNA_LIT_BACKSLASH,
    ANNA_LIT_HEX
};

/*
  Remove quote characters and unescape backslash escapes in a string
  literal. (Works with either double or single quotes, e.g. also for
  char literals)

  Returns a newly malloc-ed string
 */
static wchar_t *anna_yacc_string_unescape(anna_location_t *loc, char *str, size_t *count)
{
    str++;
    str[strlen(str)-1]=0;
    wchar_t *str2 = str2wcs(str);
    wchar_t *str3 = malloc(sizeof(wchar_t)*(wcslen(str2)+1));
    wchar_t *ptr_in;
    wchar_t *ptr_out = str3;
    int mode = ANNA_LIT_BASE;
    int chars_left=0;
    int hex_val=0;
    
    for(ptr_in=str2; *ptr_in; ptr_in++)
    {
	switch(mode)
	{
	    case ANNA_LIT_BASE:
		
		if(*ptr_in == L'\\')
		{
		    mode = ANNA_LIT_BACKSLASH;
		}
		else
		{
		    *ptr_out++ = *ptr_in;
		}
		break;
		
	    case ANNA_LIT_BACKSLASH:
		mode = ANNA_LIT_BASE;
		
		switch(*ptr_in)
		{
		    case L'x':
			mode = ANNA_LIT_HEX;
			chars_left = 2;
			hex_val = 0;
			break;

		    case L'u':
			mode = ANNA_LIT_HEX;
			chars_left = 4;
			hex_val = 0;
			break;

		    case L'U':
			mode = ANNA_LIT_HEX;
			chars_left = 8;
			hex_val = 0;
			break;

		    case L'n':
			*ptr_out++ = L'\n';
			break;
			
		    case L'r':
			*ptr_out++ = L'\r';
			break;
			
		    case L'e':
			*ptr_out++ = L'\x1b';
			break;
			
		    case L't':
			*ptr_out++ = L'\t';
			break;
			
		    case L'0':
			*ptr_out++ = L'\0';
			break;
			
		    default:
			*ptr_out++ = *ptr_in;
			break;
		}
		break;

	    case ANNA_LIT_HEX:
	    {
		int digit;
		if(*ptr_in >= L'0' && *ptr_in <= L'9')
		{
		    digit = *ptr_in - L'0';
		}
		else if(*ptr_in >= L'a' && *ptr_in <= L'f')
		{
		    digit = *ptr_in - L'a'+10;
		}
		else if(*ptr_in >= L'A' && *ptr_in <= L'F')
		{
		    digit = *ptr_in - L'A'+10;
		}
		else
		{
		    anna_error(
			(anna_node_t *)anna_node_create_dummy(loc, 0),
			L"Invalid hex sequence");
		    return 0;
		}
		
		hex_val = hex_val*16 + digit;
		if(--chars_left == 0)
		{
		    mode = ANNA_LIT_BASE;
		    *ptr_out++ = hex_val;
		}
	    }
	}
    }
    free(str2);
    if(mode != ANNA_LIT_BASE)
    {
	anna_error(
	    (anna_node_t *)anna_node_create_dummy(loc, 0),
	    L"Invalid hex sequence");
	return 0;
    }
    *ptr_out = 0;
    *count = ptr_out - str3;
    return str3;
}

static anna_node_t *anna_yacc_string_literal_create(anna_location_t *loc, char *str)
{
    size_t count;
    char *str_start = strchr(str, '"');
    wchar_t *wstr = anna_yacc_string_unescape(loc, str_start, &count);
    if(!wstr)
    {
	return anna_node_create_null(loc);
    }
    int free = 1;

    if(count < 8 && wcslen(wstr) == count)
    {
	wstr = anna_intern_or_free(wstr);
	free = 0;
    }
    anna_node_t *res = (anna_node_t *)anna_node_create_string_literal(loc, count, wstr, free);
    if(str_start != str)
    {
	*str_start = 0;
	res = (anna_node_t *)anna_node_create_call2(
	    loc,
	    anna_node_create_identifier(
		loc,anna_yacc_string(str, "StringLiteral")),
	    res);
    }
    
    return res;
}
 
static anna_node_t *anna_yacc_char_literal_create(anna_location_t *loc, char *str)
{
    size_t count;
    wchar_t *wstr = anna_yacc_string_unescape(loc, str, &count);

    if(!wstr)
    {
	return anna_node_create_null(loc);
    }

    if(count != 1){
	anna_error(
	    (anna_node_t *)anna_node_create_dummy(loc, 0),
	    L"Invalid character literal");
	return anna_node_create_null(loc);
    }

    anna_node_t *res = (anna_node_t *)anna_node_create_char_literal(loc, *wstr);
    free(wstr);
    return res;
}

static anna_node_identifier_t *identifier_enclose(wchar_t *pre, anna_node_identifier_t *node, wchar_t *post)
{
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%ls%ls%ls", pre, node->name, post);
    anna_node_identifier_t *res = anna_node_create_identifier(&node->location, sb_content(&sb));
    sb_destroy(&sb);
    return res;
}

 
%}

%error-verbose

%union {
    anna_node_t *node_val;
    anna_node_identifier_t *identifier_val;
    anna_node_call_t *call_val;
}

%locations

%token LITERAL_INTEGER_BASE_2
%token LITERAL_INTEGER_BASE_8
%token LITERAL_INTEGER_BASE_10
%token LITERAL_INTEGER_BASE_16
%token LITERAL_FLOAT
%token LITERAL_CHAR
%token LITERAL_STRING
%token IDENTIFIER
%token TYPE_IDENTIFIER
%token APPEND
%token INCREASE
%token DECREASE
%token NEXT
%token PREV
%token RANGE
%token DEF
%token MACRO
%token NULL_SYM
%token EQUAL
%token NOT_EQUAL
%token GREATER_OR_EQUAL
%token LESS_OR_EQUAL
%token AND
%token OR
%token VAR
%token CONST
%token RETURN
%token RAISE
%token BREAK
%token CONTINUE
%token SEPARATOR
%token IGNORE
%token LINE_BREAK
%token AS
%token IN
%token ELLIPSIS
%token TO
%token PAIR
%token DECLARE_VAR
%token DECLARE_CONST
%token SPECIALIZATION_BEGIN
%token SPECIALIZATION_END
%token SPECIALIZATION_BEGIN2
%token SPECIALIZATION_END2
%token STATIC_MEMBER_GET

%type <call_val> block opt_expression_list expression_list
%type <call_val> module
%type <node_val> expression expression2 expression3 expression4 expression5 expression6 expression7 expression8 expression9 expression10 
%type <node_val> literal var_or_const
%type <node_val> opt_declaration_init opt_ellipsis
%type <node_val> function_definition function_declaration function_signature
%type <node_val> opt_identifier identifier identifier2 type_identifier type_identifier2 any_identifier
%type <node_val> op op2 op3 op4 op5 op6 op7 post_op8
%type <node_val> type_definition type_remainder
%type <call_val> declaration_list declaration_list2
%type <node_val> declaration_list_item declaration_expression variable_declaration
%type <call_val> attribute_list 
%type <call_val> opt_block
%type <call_val> specialization opt_specialization
%type <call_val> opt_type_and_opt_name type_and_name
%type <node_val> jump

%pure-parser

%%

module: opt_expression_list
	{
	    $$->function = 
		(anna_node_t *)anna_node_create_identifier(
		    &@$,L"nothing");
	    *parse_tree_ptr = (anna_node_t *)$$;
	} |
	error separators
	{
	    yyerrok;
	    $$ = 0;  
	};

block: '{' opt_expression_list '}'
	{
	    $$ = $2;
	};

separators : SEPARATOR | separators SEPARATOR;

opt_expression_list : 
	opt_separator
	{
	    $$ = anna_node_create_block2(&@$);
	} |
	opt_separator expression_list opt_separator
	{
	    $$ = $2;
	};

expression_list :
	expression_list separators expression
	{
	    if($1)
	    {
		$$ = $1;
		anna_node_call_add_child($1,$3);
		anna_node_set_location((anna_node_t *)$$, &@$);
	    }
	} |
	expression
	{
	    $$ = anna_node_create_block2(&@$, $1);
	};

opt_declaration_init :
	'=' expression
	{
	    $$ = $2;
	} |
	/* Empty */
	{
	    $$ = 0;
	};

expression:
	expression2 '=' expression
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		(anna_node_t *)anna_node_create_identifier(&@$,L"__assign__"),
		$1, $3);
	} |
	expression2 op expression
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		$2,
		$1, $3);
	} | 
	expression2 | 
	declaration_expression |
	type_definition |
	jump expression
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		$1,
		$2);
	} |
	jump
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
	      &@$, 
	      $1,
	      anna_node_create_null(&@$));  
	};

jump:
	RETURN
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@1,L"return");
	} |
	CONTINUE
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@1,L"continue");
	} |
	BREAK
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@1,L"break");
	};

expression2 :
	expression2 op2 expression3
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		$2,
		$1, $3);
	} |
	expression3;

opt_separator: /* Empty */ | separators;

expression3 :
	expression3 op3 expression4
	{
	    if(anna_node_is_named($2, L"__in__"))
	    {
		$$ = (anna_node_t *)
		    anna_node_create_call2(
			&@$, 
			anna_node_create_call2(
			    &@$, anna_node_create_identifier(&@$,L"__memberGet__"),
			    $3, $2),
			$1);
	    }
	    else 
	    {
		$$ = (anna_node_t *)
		    anna_node_create_call2(
			&@$, 
			anna_node_create_call2(
			    &@$, 
			    anna_node_create_identifier(&@$, L"__memberGet__"),
			    $1, $2),
			$3);
	    }	    
	} |
	expression4;

expression4 :
	expression4 op4 expression5
	{
	    $$ = (anna_node_t *)
		anna_node_create_call2(
		    &@$, 
		    anna_node_create_call2(
			&@$, 
			anna_node_create_identifier(&@$, L"__memberGet__"),
			$1, $2),
		    $3);
	} |
	expression4 RANGE expression5
	{
	    anna_node_t *op = (anna_node_t *)anna_node_create_identifier(
		&@$,L"__range__");
	    $$ = (anna_node_t *)anna_node_create_call2(&@$, op, $1, $3);
	} |
	expression4 ELLIPSIS
	{
	    anna_node_t *op = (anna_node_t *)anna_node_create_identifier(
		&@$,L"__range__");
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$, op, $1, anna_node_create_null(&@$));
	} |
	expression5;

expression5 :
	expression5 op5 expression6
	{
	    $$ = (anna_node_t *)anna_node_create_call2(&@$, $2, $1, $3);
	} |
	expression6;

expression6 :
	expression6 op6 expression7
	{
	    $$ = (anna_node_t *)
		anna_node_create_call2(
		    &@$, 
		    anna_node_create_call2(
			&@$, 
			anna_node_create_identifier(&@$, L"__memberGet__"),
			$1, $2),
		    $3);
	} |
	expression7;

expression7:
	expression7 op7 expression8
	{
	    $$ = (anna_node_t *)
		anna_node_create_call2(
		    &@$, 
		    anna_node_create_call2(
			&@$, 
			anna_node_create_identifier(&@$, L"__memberGet__"),
			$1, $2),
		    $3);
	} |
	expression8;

expression8:
	'!' expression9
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		anna_node_create_identifier(&@$,L"__not__"), 
		$2);
	} |
	'^' identifier expression9
	{
	    $$ = (anna_node_t *)
		anna_node_create_call2(
		    &@$, 
		    anna_node_create_call2(
			&@$, 
			anna_node_create_identifier(&@2, L"__memberGet__"),
			$3, $2));
	} |
	expression9 post_op8
	{
	  $$ = (anna_node_t *)
	      anna_node_create_call2(
		  &@$, 
		  $2,
		  $1);
	} |
	expression9 |
	expression8 AS expression9
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$, 
		anna_node_create_identifier(&@2,L"cast"), 
		$1, $3);
	};

expression9 :
	expression9 '(' opt_expression_list ')' opt_block
	{
	    $$ = (anna_node_t *)$3;
	    anna_node_t *fun = $1;

	    anna_node_call_set_function($3, fun);
	    anna_node_set_location($$, &@$);
	    if ($5) 
		anna_node_call_add_child($3, (anna_node_t *)$5);
	} |
	'-' expression10
	{
	    if($2->node_type == ANNA_NODE_INT_LITERAL)
	    {
		anna_node_int_literal_t *val = (anna_node_int_literal_t *)$2;
		mpz_neg(val->payload, val->payload);
		$$ = $2;		
	    }
	    else if($2->node_type == ANNA_NODE_FLOAT_LITERAL)
	    {
		anna_node_float_literal_t *val = (anna_node_float_literal_t *)$2;
		val->payload = -val->payload;
		$$ = $2;
	    }
	    else
	    {
		anna_node_t *op = (anna_node_t *)anna_node_create_identifier(&@$,L"__neg__");
		$$ = (anna_node_t *)
		    anna_node_create_call2(
			&@$, 
			anna_node_create_call2(
			    &@$, 
			    anna_node_create_identifier(&@2, L"__memberGet__"),
			    $2, op));
	    }
	} |
	expression10 block
	{
	    $$ = (anna_node_t *)anna_node_create_call2(&@$, $1, $2);
	} |
	expression9 '[' opt_expression_list ']'
	{
	    $$ = (anna_node_t *)
		anna_node_create_call2(
		    &@$, 
		    anna_node_create_call2(
			&@$, 
			anna_node_create_identifier(&@$, L"__memberGet__"),
			$1, 
			anna_node_create_identifier(&@$, L"__get__")));
	    int i;
	    if($3)
	    {
		for(i=0; i<$3->child_count; i++)
		{
		    anna_node_call_add_child((anna_node_call_t *)$$,$3->child[i]);
		}
	    }
	    
	} |
	expression10 |
	opt_specialization '[' opt_expression_list ']'
	{
	    $$ = (anna_node_t *)$3;
	    anna_node_t *spec = (anna_node_t *)$1;
	    
	    if(!spec)
	    {
		spec = anna_node_create_null(&@$);
	    }
	    
	    anna_node_call_set_function(
		$3,
		(anna_node_t *)anna_node_create_identifier(
		    &@$,
		    L"__collection__"));
	    anna_node_call_prepend_child($3, spec);
	} |
	expression9 '.' expression10
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		anna_node_create_identifier(&@2, L"__memberGet__"), 
		$1, $3);
	} |
	expression9 STATIC_MEMBER_GET expression10
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		anna_node_create_identifier(&@2, L"__staticMemberGet__"), 
		$1, $3);
	} |
	expression9 specialization
	{
		$$ = (anna_node_t *)anna_node_create_call2(
		    &@$, anna_node_create_identifier(&@$, L"__specialize__"), 
		    $1, $2);		
	};

expression10:
	literal |
	any_identifier |
	'(' expression_list ')'
	{
	    if($2->child_count == 1)
	    {
		$$ = $2->child[0];
	    }
	    else
	    {
		$2->function = (anna_node_t *)anna_node_create_identifier(&@2, L"nothing");
		$$ = (anna_node_t *)$2;
	    }
	} |
	NULL_SYM
	{
	    $$ = anna_node_create_null(&@$);
	} |
	block 
	{
	    $$ = (anna_node_t *)$1; 
	};


op:
	APPEND
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__append__");
	} |
	INCREASE
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__increase__");
	} |
	DECREASE
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__decrease__");
	} |
	TO
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__mapping__");
	};

op2:
	AND
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__and__");
	} |
	OR
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__or__");
	};

op3:
	'<'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__lt__");
	} |
	'>'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__gt__");
	} |
	EQUAL
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__eq__");
	} |
	NOT_EQUAL
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__neq__");
	} |
	LESS_OR_EQUAL
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__lte__");
	} |
	GREATER_OR_EQUAL
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__gte__");
	} |
	IN
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__in__");
	};

op4:
	'^' identifier
	{
	    $$ = $2
	};

op5:
	PAIR
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"Pair");
	};

op6:
	'+'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__add__");
	} |
	'-'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__sub__");
	} |
	'~'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__join__");
	};

op7:
	'*'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__mul__");
	} |
	'/'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__div__");
	} |
	'%'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__format__");
	};

post_op8:
	NEXT
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__next__")
	} |
	PREV
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__prev__")
	};

opt_identifier:
	identifier |
	/* Empty */
	{
	    $$ = 0;
	};

type_identifier:
	type_identifier2 |
	'%' type_identifier2
	{
	    anna_node_identifier_t *ii = (anna_node_identifier_t *)$2;
	    ii->node_type = ANNA_NODE_INTERNAL_IDENTIFIER;
	    ii->location.first_line = @1.first_line;
	    ii->location.first_column = @1.first_column;
	    $$ = $2;
	};

type_identifier2:
	TYPE_IDENTIFIER
	{
	    $$ = anna_text_as_id(&@$, scanner);
	};

identifier:
	identifier2 |
	'%' identifier2
	{
	    anna_node_identifier_t *ii = (anna_node_identifier_t *)$2;
	    ii->node_type = ANNA_NODE_INTERNAL_IDENTIFIER;
	    ii->location.first_line = @1.first_line;
	    ii->location.first_column = @1.first_column;
	    $$ = $2;
	};

identifier2:
	IDENTIFIER
	{
	    $$ = anna_text_as_id(&@$, scanner);
	} |
	IN
	{
	    $$ = anna_text_as_id(&@$, scanner);
	} |
	AS
	{
	    $$ = anna_text_as_id(&@$, scanner);
	} |
	AND
	{
	    $$ = anna_text_as_id(&@$, scanner);
	} |
	OR
	{
	    $$ = anna_text_as_id(&@$, scanner);
	};

any_identifier: identifier | type_identifier;

literal:
	LITERAL_INTEGER_BASE_10
	{
	    $$ = anna_atoi(&@$, anna_lex_get_text(scanner), 10);
	} | 
	LITERAL_INTEGER_BASE_16
	{
	    $$ = anna_atoi(&@$, anna_lex_get_text(scanner)+2, 16);
	} | 
	LITERAL_INTEGER_BASE_8
	{
	    $$ = anna_atoi(&@$, anna_lex_get_text(scanner)+2, 8);
	} | 
	LITERAL_INTEGER_BASE_2
	{
	    $$ = anna_atoi(&@$, anna_lex_get_text(scanner)+2, 2);
	} | 
	LITERAL_FLOAT
	{
	    $$ = anna_atof(&@$, anna_lex_get_text(scanner));
	} | 
	LITERAL_CHAR
	{
	    $$ = anna_yacc_char_literal_create(&@$, anna_lex_get_text(scanner));
	} | 
	LITERAL_STRING
	{
	    $$ = (anna_node_t *)anna_yacc_string_literal_create(
		&@$, anna_lex_get_text(scanner));
	};

opt_block: /* Empty */{$$ = 0;} | block;

function_declaration: 
	DEF opt_type_and_opt_name declaration_list opt_declaration_init
	{
	    if($2->child[0]->node_type == ANNA_NODE_NULL)
	    {
		anna_error((anna_node_t *)$2, L"missing return type");
	    }
	    if($2->child[1]->node_type == ANNA_NODE_NULL)
	    {
		anna_error((anna_node_t *)$2, L"missing declaration name");
	    }
	    
	    anna_node_call_t *attr = anna_node_create_block2(&@$);
	    if($4)
	    {
		anna_node_call_add_child(
		    attr,
		    (anna_node_t *)anna_node_create_call2(
			&$4->location,
			anna_node_create_identifier(
			    &$4->location,
			    L"default"),
			anna_node_clone_deep($4)));
	    }
	    	    
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		anna_node_create_identifier(&@1,L"__var__"),
		$2->child[1],
		anna_node_create_call2(
		    &@$,
		    anna_node_create_identifier(
			&@$,
			L"__staticTypeOf__"),
		    anna_node_create_call2(
			&@$,
			anna_node_create_identifier(&@1,L"__def__"),
			$2->child[1], $2->child[0],
			$3, anna_node_create_block2(&@$),
			anna_node_create_block2(&@$))), 
		anna_node_create_null(&@$), 
		attr);
	};

function_signature: 
	DEF opt_type_and_opt_name declaration_list
	{
	    if($2->child[0]->node_type == ANNA_NODE_NULL)
	    {
		anna_error((anna_node_t *)$2, L"missing return type");
	    }
	    $$ = (anna_node_t *)anna_node_create_call2(
		    &@$,
		    anna_node_create_identifier(&@1,L"__def__"),
		    anna_node_create_identifier(&@$,L"!anonymous"), $2->child[0],
		    $3, anna_node_create_block2(&@$),
		    anna_node_create_block2(&@$));
	};

opt_type_and_opt_name: 
    type_remainder '.' any_identifier opt_specialization opt_identifier
    {
	anna_node_t *type=(anna_node_t *)anna_node_create_call2(
	    &@$,
	    anna_node_create_identifier(&@2, L"__memberGet__"), 
	    $1, $3);

	if($4)
	{
	    type = (anna_node_t *)anna_node_create_call2(
		&@$, anna_node_create_identifier(&@$, L"__specialize__"), 
		type, $4);
	}

	$$ = anna_node_create_block2(
	    &@$, 
	    type, $5?$5:(anna_node_t *)anna_node_create_null(&@$));
    } |
    function_signature opt_identifier
    {	
	$$ = anna_node_create_block2(
	    &@$, 
	    $1, $2?$2:anna_node_create_null(&@$));
    } |
    identifier
    {
	$$ = anna_node_create_block2(
	    &@$, 
	    anna_node_create_null(&@$), $1);
    } |
    type_identifier opt_specialization opt_identifier
    {
	anna_node_t *t = $1;
	if($2)
	{
	    t = (anna_node_t *)anna_node_create_call2(
		&@$, anna_node_create_identifier(&@$, L"__specialize__"), 
		t, $2);
	}

	$$ = anna_node_create_block2(
	    &@$, 
	    t, $3?$3:anna_node_create_null(&@$));
    } |
    /* Empty */
    {
	$$ = anna_node_create_block2(
	    &@$, 
	    anna_node_create_null(&@$), 
	    anna_node_create_null(&@$));
    };

type_remainder:
    any_identifier |
    type_remainder '.' any_identifier
    {
	$$ = (anna_node_t *)anna_node_create_call2(
	    &@$,
	    anna_node_create_identifier(&@2, L"__memberGet__"), 
	    $1, $3);
    };


function_definition: 
	DEF opt_type_and_opt_name declaration_list attribute_list opt_block
	{
	    int anon = $2->child[1]->node_type == ANNA_NODE_NULL;
	    anna_node_t *def = (anna_node_t *)anna_node_create_call2(
		&@$,
		anna_node_create_identifier(&@1,L"__def__"), 
		anon?(anna_node_t *)anna_node_create_identifier(&@$,L"!anonymous"):$2->child[1],
		$2->child[0],
		$3, $4, $5?$5:anna_node_create_block2(&@$));
	    
	    if(anon)
	    {
		$$ = def;		
	    }
	    else
	    {
		if($5)
		{
		    $$ = (anna_node_t *)anna_node_create_call2(
			&@$, anna_node_create_identifier(&@1,L"__const__"),
			$2->child[1], anna_node_create_null(&@$), 
			def, anna_node_clone_deep((anna_node_t *)$4));
		}
		else
		{
		    $$ = (anna_node_t *)anna_node_create_call2(
			&@$, anna_node_create_identifier(&@1,L"__const__"),
			$2->child[1], 
			anna_node_create_call2(
			    &@$,
			    anna_node_create_identifier(
				&@$,
				L"__staticTypeOf__"),
			    def),
			anna_node_create_null(&@$),
			anna_node_clone_deep((anna_node_t *)$4));
		}
	    }
	} |
	MACRO identifier '(' identifier ')' attribute_list block
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		anna_node_create_identifier(&@1,L"__macro__"), $2, $4, $6, $7);
	};

declaration_list :
	'(' opt_separator ')'
	{
	    $$ = anna_node_create_block2(&@$);
	} |
	'(' declaration_list2 opt_separator ')'
	{
	    $$ = $2;
	};

declaration_list2 :
	declaration_list_item
	{
	    $$ = anna_node_create_block2(&@$);
	    anna_node_call_add_child($$,$1);
	} | 
	declaration_list2 separators declaration_list_item
	{
	    $$ = $1;
	    anna_node_call_add_child($1,$3);
	};

var_or_const:
	VAR
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$, L"__var__");
	} |
	CONST
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$, L"__const__");
	};

declaration_expression: 
	var_or_const opt_type_and_opt_name attribute_list opt_declaration_init
	{
	    if($2->child[1]->node_type == ANNA_NODE_NULL)
	    {
		anna_error((anna_node_t *)$2, L"missing declaration name");
	    }
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$, $1,
		$2->child[1], $2->child[0], $4?$4:anna_node_create_null(&@$), $3);
	} |
	function_definition |
	expression2 DECLARE_VAR expression
        {
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$, anna_node_create_identifier(&@$, L"__var__"),
		$1, anna_node_create_null(&@$), $3, anna_node_create_block2(&@$));
        } |
	expression2 DECLARE_CONST expression
        {
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$, anna_node_create_identifier(&@$, L"__const__"),
		$1, anna_node_create_null(&@$), $3, anna_node_create_block2(&@$));
        };

opt_ellipsis:
	/* Empty */
	{
	    $$=0;
	} |
	ELLIPSIS
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$, L"variadic");
	} |
	TO
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$, L"variadicNamed");
	};

type_and_name:
    type_remainder opt_specialization identifier
    {
	anna_node_t *type=(anna_node_t *)$1;

	if($2)
	{
	    type = (anna_node_t *)anna_node_create_call2(
		&@$, anna_node_create_identifier(&@$, L"__specialize__"), 
		type, $2);
	}

	$$ = anna_node_create_block2(
	    &@$, 
	    type, $3);
    } |
    function_signature identifier
    {
	$$ = anna_node_create_block2(
	    &@$, 
	    $1, $2);	
    };

variable_declaration:
	type_and_name opt_ellipsis attribute_list opt_declaration_init
	{
	    if($2)
	    {
		anna_node_call_add_child($3, $2);
	    }
	    
	    if($4)
	    {
		anna_node_call_add_child(
		    $3,
		    (anna_node_t *)anna_node_create_call2(
			&$4->location,
			anna_node_create_identifier(
			    &$4->location,
			    L"default"),
			anna_node_clone_deep($4)));
	    }
	    
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$, anna_node_create_identifier(&@$, L"__var__"),
		$1->child[1], $1->child[0], $4?$4:anna_node_create_null(&@$), $3);
	};

declaration_list_item: 
	variable_declaration
	| function_declaration;


opt_specialization:
	{
	    $$=0;
	} | 
	specialization;

specialization:
	SPECIALIZATION_BEGIN2 expression_list opt_separator SPECIALIZATION_END2
	{
	    $$ = $2;
	} |
	SPECIALIZATION_BEGIN expression_list opt_separator SPECIALIZATION_END
	{
	    $$ = $2;
	};

type_definition:
	identifier type_identifier2 attribute_list block 
	{
	    $1 = (anna_node_t *)identifier_enclose(L"", (anna_node_identifier_t *)$1, L"Type");
	    
	    anna_node_t *type  = (anna_node_t *)anna_node_create_call2(
		&@$,
		$1,
		$2?$2:(anna_node_t *)anna_node_create_identifier(&@$, L"!anonymous"),
		$3,$4);
	    
	    if($2)
	    {
		$$ = (anna_node_t *)anna_node_create_call2(
		    &@$, anna_node_create_identifier(&@1,L"__const__"),
		    $2?$2:(anna_node_t *)anna_node_create_identifier(&@$, L"!anonymous"),
		    anna_node_create_null(&@$), type, 
		    anna_node_clone_deep((anna_node_t *)$3));
	    }
	    else
	    {
		$$ = (anna_node_t *)type;
	    }
	};

attribute_list:
	/* Empty */
	{
	    $$ = anna_node_create_block2(&@$);
	} | 
	'(' opt_expression_list ')'
	{
	    $$ = $2;
	};

%%

void anna_yacc_init()
{
    anna_yacc_do_init = 1;    
    static int first_init = 1;
    if(first_init)
    {
	first_init = 0;
	sb_init(&anna_yacc_str_buff);
    }
}

/**
   Internal helper function for anna_yacc_lex, never ever call directly.

   Reads the input stream from the lexer, keep track of line and column
   number, and ignore any tokens of type IGNORE.
 */
static int anna_yacc_lex_inner (
    YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner, wchar_t *filename)
{
    if(anna_yacc_do_init)
    {
	anna_yacc_do_init = 0;
	llocp->first_line= llocp->last_line=1;
	llocp->first_column = llocp->last_column=0;
    }
    
    while(1)
    {
	char *p;
		
	yylex_val = anna_lex_lex(lvalp, scanner);
	llocp->filename = filename;
	llocp->first_line= llocp->last_line;
	llocp->first_column = llocp->last_column;
	for(p=anna_lex_get_text(scanner); *p; p++)
	{
	    if(*p == '\n')
	    {
		llocp->last_line++;
		llocp->last_column = 0;
	    }
	    else
	    {
		llocp->last_column++;
	    }
	}
	if(yylex_val != IGNORE)
	    break;
    }
    
    return yylex_val;
}

/**
   Returns the next token for the parser. Ignores IGNORE and
   LINE_BREAK tokens. Adds implicit separators after brace/LINE_BREAK
   combos. Keeps track of file location.
 */
int anna_yacc_lex (
    YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner, wchar_t *filename)
{
    static int was_injected_separator = 0;
    static int real_val = 0;
    
    if(was_injected_separator)
    {
	was_injected_separator = 0;
	return real_val;
    }
    
    int val = anna_yacc_lex_inner(lvalp, llocp, scanner, filename);

    /*
      Line breaks directly following an end brace are implicitly
      interpreted as an expression separator.

      Otherwise we'd need a semi-colon after any block call, e.g.

      if(foo){bar};

      Which, while logical and consistent, is rather non-C and avoided
      to avoid surprises.

      After any other token than an end brace, line breaks are a noop.
     */
    if(val == LINE_BREAK || val == 0){
	if(was_end_brace)
	{
	    was_end_brace = 0;
	    return SEPARATOR;
	}
	else
	{
	    return val==LINE_BREAK?anna_yacc_lex(lvalp, llocp, scanner, filename):0;
	}
    }
    if(was_end_brace && ((val == IDENTIFIER) || (val == TYPE_IDENTIFIER) || (val == '%')))
    {
	was_end_brace = 0;
	was_injected_separator = 1;
	real_val = val;
	return SEPARATOR;
    }    
    
    was_end_brace = (val == '}');
    return val;
}

void anna_yacc_error (
    YYLTYPE *llocp, yyscan_t scanner, 
    wchar_t *filename, anna_node_t **parse_tree_ptr, char *s) 
{
    anna_message(
	L"Error in %ls, on line %d:\n", 
	llocp->filename,
	llocp->first_line);
    anna_node_print_code((anna_node_t *)anna_node_create_dummy(llocp, 0));
    
    anna_message(L"%s\n", s);
    anna_yacc_error_count++;
}
