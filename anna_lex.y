%{

#include "anna.h"
#include "anna_node.h"
#include "anna_yacc.h"
#include "util.h"

int anna_lex_wrap(yyscan_t yyscanner) 
{
  return (1); 
}

static hash_table_t *anna_lex_nest=0;

static void anna_lex_nest_init()
{
   if(anna_lex_nest != 0)
   {
      return;
   }
   anna_lex_nest = malloc(sizeof(hash_table_t));
   hash_init(anna_lex_nest, hash_ptr_func, hash_ptr_cmp);
}
 
static void anna_lex_push_state(yyscan_t yyscanner)
{
   anna_lex_nest_init();
   long current = (long)hash_get(anna_lex_nest, yyscanner);
   current++;
   hash_put(anna_lex_nest, yyscanner, (void *)current);
}
 
static int anna_lex_pop_state(yyscan_t yyscanner)
{
   anna_lex_nest_init();
   long current = (long)hash_get(anna_lex_nest, yyscanner);
   current--;
   if(current)
   {
       hash_put(anna_lex_nest, yyscanner, (void *)current);
   }
   else 
   {
      hash_remove(anna_lex_nest, yyscanner, 0, 0);
   }
   return current;
}

static void anna_lex_unbalanced_comment()
{
    fwprintf(stderr, L"Error: Unbalanced comment at end of file.\n");
    anna_error_count++;
}

%}

%option reentrant 
%option header-file="anna_lex.h"
%x COMMENT

%%

<COMMENT><<EOF>> anna_lex_unbalanced_comment();return 0;
\/\* BEGIN(COMMENT); anna_lex_push_state(yyscanner); return IGNORE;
<COMMENT>\/\* anna_lex_push_state(yyscanner); return IGNORE;
<COMMENT>\*\/ if(anna_lex_pop_state(yyscanner)==0){BEGIN(INITIAL);}; return IGNORE;
<COMMENT>.  return IGNORE;
<COMMENT>[ \t\n]  return IGNORE;
\/\/[^\n]*\n  return IGNORE;
[1-9][0-9_]* return LITERAL_INTEGER_BASE_10;
0[oO][0-7][0-7_]* return LITERAL_INTEGER_BASE_8;
0[bB][0-1][0-1_]* return LITERAL_INTEGER_BASE_2;
0[xX][0-9a-fA-F][0-9a-fA-F_]* return LITERAL_INTEGER_BASE_16;
[0-9][0-9_]*\.[0-9_]+([eE]-?[0-9_]+)? return LITERAL_FLOAT;
0 return LITERAL_INTEGER_BASE_10;
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
if return IF;
else return ELSE;
[a-z_][a-zA-Z0-9_]* return IDENTIFIER;
[A-Z][a-zA-Z0-9_]* return TYPE_IDENTIFIER;
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
: return TO;
\< return '<';
> return '>';
= return '=';
:= return DECLARE_VAR;
:== return DECLARE_CONST;
\| return PAIR;
[,;]([ \t\n\r]*[,;])* return SEPARATOR;
! return '!';
% return '%';
\^ return '^';
\.\.\. return ELLIPSIS;
\.\. return RANGE;
\. return '.';
'([^\\]|\\.)' return LITERAL_CHAR;
« return SPECIALIZATION_BEGIN;
» return SPECIALIZATION_END;
\<\< return SPECIALIZATION_BEGIN2;
>> return SPECIALIZATION_END2;
\"([^\"\\]|\\.)*\" return LITERAL_STRING;
[ \t] return IGNORE;
[\n\r] return LINE_BREAK;
. return 0;
