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

#include "common.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_lex.h"
#include "anna_yacc.h"

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

static wchar_t *enclose(wchar_t *in)
{
    string_buffer_t sb;
    sb_init(&sb);
    sb_append(&sb,L"__");
    sb_append(&sb,in);
    sb_append(&sb,L"__");
    return sb_content(&sb);
}

static wchar_t *anna_yacc_string(char *in)
{
    sb_clear(&anna_yacc_str_buff);
    sb_printf(&anna_yacc_str_buff, L"%s", in);
    return sb_content(&anna_yacc_str_buff);
    
}

static anna_node_t *anna_yacc_string_literal_create(anna_location_t *loc, char *str)
{
    str++;
    str[strlen(str)-1]=0;
    wchar_t *str2 = str2wcs(str);
    wchar_t *str3 = malloc(sizeof(wchar_t)*(wcslen(str2)));
    wchar_t *ptr_in;
    wchar_t *ptr_out = str3;
    
    for(ptr_in=str2; 
	*ptr_in; 
	ptr_in++)
    {

	/*
	  FIXME: We're not handling hex escape sequences.
	 */

	switch(*ptr_in)
	{
	    case L'\\':
		ptr_in++;
		switch(*ptr_in)
		{
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
			
		    case L'\0':
			wprintf(L"Error in string.");
			exit(1);
			break;
			
		    default:
			*ptr_out++ = *ptr_in;
			break;
		}
		break;
		
	    default:
		*ptr_out++ = *ptr_in;
		break;		
	}
    }
    
    free(str2);
    
    return (anna_node_t *)anna_node_create_string_literal(loc, ptr_out-str3, str3);
}

 
static anna_node_t *anna_yacc_char_literal_create(anna_location_t *loc, char *str)
{
    str++;
    str[strlen(str)-1]=0;
    wchar_t *str2 = str2wcs(str);
    wchar_t *str3 = str2;
    wchar_t chr;

    switch(*str3)
    {
	case L'\\':
	    str3++;
	    switch(*str3)
	    {
		case L'n':
		    chr= L'\n';
		    break;
		    
		case L'r':
		    chr = L'\r';
		    break;
		    
		case L'e':
		    chr = L'\x1b';
		    break;
		    
		case L't':
		    chr = L'\t';
		    break;
			
		case L'0':
		    chr = L'\0';
		    break;
			
		case L'\0':
		    wprintf(L"Error in string.");
		    exit(1);
		    break;
			
		default:
		    chr = *str3;
		    break;
	    }
	    break;
	    
	default:
	    chr = *str3;
	    break;
    }

    if(*(str3+1)){
	anna_error(
	    (anna_node_t *)anna_node_create_dummy(loc, 0),
	    L"Invalid character literal");
    }
    free(str2);
    
    return (anna_node_t *)anna_node_create_char_literal(loc, chr);
}

 
%}

%error-verbose

%union {
    anna_node_t *node_val;
    anna_node_identifier_t *identifier_val;
    anna_node_call_t *call_val;
}

%locations

%token LITERAL_INTEGER
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
%token RETURN
%token SEMICOLON
%token SIGN
%token IGNORE
%token LINE_BREAK
%token BITAND
%token BITOR
%token XOR
%token BITNOT
%token MODULO
%token AS
%token IN
%token ELLIPSIS
%token IF
%token ELSE
%token TO
%token SPECIALIZE_BEGIN2
%token SPECIALIZATION_BEGIN
%token SPECIALIZATION_END

%type <call_val> block block2 block3 opt_else
%type <call_val> module
%type <node_val> expression expression1 expression2 expression3 expression4 expression5 expression6 expression7 expression8 expression9 expression10 
%type <node_val> constant
%type <node_val> opt_declaration_init opt_declaration_expression_init
%type <node_val> function_definition 
%type <node_val> function_declaration 
%type <node_val> opt_identifier identifier type_identifier any_identifier op op1 op3 op4 op5 op6 op7 pre_op8 post_op8
%type <call_val> argument_list argument_list2 
%type <node_val> type_definition 
%type <call_val> declaration_list declaration_list2
%type <node_val> declaration_list_item declaration_expression variable_declaration
%type <call_val> attribute_list attribute_list2
%type <call_val> opt_block
%type <call_val> specialization specialization2 opt_specialization
%type <node_val> opt_templatized_type  templatized_type 

%right '='

%pure-parser

%%

module:
	module expression SEMICOLON
	{	    
	    $$ = $1;
	    if($1 && $2)
	    {
		anna_node_call_add_child($1,$2);
		anna_node_set_location((anna_node_t *)$$, &@$);
	    }
	}
	| 
	expression SEMICOLON
	{
	    $$ = anna_node_create_call2(
		&@$, 
		(anna_node_t *)anna_node_create_identifier(
		    &@$,L"__module__"));
	    if ($1)
		anna_node_call_add_child(
		    $$,
		    $1);
	    *parse_tree_ptr = (anna_node_t *)$$;
	}
|
error SEMICOLON
{
  yyerrok;
  $$ = 0;
  
}
;

block: '{' block2 '}'
	{
		$$ = $2;
	}
;

block2 : /* Empty */ 
	{
	    $$ = anna_node_create_block2(&@$);
	}
	| 
	block3 opt_semicolon
;

block3 :
	block3 SEMICOLON expression
	{
	    $$ = $1;
	    anna_node_call_add_child($1,$3);
	    anna_node_set_location((anna_node_t *)$$, &@$);
	}
	| 
	expression
	{
	    $$ = anna_node_create_block2(&@$, $1);
	}
;

opt_declaration_init :
	'=' expression
	{
	    $$ = $2;
	}
	|
	{
	    $$ = 0;
	}
	|
	ELLIPSIS
	{
		$$ = (anna_node_t *)anna_node_create_identifier(&@$, L"__variadic__");
	}
;

opt_declaration_expression_init :
	'=' expression
	{
	    $$ = $2;
	}
	|
	{
	    $$ = 0;
	}
;

opt_else: 
{
    $$=anna_node_create_block2(&@$);
}
|
ELSE block
{
    $$ = $2;
}
;


expression:
	expression1 '=' expression
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		(anna_node_t *)anna_node_create_identifier(&@$,L"__assign__"),
		$1, $3);
	}
	|
	expression1 op expression
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		$2,
		$1, $3);
	}
        | expression1
	| declaration_expression 
	| type_definition
	| RETURN expression
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		(anna_node_t *)anna_node_create_identifier(&@1,L"return"),
		$2);
	}
	| RETURN
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
	      &@$, 
	      (anna_node_t *)anna_node_create_identifier(
		  &@$,
		  L"return"), 
	      anna_node_create_null(&@$));  
	}
;

expression1 :
	expression1 op1 expression2
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		$2,
		$1, $3);
	}
        | 
	expression2
;


opt_semicolon: | SEMICOLON;

expression2 :
	expression3
	;

expression3 :
	expression3 op3 expression4
	{
	    if(anna_node_is_named($2, L"__in__"))
	    {
		$$ = (anna_node_t *)
		    anna_node_create_call2(
			&@$, 
			(anna_node_t *)anna_node_create_call2(
			    &@$, 
			    (anna_node_t *)anna_node_create_identifier(
				&@$,
				L"__memberGet__"),
			    $3, $2),
			$1);
	    }
	    else 
	    {
		$$ = (anna_node_t *)
		    anna_node_create_call2(
			&@$, 
			(anna_node_t *)anna_node_create_call2(
			    &@$, 
			    (anna_node_t *)anna_node_create_identifier(
				&@$,
				L"__memberGet__"),
			    $1, $2),
			$3);
	    }	    
	}
        |
	expression4
	;

expression4 :
	expression4 op4 expression5
	{
	    anna_node_t *enc = (anna_node_t *)anna_node_create_identifier(
		&$2->location, 
		enclose(((anna_node_identifier_t *)$2)->name));
	    
	    $$ = (anna_node_t *)
		anna_node_create_call2(
		    &@$, 
		    (anna_node_t *)anna_node_create_call2(
			&@$, 
			(anna_node_t *)anna_node_create_identifier(
			    &@$,
			    L"__memberGet__"),
			$1, enc),
		    $3);
	}
        | 
	expression4 RANGE expression5
	{
	    anna_node_t *op = (anna_node_t *)anna_node_create_identifier(
		&@$,L"__range__");
	    $$ = (anna_node_t *)anna_node_create_call2(&@$, op, $1, $3);
	}
	|
	expression4 ELLIPSIS
	{
	    anna_node_t *op = (anna_node_t *)anna_node_create_identifier(
		&@$,L"__range__");
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$, op, $1, anna_node_create_null(&@$));	    
	}
	|
	expression5
	;

expression5 :
	expression5 op5 expression6
	{
	    $$ = (anna_node_t *)anna_node_create_call2(&@$, $2, $1, $3);
	}
        | 
	expression6
	;

expression6 :
	expression6 op6 expression7
	{
	    $$ = (anna_node_t *)
		anna_node_create_call2(
		    &@$, 
		    (anna_node_t *)anna_node_create_call2(
			&@$, 
			(anna_node_t *)anna_node_create_identifier(
			    &@$,
			    L"__memberGet__"),
			$1, $2),
		    $3);
	}
        | 
	expression7
	;

expression7 :
	expression7 op7 expression8
	{
	    $$ = (anna_node_t *)
		anna_node_create_call2(
		    &@$, 
		    (anna_node_t *)anna_node_create_call2(
			&@$, 
			(anna_node_t *)anna_node_create_identifier(
			    &@$,
			    L"__memberGet__"),
			$1, $2),
		    $3);
	}
        | 
	expression8
	;

expression8 :
	'!' expression9
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		(anna_node_t *)anna_node_create_identifier(&@$,L"__not__"), 
		$2);
	}
	|
	'^' identifier expression9
	{
	    anna_node_identifier_t *id = (anna_node_identifier_t *)$2;
	    $$ = (anna_node_t *)
		anna_node_create_call(
		    &@$, 
		    (anna_node_t *)anna_node_create_call2(
			&@$, 
			(anna_node_t *)anna_node_create_identifier(
			    &@2, 
			    L"__memberGet__"),
			$3, 
			(anna_node_t *)anna_node_create_identifier(
			    &id->location,enclose(id->name))), 0, 0);
	}
	|
	pre_op8 expression9
	{
	  $$ = (anna_node_t *)
	    anna_node_create_call2(
		&@$, 
		(anna_node_t *)anna_node_create_call2(
		    &@$, 
		    (anna_node_t *)anna_node_create_identifier(&@2, L"__memberGet__"),
		    $2, $1));
	}
	| 
	expression9 post_op8
	{
	  $$ = (anna_node_t *)
	      anna_node_create_call2(
		  &@$, 
		  (anna_node_t *)$2,
		  $1);
	}
	| 
	expression9
;

expression9 :
	expression9 '.' any_identifier
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		(anna_node_t *)anna_node_create_identifier(
		    &@2,
		    L"__memberGet__"), 
		$1, $3);
	}
        |
	expression9 opt_specialization '(' argument_list ')' opt_block
	{
	    $$ = (anna_node_t *)$4;
	    anna_node_t *fun = $1;
	    if($2)
	    {
		fun = (anna_node_t *)anna_node_create_call2(
		    &@$,(anna_node_t *)anna_node_create_identifier(
			&@$,
			L"__specialize__"), 
		    fun, (anna_node_t *)$2);
	    }
	    
	    anna_node_call_set_function($4, fun);
	    anna_node_set_location($$, &@$);
	    if ($6) 
		anna_node_call_add_child($4, (anna_node_t *)$6);
	}
	| 
	expression10 block
	{
	    $$ = (anna_node_t *)anna_node_create_call2(&@$, $1, $2);
	}
	| 
	expression9 '[' expression ']'
	{
	    $$ = (anna_node_t *)
		anna_node_create_call2(
		    &@$, 
		    (anna_node_t *)anna_node_create_call2(
			&@$, 
			(anna_node_t *)anna_node_create_identifier(
			    &@$,
			    L"__memberGet__"),
			$1, 
			(anna_node_t *)anna_node_create_identifier(
			    &@$,
			    L"__get__")),
		    $3);

	}
	| 
	'[' argument_list ']' /* Alternative list constructor syntax */
	{	    
	    $$ = (anna_node_t *)$2;
	    anna_node_call_set_function(
		$2,
		(anna_node_t *)anna_node_create_identifier(
		    &@$,
		    L"__collection__"));
	}
	|
	expression9 AS expression10
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$, 
		(anna_node_t *)anna_node_create_identifier(&@2,L"__as__"), 
		$1, $3);
	}
	|
	expression10;
;

expression10:
	constant
	| 
	any_identifier 
	{
	    $$ = $1;	    
	}
	| 
	'(' expression ')'
	{
	    $$ = $2;
	}
	|
	NULL_SYM
	{
	    $$ = anna_node_create_null(&@$);
	}
	| block 
	{
	    $$ = (anna_node_t *)$1; 
	}
        |
	IF '(' expression ')' block opt_else
        {
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		(anna_node_t *)anna_node_create_identifier(&@$,L"__if__"),
		$3, (anna_node_t *)$5, (anna_node_t *)$6);
        }
;

op:
	APPEND
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(
		&@$,L"__append__");
	}
	|
	INCREASE
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(
		&@$,L"__increase__");
	}
	|
	DECREASE
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(
		&@$,L"__decrease__");
	}
	|
	TO
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(
		&@$,L"__mapping__");
	}
;


op1:
	AND
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(
		&@$,L"__and__");
	}
	|
	OR
	{
		$$ = (anna_node_t *)anna_node_create_identifier(
		    &@$,L"__or__");
	}
;

op3:
	'<'
	{
		$$ = (anna_node_t *)anna_node_create_identifier(
		    &@$,L"__lt__");
	}
	|
	'>'
	{
		$$ = (anna_node_t *)anna_node_create_identifier(
		    &@$,L"__gt__");
	}
	|
	EQUAL
	{
		$$ = (anna_node_t *)anna_node_create_identifier(
		    &@$,L"__eq__");
	}
	|
	NOT_EQUAL
	{
		$$ = (anna_node_t *)anna_node_create_identifier(
		    &@$,L"__neq__");
	}
	|
	LESS_OR_EQUAL
	{
		$$ = (anna_node_t *)anna_node_create_identifier(
		    &@$,L"__lte__");
	}
	|
	GREATER_OR_EQUAL
	{
		$$ = (anna_node_t *)anna_node_create_identifier(
		    &@$,L"__gte__");
	}
        |
	IN
	{
		$$ = (anna_node_t *)anna_node_create_identifier(
		    &@$,L"__in__");
	}
;

op4: 
	'^' identifier
	{
	    $$ = $2
	}
;


op5:
	':'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(
		&@$,L"Pair");
	}
;



op6:
	'+'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__add__");
	}
	|
	'-'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__sub__");
	}
	|
	'~'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__join__");
	}
;

op7:
	'*'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__mul__");
	}
	|
	'/'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__div__");
	}
	|
	'%'
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__format__");
	}
;

pre_op8:
	'-'
	{
		$$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__neg__")
	}
;

post_op8:
	NEXT
	{
		$$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__next__")
	}
	|
	PREV
	{
		$$ = (anna_node_t *)anna_node_create_identifier(&@$,L"__prev__")
	}
;

opt_identifier:
identifier
|
{
    $$ = (anna_node_t *)anna_node_create_null(&@$);
}
;


identifier:
	IDENTIFIER
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,anna_yacc_string(anna_lex_get_text(scanner)));
	}
;

type_identifier :
	TYPE_IDENTIFIER
	{
	    $$ = (anna_node_t *)anna_node_create_identifier(&@$,anna_yacc_string(anna_lex_get_text(scanner)));
	}
;

any_identifier:
	identifier
	|
	type_identifier
;



constant :
	LITERAL_INTEGER
	{
	    $$ = (anna_node_t *)anna_node_create_int_literal(
		&@$,
		atoi(anna_lex_get_text(scanner)));
	}
	| 
	LITERAL_FLOAT
	{
	    $$ = (anna_node_t *)anna_node_create_float_literal(
		&@$,
		atof(anna_lex_get_text(scanner)));
	}
	| 
	LITERAL_CHAR
	{
	    $$ = anna_yacc_char_literal_create(&@$, anna_lex_get_text(scanner));
	}
	| 
	LITERAL_STRING
	{
	    $$ = (anna_node_t *)anna_yacc_string_literal_create(&@$, anna_lex_get_text(scanner));
	}
	;


opt_block:
	/* Empty */
	{
		$$ = 0;
	}
	|
	block 
	;

argument_list:
	{
	    $$ = anna_node_create_block2(&@$);
	}
	|
	argument_list2 opt_semicolon
;

argument_list2 :
	expression 
	{
	    $$ = anna_node_create_block2(&@$, $1);
	}
	| 
	argument_list2 ',' expression
	{
	    $$ = $1;
	    anna_node_call_add_child($$, (anna_node_t *)$3);
	    anna_node_set_location((anna_node_t *)$$, &@$);
	}
	;

function_declaration: 
	DEF opt_templatized_type identifier declaration_list
	{
	    anna_node_t *param[] ={
		$3,
		(anna_node_t *)($2?$2:anna_node_create_null(&@$)),
		(anna_node_t *)$4, 
		(anna_node_t *)anna_node_create_block2(&@$),
		(anna_node_t *)anna_node_create_block2(&@$)
	    };
	    
	    anna_node_t *param2[] ={$3, (anna_node_t *)anna_node_create_null(&@$), 0, (anna_node_t *)anna_node_create_block2(&@$)};
	    
	    param2[2] = (anna_node_t *)anna_node_create_call(
		&@$,
		(anna_node_t *)anna_node_create_identifier(&@1,L"__def__"),
		5,
		param);
	    $$ = (anna_node_t *)anna_node_create_call(
		&@$,
		(anna_node_t *)anna_node_create_identifier(&@1,L"__var__"),
		4,
		param2);
	}
;

function_definition: 
	DEF opt_templatized_type opt_identifier declaration_list attribute_list opt_block
	{
	    anna_node_t *param[] ={
		($3->node_type == ANNA_NODE_IDENTIFIER)?(anna_node_t *)$3:(anna_node_t *)anna_node_create_identifier(
		    &@$,
		    L"!anonymous"),
		$2?(anna_node_t *)$2:(anna_node_t *)anna_node_create_null(&@$),
		(anna_node_t *)$4, 
		(anna_node_t *)$5, 
		$6?(anna_node_t *)$6:(anna_node_t *)anna_node_create_block2(&@$)
	    };
	    anna_node_t *param2[] ={$3, (anna_node_t *)anna_node_create_null(&@$), 0, (anna_node_t *)anna_node_create_block2(&@$)};
	    
	    param2[2] = (anna_node_t *)anna_node_create_call(&@$,(anna_node_t *)anna_node_create_identifier(&@1,L"__def__"), 5, param);
	    if($3->node_type != ANNA_NODE_NULL)
	    {
		$$ = (anna_node_t *)anna_node_create_call(
		    &@$,
		    (anna_node_t *)anna_node_create_identifier(&@1,L"__const__"),
		    4,
		    param2);
	    }
	    else
	    {
		$$ = param2[2];
	    }
	}
	|
	MACRO identifier '(' identifier ')' block
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		anna_node_create_identifier(&@1,L"__macro__"), $2, $4, $6);
	}
;

declaration_list :
	'(' ')'
	{
	    $$ = anna_node_create_block2(&@$);
	}
	|
	'(' declaration_list2 ')'
	{
	    $$ = $2;
	}
	;

declaration_list2 :
	declaration_list_item
	{
	    $$ = anna_node_create_block2(&@$);
	    anna_node_call_add_child($$,$1);
	}
	| 
	declaration_list2 ',' declaration_list_item
	{
	    $$ = $1;
	    anna_node_call_add_child($1,$3);
	}
	;

declaration_expression: 
	VAR opt_templatized_type identifier attribute_list opt_declaration_expression_init
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		(anna_node_t *)anna_node_create_identifier(
		    &@$,L"__var__"),
		$3, $2, $5?$5:anna_node_create_null(&@$), $4);
	}
	|
	function_definition
	;

variable_declaration:
	templatized_type identifier attribute_list opt_declaration_init
	{
	    $$ = (anna_node_t *)anna_node_create_call2(
		&@$,
		(anna_node_t *)anna_node_create_identifier(
		    &@$,
		    L"__var__"),
		$2, $1, $4?$4:anna_node_create_null(&@$), $3);
	}
;

declaration_list_item : variable_declaration {$$=$1;} | function_declaration;

opt_templatized_type:
{
    $$=anna_node_create_null(&@$);
}
|
templatized_type;

templatized_type:
type_identifier opt_specialization
{
   if(!$2)
   {
      $$=$1;
   }
   else 
   {
      $$ = (anna_node_t *)anna_node_create_call2(
	  &@$,(anna_node_t *)anna_node_create_identifier(
	      &@$,
	      L"__specialize__"), 
	  $1, (anna_node_t *)$2);      
   }
}
;

opt_specialization:
{
   $$=0;
}
|
specialization;

specialization:
SPECIALIZE_BEGIN2 specialization2 ')'
{
  $$ = $2;
}
|
SPECIALIZATION_BEGIN specialization2 SPECIALIZATION_END
{
  $$ = $2;
}
;

specialization2:
	expression
	{
	    $$ = anna_node_create_block2(&@$,$1);
	}
	|
	specialization2 ',' expression
	{
	  anna_node_call_add_child($1,$3);
	  $$ = $1;
	}
;

type_definition :
	identifier type_identifier attribute_list block 
	{
	    
	  anna_node_t *type  = (anna_node_t *)anna_node_create_call2(
	      &@$,
	      $1,$2,$3,$4);
	  
	  $$ = (anna_node_t *)anna_node_create_call2(
	      &@$,
	      (anna_node_t *)anna_node_create_identifier(&@1,L"__const__"),
	      $2, anna_node_create_null(&@$), type, anna_node_clone_deep((anna_node_t *)$3));
	}
	;

attribute_list :
	/* Empty */
	{
	    $$ = anna_node_create_block2(&@$);
	}
	| 
	'(' attribute_list2 ')'
	{
	    $$ = $2;
	}
;

attribute_list2 :
	attribute_list2 ',' expression
	{
	    $$ = $1;
	    anna_node_call_add_child($$,(anna_node_t *)$3);
	}
	|
	expression
	{
	    $$ = anna_node_create_block2(&@$, $1);
	}
;

%%

void anna_yacc_init()
{
    anna_yacc_do_init = 1;    
    sb_init(&anna_yacc_str_buff);
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
		
	yylex_val = anna_lex_lex(scanner);
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
   LINE_BREAK tokens. Adds implicit semicolons after brace/LINE_BREAK
   combos. Keeps track of file location.
 */
int anna_yacc_lex (
    YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner, wchar_t *filename)
{
    
    int val = anna_yacc_lex_inner(lvalp, llocp, scanner, filename);

    /*
      Line breaks directly following an end brace are implicitly
      interpreted as a semicolon.

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
	    return SEMICOLON;
	}
	else
	{
	    return val==LINE_BREAK?anna_yacc_lex(lvalp, llocp, scanner, filename):0;
	}
    }
    
    was_end_brace = (val == '}');
    return val;
}

void anna_yacc_error (
    YYLTYPE *llocp, yyscan_t scanner, 
    wchar_t *filename, anna_node_t **parse_tree_ptr, char *s) 
{
    fwprintf(stderr,L"Error in %ls, on line %d:\n", 
	     llocp->filename,
	     llocp->first_line);
    anna_node_print_code((anna_node_t *)anna_node_create_dummy(llocp, 0));
    
    fwprintf (stderr, L"%s\n", s);
    anna_yacc_error_count++;
}
