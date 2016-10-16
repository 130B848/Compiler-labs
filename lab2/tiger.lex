%{
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

int charPos = 1;

int yywrap(void)
{
    charPos = 1;
    return 1;
}

void adjust(void)
{
    EM_tokPos = charPos;
    charPos += yyleng;
}
/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/
int nest_cmt_cnt = 0;

#define MAXLEN 512
char buf[MAXLEN];
int str_pos = 0;

%}
  /* You can add lex definitions here. */
%x COMMENTS STRINGS

%%
  /*
  * Below are some examples, which you can wipe out
  * and write reguler expressions and actions of your own.
  */

  /* Reserved words. */
while       {adjust(); return WHILE;}
for         {adjust(); return FOR;}
to          {adjust(); return TO;}
break       {adjust(); return BREAK;}
let         {adjust(); return LET;}
in          {adjust(); return IN;}
end         {adjust(); return END;}
function    {adjust(); return FUNCTION;}
var         {adjust(); return VAR;}
type        {adjust(); return TYPE;}
array       {adjust(); return ARRAY;}
if          {adjust(); return IF;}
then        {adjust(); return THEN;}
else        {adjust(); return ELSE;}
do          {adjust(); return DO;}
of          {adjust(); return OF;}
nil         {adjust(); return NIL;}
  /* Punctuation symbol */
","         {adjust(); return COMMA;}
":"         {adjust(); return COLON;}
";"         {adjust(); return SEMICOLON;}
"("         {adjust(); return LPAREN;}
")"         {adjust(); return RPAREN;}
"["         {adjust(); return LBRACK;}
"]"         {adjust(); return RBRACK;}
"{"         {adjust(); return LBRACE;}
"}"         {adjust(); return RBRACE;}
"."         {adjust(); return DOT;}
"+"         {adjust(); return PLUS;}
"-"         {adjust(); return MINUS;}
"*"         {adjust(); return TIMES;}
"/"         {adjust(); return DIVIDE;}
"="         {adjust(); return EQ;}
"<>"        {adjust(); return NEQ;}
"<"         {adjust(); return LT;}
"<="        {adjust(); return LE;}
">"         {adjust(); return GT;}
">="        {adjust(); return GE;}
"&"         {adjust(); return AND;}
"|"         {adjust(); return OR;}
":="        {adjust(); return ASSIGN;}
  /* White space */
[ \t]       {adjust();}
\n	        {adjust(); EM_newline();}

  /* Identifiers */
[a-zA-Z]+[a-zA-Z0-9_]*  {adjust(); yylval.sval = yytext; return ID;}

  /* Comments */
"/*"        {adjust(); nest_cmt_cnt++; BEGIN(COMMENTS);}
"*/"        {EM_error(EM_tokPos, "ERROR: Unopened comment."); yyterminate();}

<COMMENTS>{
    <<EOF>> {EM_error(EM_tokPos, "ERROR: Unclosed comment."); yyterminate();}
    "/*"    {adjust(); nest_cmt_cnt++;}
    "*/"    {adjust(); nest_cmt_cnt--; if (!nest_cmt_cnt) BEGIN(INITIAL);}
    \n      {adjust(); EM_newline();}
    .       {adjust();}
}

  /* Integer literal */
[0-9]+      {adjust(); yylval.ival = atoi(yytext); return INT;}

  /* String literal */
\"          {adjust(); bzero(buf, MAXLEN); str_pos = EM_tokPos; BEGIN(STRINGS);}

<STRINGS>{
    <<EOF>> {EM_error(EM_tokPos, "ERROR: Unclosed string."); yyterminate();}
    \"      {adjust(); yylval.sval = strlen(buf) ? buf : "(null)"; EM_tokPos = str_pos; BEGIN(INITIAL); return STRING;}
    \\n     {adjust(); EM_newline(); strcat(buf, "\n");}
    .       {adjust(); strcat(buf, yytext);}
}

  /* Other cases are all illegal */
.	        {adjust(); EM_error(EM_tokPos, "ERROR: Illegal input.");}
