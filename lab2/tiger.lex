%{
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}
/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

%}
  /* You can add lex definitions here. */

%%
  /*
  * Below are some examples, which you can wipe out
  * and write reguler expressions and actions of your own.
  */

/* Skip white spaces. */
[ \r\t]	 {adjust(); continue;}
\n	 {adjust(); EM_newline(); continue;}


/* Reserved words. */
while     {adjust(); return WHILE;}
for       {adjust(); return FOR;}
to        {adjust(); return TO;}
break     {adjust(); return BREAK;}
let       {adjust(); return LET;}
in        {adjust(); return IN;}
end       {adjust(); return END;}
function  {adjust(); return FUNCTION;}
var       {adjust(); return VAR;}
type      {adjust(); return TYPE;}
array     {adjust(); return ARRAY;}
if        {adjust(); return IF;}
then      {adjust(); return THEN;}
else      {adjust(); return ELSE;}
do        {adjust(); return DO;}
of        {adjust(); return OF;}
nil       {adjust(); return NIL;}

/* Punctuation symbols. */
","   {adjust(); return COMMA;}
":"   {adjust(); return COLON;}
";"   {adjust(); return SEMICOLON;}
"("   {adjust(); return LPAREN;}
")"   {adjust(); return RPAREN;}
"["   {adjust(); return LBRACK;}
"]"   {adjust(); return RBRACK;}
"{"   {adjust(); return LBRACE;}
"}"   {adjust(); return RBRACE;}
"."   {adjust(); return DOT;}
"+"   {adjust(); return PLUS;}
"-"   {adjust(); return MINUS;}
"*"   {adjust(); return TIMES;}
"/"   {adjust(); return DIVIDE;}
"="   {adjust(); return EQ;}
"<>"  {adjust(); return NEQ;}
"<"   {adjust(); return LT;}
"<="  {adjust(); return LE;}
">"   {adjust(); return GT;}
">="  {adjust(); return GE;}
"&"   {adjust(); return AND;}
"|"   {adjust(); return OR;}
":="  {adjust(); return ASSIGN;}

/* Identifiers. */
[a-zA-Z]+[a-zA-Z0-9_]*  {
  adjust();
  yylval.sval = yytext;
  return ID;
}

/* Integer */
[0-9]+  {
  adjust();
  yylval.ival = atoi(yytext);
  return INT;
}
