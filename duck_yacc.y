%{

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <wchar.h>
#include <string.h>
#include "common.h"
#include "duck_node.h"

extern char *duck_text;

int yylex();
int duck_lex();
void yyerror (char *s) ;

duck_node_t *duck_parse_tree;
int was_end_brace=0;
int peek_token = -1;
size_t last_length = 0;


int yylex();

int yylex_val;
int duck_yacc_error=0; 

# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
    {									\
	(Current).filename = yylloc.filename;				\
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



static wchar_t *duck_yacc_string(char *in)
{
    /*
      FIXME: Add resource tracking and cleanup
     */   
    return str2wcs(in);
    
}

static duck_node_t *duck_yacc_string_literal_create(duck_location_t *loc, char *str)
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
			*ptr_out++ = L'\e';
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
    
    return (duck_node_t *)duck_node_string_literal_create(loc, ptr_out-str3, str3);
}

 
%}

%error-verbose

%union {
    duck_node_t *node_val;
    duck_node_lookup_t *lookup_val;
    duck_node_call_t *call_val;
}

%locations

%token LITERAL_INTEGER
%token LITERAL_FLOAT
%token LITERAL_CHAR
%token LITERAL_STRING
%token IDENTIFIER
%token TYPE_IDENTIFIER
%token APPEND
%token RANGE
%token FUNCTION
%token MACRO
%token NULL_SYM
%token EQUAL
%token NOT_EQUAL
%token GREATER_OR_EQUAL
%token LESS_OR_EQUAL
%token AND
%token OR
%token SHL
%token SHR
%token VAR
%token RETURN
%token SEMICOLON
%token SIGN
%token IGNORE

%type <call_val> block block2 block3
%type <call_val> module module1
%type <node_val> expression expression1 expression2 expression3 expression4 expression5 expression6 expression7 
%type <node_val> simple_expression
%type <node_val> constant
%type <node_val> opt_declaration_init
%type <node_val> function_definition 
%type <node_val> opt_simple_expression 
%type <node_val> opt_identifier identifier type_identifier any_identifier op op1 op2 op3 op4 op5 pre_op6
%type <call_val> argument_list argument_list2 argument_list3 
%type <node_val> type_definition 
%type <call_val> declaration_list declaration_list2
%type <node_val> declaration declaration_expression variable_declaration function_declaration
%type <call_val> attribute_list attribute_list2
%type <call_val> opt_block
%type <call_val> templatization templatization2
%type <node_val> opt_templatized_type  templatized_type 

%right '='

%%

module: module1 

module1:
	module expression SEMICOLON
	{
	    $$ = $1;
	    duck_node_call_add_child($1,$2);
	    duck_node_set_location((duck_node_t *)$$, &@$);
	}
	| 
	expression SEMICOLON
	{
	    $$ = duck_node_call_create(&@$, (duck_node_t *)duck_node_lookup_create(&@$,L"__block__"),0,0);
	    if ($1)
		duck_node_call_add_child($$,$1);
	    duck_parse_tree = (duck_node_t *)$$;
	}
|
error SEMICOLON
{
  yyerrok;  
}
;

block: '{' block2 '}'
	{
		$$ = $2;
	}
;

block2 : /* Empty */ 
	{
	    $$ = duck_node_call_create(&@$, (duck_node_t *)duck_node_lookup_create(&@$, L"__block__"),0,0);
	}
	| 
	block3 opt_semicolon
;

block3 :
	block3 SEMICOLON expression
	{
	    $$ = $1;
	    duck_node_call_add_child($1,$3);
	    duck_node_set_location((duck_node_t *)$$, &@$);
	}
	| 
	expression
	{
	    $$ = duck_node_call_create(&@$, (duck_node_t *)duck_node_lookup_create(&@$, L"__block__"), 0, 0);
	    duck_node_call_add_child($$,$1);
	}
;

opt_declaration_init :
	'=' expression
	{
	    $$ = $2;
	}
	|
	argument_list
	{
	    $$=(duck_node_t *)$1;
	}
        |
	{
	    $$ = 0;
	}
;

expression:
	expression1 '=' expression
	{
	    duck_node_t *param[] ={$1, $3};	    
	    $$ = (duck_node_t *)duck_node_call_create(&@$, (duck_node_t *)duck_node_lookup_create(&@2, L"__assign__"), 2, param);
	}
	|
	expression op expression1
	{
	    duck_node_t *param[] ={$1, $3};	    
	    $$ = (duck_node_t *)duck_node_call_create(&@$, $2, 2, param);
	}
        | expression1
        | function_definition
	| declaration_expression 
	| type_definition
	| RETURN expression1
	{
	    duck_node_t *param[] ={$2};	    
	    $$ = (duck_node_t *)duck_node_call_create(&@$, (duck_node_t *)duck_node_lookup_create(&@1, L"__return__"), 1, param);	  
	}
	| RETURN
	{
	  duck_node_t *param[] ={duck_node_null_create(&@$)};	    
	  $$ = (duck_node_t *)duck_node_call_create(&@$, (duck_node_t *)duck_node_lookup_create(&@$, L"__return__"), 1, param);	  
	}
	| RETURN '=' expression1
	{
	    duck_node_t *param[] ={$3};	    
	    $$ = (duck_node_t *)duck_node_call_create(&@$, (duck_node_t *)duck_node_lookup_create(&@2, L"__assignReturn__"), 1, param);	  
	}
;

expression1 :
	expression1 op1 expression2
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)duck_node_call_create(&@$, $2, 2, param);
	}
        | 
	expression2
;


opt_semicolon: | SEMICOLON;

expression2 :
	expression2 op2 expression3
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)duck_node_call_create(&@$, $2, 2, param);
	}
        | 
	expression3
	;

expression3 :
	expression3 op3 expression4
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)duck_node_call_create(&@$, $2, 2, param);
	}
        | 
	expression4
	;

expression4 :
	expression4 op4 expression5
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)duck_node_call_create(&@$, $2, 2, param);
	}
        | 
	expression5
	;

expression5 :
	expression5 op5 expression6
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)duck_node_call_create(&@$, $2, 2, param);
	}
        | 
	expression6
	;

expression6 :
	expression6 '.' expression7
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)duck_node_call_create(&@$, (duck_node_t *)duck_node_lookup_create(&@2, L"__memberGet__"), 2, param);
	}
        |
	expression6 '(' argument_list2 ')' opt_block
	{
	    $$ = (duck_node_t *)$3;
	    duck_node_call_set_function($3, $1);
	    duck_node_set_location($$, &@$);
	    if ($5) 
		duck_node_call_add_child($3, (duck_node_t *)$5);
	}
	| 
	expression6 block
	{
	    duck_node_t *param[] ={(duck_node_t *)$2};
	    $$ = (duck_node_t *)duck_node_call_create(&@$, $1, 1, param);
	}
	| 
	expression6 '[' expression ']'
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@2, L"__get__"), 2, param);
	}
	| 
	'[' argument_list2 ']' /* Alternative list constructor syntax */
	{	    
	    $$ = (duck_node_t *)$2;
	    duck_node_call_set_function($2, (duck_node_t *)duck_node_lookup_create(&@$,L"__list__"));
	}
	| 
	'|' argument_list2 '|' 
	{	    
	    $$ = (duck_node_t *)$2;
	    duck_node_call_set_function($2, (duck_node_t *)duck_node_lookup_create(&@$,L"__abs__"));
	}
	|
	pre_op6 expression7
	{
	    duck_node_t *param[] ={$2};   
	    $$ = (duck_node_t *)duck_node_call_create(&@$,$1, 1, param);
	}
	| 
	expression7
;


expression7 :
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
	    $$ = duck_node_null_create(&@$);
	}
	| block 
	{
	    $$ = (duck_node_t *)$1; 
	}
;

op:
	APPEND
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__append__");
	}
;


op1:
	AND
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__and__");
	}
	|
	OR
	{
		$$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__or__");
	}
;

op2:
	'<'
	{
		$$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__lt__");
	}
	|
	'>'
	{
		$$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__gt__");
	}
	|
	EQUAL
	{
		$$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__eq__");
	}
	|
	NOT_EQUAL
	{
		$$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__ne__");
	}
	|
	LESS_OR_EQUAL
	{
		$$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__le__");
	}
	|
	GREATER_OR_EQUAL
	{
		$$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__ge__");
	}
;

op3:
	'+'
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__add__");
	}
	|
	'-'
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__sub__");
	}
	|
	'~'
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__join__");
	}
	|
	':'
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"Pair");
	}
	|
	RANGE
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"Range");
	}
;

op4:
	'*'
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__mul__");
	}
	|
	'/'
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__div__");
	}
	|
	'%'
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__mod__");
	}
;

op5: '^'
{
    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__exp__");
}


pre_op6:
	'!'
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__not__");
	}
	|
	'-'
	{
		$$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__neg__")
	}
	|
	SIGN
	{
		$$ = (duck_node_t *)duck_node_lookup_create(&@$,L"__sign__")
	}
;

opt_identifier:
identifier
|
{
  $$=0;
}
;


identifier:
	IDENTIFIER
	{
	    $$ = (duck_node_t *)(duck_node_t *)duck_node_lookup_create(&@$,duck_yacc_string(duck_text));
	}
;


type_identifier :
	TYPE_IDENTIFIER
	{
	    $$ = (duck_node_t *)duck_node_lookup_create(&@$,duck_yacc_string(duck_text));
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
	    $$ = (duck_node_t *)(duck_node_t *)duck_node_int_literal_create(&@$,atoi(duck_text));
	}
	| 
	LITERAL_FLOAT
	{
	    $$ = (duck_node_t *)(duck_node_t *)duck_node_float_literal_create(&@$,atof(duck_text));
	}
	| 
	LITERAL_CHAR
	{
	    /*
	      FIXME: We're not handling escape sequences!
	     */
	    $$ = (duck_node_t *)(duck_node_t *)duck_node_char_literal_create(&@$,duck_text[1]);
	}
	| 
	LITERAL_STRING
	{
	    $$ = (duck_node_t *)duck_yacc_string_literal_create(&@$, duck_text);
	}
	;


argument_list :
	'(' argument_list2 ')' opt_block
	{
	    $$ = $2;
	    if ($4) 
		duck_node_call_add_child($$, (duck_node_t *)$4);
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

argument_list2:
	{
	    $$ = duck_node_call_create(&@$,0,0,0);
	}
	|
	argument_list3
;

argument_list3 :
	expression 
	{
	    $$ = duck_node_call_create(&@$,0, 0, 0);
	    duck_node_call_add_child($$, (duck_node_t *)$1);
	}
	| 
	argument_list3 ',' expression
	{
	    $$ = $1;
	    duck_node_call_add_child($$, (duck_node_t *)$3);
	    duck_node_set_location((duck_node_t *)$$, &@$);
	}
	;

function_definition: 
	FUNCTION opt_identifier declaration_list attribute_list block
	{
	    duck_node_t *param[] ={
		(duck_node_t *)($2?$2:duck_node_null_create(&@$)),
		duck_node_null_create(&@$), 
		(duck_node_t *)$3, 
		(duck_node_t *)$5, 
		(duck_node_t *)$4
	    };
	    $$ = (duck_node_t *)duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@1,L"__function__"), 5, param);
	  	  
	}
;

declaration_list :
	'(' ')'
	{
	    $$ = duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@$,L"__block__"),0,0);
	}
	|
	'(' declaration_list2 ')'
	{
	    $$ = $2;
	}
	;

declaration_list2 :
	declaration
	{
	    $$ = duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@$,L"__block__"), 0, 0);
	    duck_node_call_add_child($$,$1);
	}
	| 
	declaration_list2 ',' declaration
	{
	    $$ = $1;
	    duck_node_call_add_child($1,$3);
	}
	;

declaration_expression: 
	VAR variable_declaration
	{
	  $$=$2;
	}
	|
	function_declaration
	;

variable_declaration:
	opt_templatized_type identifier opt_declaration_init
	{
	    duck_node_t *param[] ={$2, $1, 0};	    
 	    param[2] = $3?$3:duck_node_null_create(&@$);
	    $$ = (duck_node_t *)duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@$,L"__declare__"), 3, param);    
	}
;

declaration : opt_var variable_declaration {$$=$2;} | function_declaration;

opt_var : | VAR;


function_declaration :
	FUNCTION templatized_type identifier declaration_list
	{
	  duck_node_t *param[] ={$3, $2, (duck_node_t *)$4, duck_node_null_create(&@$), duck_node_null_create(&@$)};	    
	  $$ = (duck_node_t *)duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@1,L"__function__"), 5, param);
	}
;

opt_templatized_type:
{
    $$=duck_node_null_create(&@$);
}
|
templatized_type;

templatized_type:
type_identifier 
| type_identifier templatization
{
  duck_node_t *param[] ={$1, (duck_node_t *)$2};	    
  $$ = (duck_node_t *)duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@$,L"__templatize__"), 2, param);
}

templatization:
'<' templatization2 '>'
{
  $$ = $2;
}
;

templatization2:
	simple_expression
	{
	    duck_node_t *param[] ={$1};	    
	    $$ = duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@$,L"__block__"), 1, param);
	}
	|
	templatization2 ',' simple_expression
	{
	  duck_node_call_add_child($1,$3);
	  $$ = $1;
	}
;

type_definition :
	identifier type_identifier attribute_list block 
	{
	  duck_node_t *param[] ={$2, $1, (duck_node_t *)$3, (duck_node_t *)$4};	    
	  $$ = (duck_node_t *)duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@$,L"__type__"), 4, param);
	}
	;

attribute_list :
	/* Empty */
	{
	    $$ = duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@$,L"__block__"),0,0);
	}
	| 
	attribute_list2
;

attribute_list2 :
	attribute_list ',' identifier opt_simple_expression
	{
	    $$ = $1;
	    duck_node_call_t *attr = duck_node_call_create(&@$,$3, 0, 0);
	    if($4)
	      duck_node_call_add_child(attr,$4);
	    duck_node_call_add_child($$,(duck_node_t *)attr);
	}
	|
	identifier opt_simple_expression
	{
	    $$ = duck_node_call_create(&@$,(duck_node_t *)duck_node_lookup_create(&@$,L"__block__"), 0, 0);
	    duck_node_call_t *attr = duck_node_call_create(&@$,$1, 0, 0);
	    if($2)
	      duck_node_call_add_child(attr,$2);
	    duck_node_call_add_child($$,(duck_node_t *)attr);
	}
;

opt_simple_expression: 
{
	$$ = 0;
}
|
simple_expression;

simple_expression:
any_identifier
{
  $$ = $1;
}
|
simple_expression '(' argument_list2 ')'
{
  $$ = (duck_node_t *)$3;
  duck_node_call_set_function($3, $1);
}
|
'(' expression ')'
{
  $$ = $2;
}
;

%%

int yylex ()
{
    if(peek_token != -1)
    {
	int ret = peek_token;
	peek_token = -1;
	return ret;      
    }

    while(1)
    {
	char *p;
	yylex_val = duck_lex();
	yylloc.first_line= yylloc.last_line;
	yylloc.first_column = yylloc.last_column;
	for(p=duck_text; *p; p++)
	{
	    if(*p == '\n')
	    {
		yylloc.last_line++;
		yylloc.last_column = 0;
	    }
	    else
	    {
		yylloc.last_column++;
	    }
	}
	if(yylex_val != IGNORE)
	    break;
    }
    
    if(was_end_brace)
    {
	was_end_brace = 0;
	if(yylex_val != '.' &&
	   yylex_val != SEMICOLON)
	{
	    peek_token = yylex_val;
	    return SEMICOLON;
	}      
    }
    
    
    if(yylex_val == '}') 
    {
	was_end_brace = 1;
    }
    
    return yylex_val;
}

void yyerror (char *s) 
{
    fwprintf (stderr, L"%s\n", s);
    duck_yacc_error=1;
}
