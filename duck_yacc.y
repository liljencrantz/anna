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
 
#define LOOKUP_CREATE(name) (duck_node_t *)duck_node_lookup_create(duck_current_filename, duck_current_pos, name)
#define CALL_CREATE(function, argc, argv) duck_node_call_create(duck_current_filename, duck_current_pos, function, argc, argv)
#define INT_LITERAL_CREATE(val) (duck_node_t *)duck_node_int_literal_create(duck_current_filename, duck_current_pos, val)
#define CHAR_LITERAL_CREATE(val) (duck_node_t *)duck_node_char_literal_create(duck_current_filename, duck_current_pos, val)
#define FLOAT_LITERAL_CREATE(val) (duck_node_t *)duck_node_float_literal_create(duck_current_filename, duck_current_pos, val)
#define STRING_LITERAL_CREATE(val) duck_yacc_string_literal_create(duck_current_filename, duck_current_pos, val)
#define NULL_CREATE(val) duck_node_null_create(duck_current_filename, duck_current_pos)

int yylex();

int yylex_val;
int duck_yacc_error=0;
 

wchar_t *duck_yacc_string(char *in)
{
    /*
      FIXME: Add resource tracking and cleanup
     */   
    return str2wcs(in);
    
}

duck_node_t *duck_yacc_string_literal_create(wchar_t *filename, size_t pos, char *str)
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
    
    return (duck_node_t *)duck_node_string_literal_create(filename, pos, ptr_out-str3, str3);
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
	    if($2)
		duck_node_call_add_child($1,$2);
	}
	| 
	expression SEMICOLON
	{
	    $$ = CALL_CREATE(LOOKUP_CREATE(L"__block__"),0,0);
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
	    $$ = CALL_CREATE(LOOKUP_CREATE(L"__block__"),0,0);
	}
	| 
	block3 opt_semicolon
;

block3 :
	block3 SEMICOLON expression
	{
	    $$ = $1;
	    duck_node_call_add_child($1,$3);
	}
	| 
	expression
	{
	    $$ = CALL_CREATE(LOOKUP_CREATE(L"__block__"), 0, 0);
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
	    $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__assign__"), 2, param);
	}
	|
	expression op expression1
	{
	    duck_node_t *param[] ={$1, $3};	    
	    $$ = (duck_node_t *)CALL_CREATE($2, 2, param);
	}
        | expression1
        | function_definition
	| declaration_expression 
	| type_definition
	| RETURN expression1
	{
	    duck_node_t *param[] ={$2};	    
	    $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__return__"), 1, param);	  
	}
	| RETURN
	{
	  duck_node_t *param[] ={NULL_CREATE()};	    
	  $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__return__"), 1, param);	  
	}
	| RETURN '=' expression1
	{
	    duck_node_t *param[] ={$3};	    
	    $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__assignReturn__"), 1, param);	  
	}
;

expression1 :
	expression1 op1 expression2
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)CALL_CREATE($2, 2, param);
	}
        | 
	expression2
;


opt_semicolon: | SEMICOLON;

expression2 :
	expression2 op2 expression3
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)CALL_CREATE($2, 2, param);
	}
        | 
	expression3
	;

expression3 :
	expression3 op3 expression4
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)CALL_CREATE($2, 2, param);
	}
        | 
	expression4
	;

expression4 :
	expression4 op4 expression5
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)CALL_CREATE($2, 2, param);
	}
        | 
	expression5
	;

expression5 :
	expression5 op5 expression6
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)CALL_CREATE($2, 2, param);
	}
        | 
	expression6
	;

expression6 :
	expression6 '.' expression7
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__memberGet__"), 2, param);
	}
        |
	expression6 '(' argument_list2 ')' opt_block
	{
	    $$ = (duck_node_t *)$3;
	    duck_node_call_set_function($3, $1);
	    if ($5) 
		duck_node_call_add_child($3, (duck_node_t *)$5);
	}
	| 
	expression6 block
	{
	  duck_node_t *param[] ={$2};
	  $$ = CALL_CREATE($1, 1, param);
	}
	| 
	expression6 '[' expression ']'
	{
	    duck_node_t *param[] ={$1, $3};   
	    $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__get__"), 2, param);
	}
	| 
	'[' argument_list2 ']' /* Alternative list constructor syntax */
	{	    
	    $$ = (duck_node_t *)$2;
	    duck_node_call_set_function($2, LOOKUP_CREATE(L"__list__"));
	}
	| 
	'|' argument_list2 '|' 
	{	    
	    $$ = (duck_node_t *)$2;
	    duck_node_call_set_function($2, LOOKUP_CREATE(L"__abs__"));
	}
	|
	pre_op6 expression7
	{
	    duck_node_t *param[] ={$2};   
	    $$ = (duck_node_t *)CALL_CREATE($1, 1, param);
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
	    $$ = NULL_CREATE();
	}
	| block 
	{
	    $$ = (duck_node_t *)$1; 
	}
;

op:
	APPEND
	{
	    $$ = LOOKUP_CREATE(L"__append__");
	}
;


op1:
	AND
	{
	    $$ = LOOKUP_CREATE(L"__and__");
	}
	|
	OR
	{
		$$ = LOOKUP_CREATE(L"__or__");
	}
;

op2:
	'<'
	{
		$$ = LOOKUP_CREATE(L"__lt__");
	}
	|
	'>'
	{
		$$ = LOOKUP_CREATE(L"__gt__");
	}
	|
	EQUAL
	{
		$$ = LOOKUP_CREATE(L"__eq__");
	}
	|
	NOT_EQUAL
	{
		$$ = LOOKUP_CREATE(L"__ne__");
	}
	|
	LESS_OR_EQUAL
	{
		$$ = LOOKUP_CREATE(L"__le__");
	}
	|
	GREATER_OR_EQUAL
	{
		$$ = LOOKUP_CREATE(L"__ge__");
	}
;

op3:
	'+'
	{
	    $$ = LOOKUP_CREATE(L"__add__");
	}
	|
	'-'
	{
	    $$ = LOOKUP_CREATE(L"__sub__");
	}
	|
	'~'
	{
	    $$ = LOOKUP_CREATE(L"__join__");
	}
	|
	':'
	{
	    $$ = LOOKUP_CREATE(L"Pair");
	}
	|
	RANGE
	{
	    $$ = LOOKUP_CREATE(L"Range");
	}
;

op4:
	'*'
	{
	    $$ = LOOKUP_CREATE(L"__mul__");
	}
	|
	'/'
	{
	    $$ = LOOKUP_CREATE(L"__div__");
	}
	|
	'%'
	{
	    $$ = LOOKUP_CREATE(L"__mod__");
	}
;

op5: '^'
{
    $$ = LOOKUP_CREATE(L"__exp__");
}


pre_op6:
	'!'
	{
	    $$ = LOOKUP_CREATE(L"__not__");
	}
	|
	'-'
	{
		$$ = LOOKUP_CREATE(L"__neg__")
	}
	|
	SIGN
	{
		$$ = LOOKUP_CREATE(L"__sign__")
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
	    $$ = (duck_node_t *)LOOKUP_CREATE(duck_yacc_string(duck_text));
	}
;


type_identifier :
	TYPE_IDENTIFIER
	{
	    $$ = LOOKUP_CREATE(duck_yacc_string(duck_text));
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
	    $$ = (duck_node_t *)INT_LITERAL_CREATE(atoi(duck_text));
	}
	| 
	LITERAL_FLOAT
	{
	    $$ = (duck_node_t *)FLOAT_LITERAL_CREATE(atof(duck_text));
	}
	| 
	LITERAL_CHAR
	{
	    $$ = (duck_node_t *)CHAR_LITERAL_CREATE(*duck_text);
	}
	| 
	LITERAL_STRING
	{
	    $$ = (duck_node_t *)STRING_LITERAL_CREATE(duck_text);
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
	    $$ = CALL_CREATE(0,0,0);
	}
	|
	argument_list3
;

argument_list3 :
	expression 
	{
	    $$ = CALL_CREATE(0, 0, 0);
	    duck_node_call_add_child($$, (duck_node_t *)$1);
	}
	| 
	argument_list3 ',' expression
	{
	    $$ = $1;
	    duck_node_call_add_child($$, (duck_node_t *)$3);
	}
	;

function_definition: 
	FUNCTION opt_identifier declaration_list attribute_list block
	{
	  duck_node_t *param[] ={(duck_node_t *)($2?$2:NULL_CREATE()),NULL_CREATE(), (duck_node_t *)$3, (duck_node_t *)$5, (duck_node_t *)$4};	    
	    $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__function__"), 5, param);
	  	  
	}
;

declaration_list :
	'(' ')'
	{
	    $$ = CALL_CREATE(LOOKUP_CREATE(L"__block__"),0,0);
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
	    $$ = CALL_CREATE(LOOKUP_CREATE(L"__block__"), 0, 0);
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
 	    param[2] = $3?$3:NULL_CREATE();
	    $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__declare__"), 3, param);    
	}
;

declaration : opt_var variable_declaration {$$=$2;} | function_declaration;

opt_var : | VAR;


function_declaration :
	FUNCTION templatized_type identifier declaration_list
	{
	  duck_node_t *param[] ={$3, $2, (duck_node_t *)$4, NULL_CREATE(), NULL_CREATE()};	    
	  $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__function__"), 5, param);
	}
;

opt_templatized_type:
{
  $$=NULL_CREATE();
}
|
templatized_type;

templatized_type:
type_identifier 
| type_identifier templatization
{
  duck_node_t *param[] ={$1, (duck_node_t *)$2};	    
  $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__templatize__"), 2, param);
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
	    $$ = CALL_CREATE(LOOKUP_CREATE(L"__block__"), 1, param);
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
	  $$ = (duck_node_t *)CALL_CREATE(LOOKUP_CREATE(L"__type__"), 4, param);
	}
	;

attribute_list :
	/* Empty */
	{
	    $$ = CALL_CREATE(LOOKUP_CREATE(L"__block__"),0,0);
	}
	| 
	attribute_list2
;

attribute_list2 :
	attribute_list ',' identifier opt_simple_expression
	{
	    $$ = $1;
	    duck_node_call_t *attr = CALL_CREATE($3, 0, 0);
	    if($4)
	      duck_node_call_add_child(attr,$4);
	    duck_node_call_add_child($$,(duck_node_t *)attr);
	}
	|
	identifier opt_simple_expression
	{
	    $$ = CALL_CREATE(LOOKUP_CREATE(L"__block__"), 0, 0);
	    duck_node_call_t *attr = CALL_CREATE($1, 0, 0);
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
  
  yylex_val = duck_lex();
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
