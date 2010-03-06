
%{

#include "anna_node.h"
#include "anna_yacc.h"

int anna_lex_wrap(yyscan_t yyscanner) 
{
  return (1); 
}

%}

%option reentrant 
%option header-file="anna_lex.h"
%x COMMENT

%%

\/\* BEGIN(COMMENT); return IGNORE;
<COMMENT>\*\/ BEGIN(INITIAL);  return IGNORE;
<COMMENT>.  return IGNORE;
<COMMENT>[ \t\n]  return IGNORE;
\/\/[^\n]*\n  return IGNORE;
[0-9]+\.[0-9]+ return LITERAL_FLOAT;
[0-9]+ return LITERAL_INTEGER;
\( return '(';
\) return ')';
\[ return '[';
\] return ']';
\{ return '{';
\} return '}';
function return FUNCTION;
macro return MACRO;
null return NULL_SYM;
and return AND;
or return OR;
shl return SHL;
shr return SHR;
cshl return CSHL;
cshr return CSHR;
var return VAR;
sign return SIGN;
bitand return BITAND;
bitor return BITOR;
xor return XOR;
bitnot return BITNOT;
mod return MODULO;
return return RETURN;
[a-z_][a-zA-Z0-9_]* return IDENTIFIER;
[A-Z][a-zA-Z0-9_]* return TYPE_IDENTIFIER;
~ return '~';
~= return APPEND;
\+ return '+';
- return '-';
\* return '*';
\/ return '/';
== return EQUAL;
!= return NOT_EQUAL;
\<= return LESS_OR_EQUAL;
>= return GREATER_OR_EQUAL;
\< return '<';
> return '>';
= return '=';
: return ':';
;([ \t\n\r]*;)* return SEMICOLON;
, return ',';
! return '!';
% return '%';
\^ return '^';
\.\. return RANGE;
\. return '.';
\| return '|';
'([^\\]|\\.)' return LITERAL_CHAR;
\"([^\"\\]|\\.)*\" return LITERAL_STRING;
[ \t\n\r] return IGNORE;
. return 0;

