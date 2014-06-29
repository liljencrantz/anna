%{

#include "anna/config.h"

#include "anna/fallback.h"
#include "anna/anna.h"
#include "anna/node.h"
#include "autogen/yacc.h"
#include "anna/util.h"
#include "anna/parse.h"

int anna_lex_wrap(yyscan_t yyscanner) 
{
  return (1); 
}

static hash_table_t *anna_lex_nest=0;
int anna_lex_error_count = 0;
static buffer_t long_buffer;

static void anna_lex_nest_init()
{
   if(anna_lex_nest != 0)
   {
      return;
   }
   anna_lex_nest = malloc(sizeof(hash_table_t));
   hash_init(anna_lex_nest, hash_ptr_func, hash_ptr_cmp);
   b_init(&long_buffer);
}
 
static void anna_lex_push_state_internal(yyscan_t yyscanner, int state)
{
   anna_lex_nest_init();
   array_list_t *list = (array_list_t *)hash_get(anna_lex_nest, yyscanner);
   if(!list)
   {
       list = calloc(1, sizeof(array_list_t));
       hash_put(anna_lex_nest, yyscanner, list);
   }
   al_push(list, (void *)(long)state);
}

static int anna_lex_peek_long_string_length(yyscan_t yyscanner)
{
   anna_lex_nest_init();
   array_list_t *list = (array_list_t *)hash_get(anna_lex_nest, yyscanner);
   long state = (long)al_peek(list);
   return state >> 8;
}
 
static int anna_lex_pop_state_internal(yyscan_t yyscanner, int default_state)
{
   anna_lex_nest_init();
   array_list_t *list = (array_list_t *)hash_get(anna_lex_nest, yyscanner);
   al_pop(list);
   int state = default_state;
   if(al_get_count(list))
   {
       state = (int)(long)al_peek(list) & 0xff;
   }
   else
   {
       al_destroy(list);
       free(list);
       hash_remove(anna_lex_nest, yyscanner, 0, 0);       
   }
   
   return state;
}

#define anna_lex_push_state(yy, state) BEGIN(state); anna_lex_push_state_internal(yy, state);
#define anna_lex_push_state_param(yy, state, param) BEGIN(state); anna_lex_push_state_internal(yy, state | (param << 8));
#define anna_lex_pop_state(yy) BEGIN(anna_lex_pop_state_internal(yy, INITIAL)); 

static void anna_lex_unbalanced_comment()
{
    anna_parse_error(0, L"Unbalanced comment.\n");
    anna_lex_error_count++;
}

static void anna_lex_invalid_input()
{
    anna_parse_error(0, L"Invalid input.\n");
    anna_lex_error_count++;
}

static int anna_lex_long_count_begin(char *str)
{
    return strlen(str) - (strrchr(str, '/')-str) -2;
}

static int anna_lex_long_count_end(char *str)
{
    return strlen(str) - 2;
}

%}

%option reentrant 
%option bison-bridge
%option header-file="autogen/lex.h"
%x COMMENT
%x LONG_STRING

%%

<COMMENT><<EOF>> anna_lex_unbalanced_comment();return 0;
<COMMENT>\/\* anna_lex_push_state(yyscanner, COMMENT); return IGNORE;
<COMMENT>\*\/ anna_lex_pop_state(yyscanner); return IGNORE;
<COMMENT>.  return IGNORE;
<COMMENT>[ \t\n]  return IGNORE;

<LONG_STRING>\"=*\/ if(anna_lex_long_count_end(anna_lex_get_text(yyscanner)) == anna_lex_peek_long_string_length(yyscanner)){ anna_lex_pop_state(yyscanner); return LITERAL_STRING_LONG_END; } return LITERAL_STRING_LONG_ELEMENT;
<LONG_STRING>\n return LITERAL_STRING_LONG_ELEMENT;
<LONG_STRING>. return LITERAL_STRING_LONG_ELEMENT;

\/\* anna_lex_push_state(yyscanner, COMMENT); return IGNORE;
_*[a-z][a-zA-Z0-9_!?]*\/=+\" anna_lex_push_state_param(yyscanner, LONG_STRING, anna_lex_long_count_begin(anna_lex_get_text(yyscanner))); b_clear(&long_buffer); return LITERAL_STRING_LONG_BEGIN;
\/=*\" anna_lex_push_state_param(yyscanner, LONG_STRING, anna_lex_long_count_begin(anna_lex_get_text(yyscanner))); return LITERAL_STRING_LONG_BEGIN;
\/\/[^\n]*\n  return SEPARATOR;
^#[^\n]*\n  return IGNORE;
0[oO][0-7][0-7_]*_*[a-z][a-zA-Z0-9_!?]* return LITERAL_INTEGER_BASE_8;
0[oO][0-7][0-7_]* return LITERAL_INTEGER_BASE_8;
0[bB][0-1][0-1_]*_*[a-z][a-zA-Z0-9_!?]* return LITERAL_INTEGER_BASE_2;
0[bB][0-1][0-1_]* return LITERAL_INTEGER_BASE_2;
0[xX][0-9a-fA-F][0-9a-fA-F_]*_*[a-z][a-zA-Z0-9_!?]* return LITERAL_INTEGER_BASE_16;
0[xX][0-9a-fA-F][0-9a-fA-F_]* return LITERAL_INTEGER_BASE_16;
[0-9][0-9_]*_*[a-z][a-zA-Z0-9_!?]* return LITERAL_INTEGER_BASE_10;
[0-9][0-9_]* return LITERAL_INTEGER_BASE_10;
[0-9][0-9_]*\.[0-9_]*[0-9]+[0-9_]*([eE][-+]?[0-9_]+)?_*[a-z][a-zA-Z0-9_!?]* return LITERAL_FLOAT;
[0-9][0-9_]*\.[0-9_]*[0-9]+[0-9_]*([eE][-+]?[0-9_]+)? return LITERAL_FLOAT;
\( return '(';
\) return ')';
\[ return '[';
\] return ']';
\{ return '{';
\} return '}';
def return DEF;
macro return MACRO;
\? return NULL_SYM;
and return AND;
or return OR;
var return VAR;
const return CONST;
return return RETURN;
break return BREAK;
continue return CONTINUE;
as return AS;
in return IN;
_*[a-z][a-zA-Z0-9_!?]*'([^\'\\]|\\.)*' return LITERAL_CHAR;
'([^\'\\]|\\.)*' return LITERAL_CHAR;
_*[a-z][a-zA-Z0-9_!?]*\"([^\"\\]|\\.)*\" return LITERAL_STRING;
\"([^\"\\]|\\.)*\" return LITERAL_STRING;
_*[a-z][a-zA-Z0-9_!?]* return IDENTIFIER;
_*[A-Z][a-zA-Z0-9_!?]* return TYPE_IDENTIFIER;
~ return '~';
~= return APPEND;
\+ return '+';
\+= return INCREASE;
\-= return DECREASE;
\+\+ return NEXT;
\-\- return PREV;
- return '-';
\* return '*';
\/ return '/';
== return EQUAL;
!= return NOT_EQUAL;
\<= return LESS_OR_EQUAL;
>= return GREATER_OR_EQUAL;
:: return STATIC_MEMBER_GET;
: return TO;
\< return '<';
> return '>';
= return '=';
:= return DECLARE_VAR;
:== return DECLARE_CONST;
\| return PAIR;
\\\r?\n return IGNORE;
\r?\n return SEPARATOR;
, return SEPARATOR;
! return '!';
% return '%';
\^ return '^';
\.\.\. return ELLIPSIS;
\.\. return RANGE;
\. return '.';
« return SPECIALIZATION_BEGIN;
» return SPECIALIZATION_END;
\<\< return SPECIALIZATION_BEGIN2;
>> return SPECIALIZATION_END2;
[\r \t] return IGNORE;
. anna_lex_invalid_input(); return 0;
