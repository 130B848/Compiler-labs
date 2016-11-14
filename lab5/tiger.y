%{
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "errormsg.h"
#include "absyn.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
 exit(1);
}
%}


%union {
	int pos;
	int ival;
	string sval;
	A_var var;
	A_exp exp;
	/* et cetera */
    A_dec dec;
    A_ty ty;

    A_decList decList;
    A_expList expList;
    A_field	field;
    A_fieldList fieldList;
    A_fundec funcdec;
    A_fundecList funcdecList;
    A_namety namety;
    A_nametyList nametyList;
    A_efield efield;
    A_efieldList efieldList;
}

%token <sval> ID STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

%left ID
%nonassoc LOWER
%nonassoc OF
%nonassoc IF THEN WHILE DO FOR TO
%left ELSE
%nonassoc ASSIGN
%left OR AND
%nonassoc EQ NEQ GT LT GE LE
%left PLUS MINUS
%left TIMES DIVIDE
%nonassoc TYPE
%nonassoc FUNCTION

%type <var> var
%type <exp> exp program varExp nilExp intExp stringExp callExp opExp recordExp seqExp assignExp ifExp whileExp forExp breakExp letExp arrayExp
/* et cetera */
%type <dec> dec fucntionDec varDec typeDec
%type <ty> ty nameTy recordTy arrayTy
%type <decList> decList
%type <expList> argList _argList seqList
%type <field> field
%type <fieldList> fieldList _fieldList
%type <funcdec> funcdec
%type <funcdecList> funcdecList
%type <namety> namety
%type <nametyList> nametyList
%type <efield> efield
%type <efieldList> efieldList _efieldList

%start program

%%

program :   exp     {absyn_root = $1;}
        ;

var :   var LBRACK exp RBRACK   {$$ = A_SubscriptVar(EM_tokPos, $1, $3);}
    |   var DOT ID              {$$ = A_FieldVar(EM_tokPos, $1, S_Symbol($3));}
    |   ID                      {$$ = A_SimpleVar(EM_tokPos, S_Symbol($1));}
    |   ID  LBRACK exp RBRACK   {$$ = A_SubscriptVar(EM_tokPos, A_SimpleVar(EM_tokPos, S_Symbol($1)), $3);}
    ;

exp :   varExp      {$$ = $1;}
    |   nilExp      {$$ = $1;}
    |   intExp      {$$ = $1;}
    |   stringExp   {$$ = $1;}
    |   callExp     {$$ = $1;}
    |   opExp       {$$ = $1;}
    |   recordExp   {$$ = $1;}
    |   seqExp      {$$ = $1;}
    |   assignExp   {$$ = $1;}
    |   ifExp       {$$ = $1;}
    |   whileExp    {$$ = $1;}
    |   forExp      {$$ = $1;}
    |   breakExp    {$$ = $1;}
    |   letExp      {$$ = $1;}
    |   arrayExp    {$$ = $1;}
    ;

varExp  :   var     {$$ = A_VarExp(EM_tokPos, $1);}
        ;

nilExp  :   NIL     {$$ = A_NilExp(EM_tokPos);}
        ;

intExp  :   INT     {$$ = A_IntExp(EM_tokPos, $1);}
        ;

stringExp : STRING  {$$ = A_StringExp(EM_tokPos, $1);}
        ;

callExp :   ID LPAREN argList RPAREN {$$ = A_CallExp(EM_tokPos, S_Symbol($1), $3);}
        ;

opExp   :   exp   PLUS    exp     {$$ = A_OpExp(EM_tokPos, A_plusOp, $1, $3);}
		|   exp   MINUS   exp     {$$ = A_OpExp(EM_tokPos, A_minusOp, $1, $3);}
		|   exp   TIMES   exp     {$$ = A_OpExp(EM_tokPos, A_timesOp, $1, $3);}
		|   exp   DIVIDE  exp     {$$ = A_OpExp(EM_tokPos, A_divideOp, $1, $3);}
		|   exp   EQ      exp     {$$ = A_OpExp(EM_tokPos, A_eqOp, $1, $3);}
		|   exp   NEQ     exp     {$$ = A_OpExp(EM_tokPos, A_neqOp, $1, $3);}
		|   exp   LT      exp     {$$ = A_OpExp(EM_tokPos, A_ltOp, $1, $3);}
		|   exp   LE      exp     {$$ = A_OpExp(EM_tokPos, A_leOp, $1, $3);}
		|   exp   GT      exp     {$$ = A_OpExp(EM_tokPos, A_gtOp, $1, $3);}
		|   exp   GE      exp     {$$ = A_OpExp(EM_tokPos, A_geOp, $1, $3);}
		|         MINUS   exp     {$$ = A_OpExp(EM_tokPos, A_minusOp, A_IntExp(EM_tokPos, 0), $2);}
        ;

recordExp : ID LBRACE efieldList RBRACE {$$ = A_RecordExp(EM_tokPos, S_Symbol($1), $3);}
        ;

seqExp  :   LPAREN seqList RPAREN     {$$ = A_SeqExp(EM_tokPos, $2);}
        ;

assignExp : var ASSIGN exp            {$$ = A_AssignExp(EM_tokPos, $1, $3);}
        ;

ifExp   :   IF  exp THEN exp ELSE exp {$$ = A_IfExp(EM_tokPos, $2, $4, $6);}
        |   IF  exp THEN exp          {$$ = A_IfExp(EM_tokPos, $2, $4, A_NilExp(EM_tokPos));}
        |   exp OR  exp               {$$ = A_IfExp(EM_tokPos, $1, A_IntExp(EM_tokPos, 1), $3);}
        |   exp AND exp               {$$ = A_IfExp(EM_tokPos, $1, $3, A_IntExp(EM_tokPos, 0));}
        ;

whileExp:   WHILE exp DO exp          {$$ = A_WhileExp(EM_tokPos, $2, $4);}
        ;

forExp  :   FOR ID ASSIGN exp TO exp DO exp {$$ = A_ForExp(EM_tokPos, S_Symbol($2), $4, $6, $8);}
        ;

breakExp:   BREAK                       {$$ = A_BreakExp(EM_tokPos);}
        ;

letExp  :   LET decList IN seqList END  {$$ = A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, $4));}
        ;

arrayExp:   ID LBRACK exp RBRACK OF exp {$$ = A_ArrayExp(EM_tokPos, S_Symbol($1), $3, $6);}
        ;

dec     :   fucntionDec {$$ = $1;}
		|   varDec      {$$ = $1;}
		|   typeDec     {$$ = $1;}
        ;

fucntionDec :   funcdecList {$$ = A_FunctionDec(EM_tokPos, $1);}
            ;

varDec  :   VAR ID ASSIGN exp            {$$ = A_VarDec(EM_tokPos, S_Symbol($2), NULL, $4);}
		|   VAR ID COLON  ID  ASSIGN exp {$$ = A_VarDec(EM_tokPos, S_Symbol($2), S_Symbol($4), $6);}
        ;

typeDec :   nametyList {$$ = A_TypeDec(EM_tokPos, $1);}

ty      :   nameTy      {$$ = $1;}
        |   recordTy    {$$ = $1;}
        |   arrayTy     {$$ = $1;}
        ;

nameTy  :   ID                      {$$ = A_NameTy(EM_tokPos, S_Symbol($1));}
        ;

recordTy:   LBRACE fieldList RBRACE {$$ = A_RecordTy(EM_tokPos, $2);}
        ;

arrayTy :   ARRAY  OF        ID     {$$ = A_ArrayTy(EM_tokPos, S_Symbol($3));}
        ;

decList :   dec %prec   LOWER   {$$ = A_DecList($1, NULL);}
        |   dec decList         {$$ = A_DecList($1, $2);}
        ;

/* Cannot merely use "expList" because of COMMA and SEMICOLON */

argList :   exp _argList        {$$ = A_ExpList($1, $2);}
        |                       {$$ = NULL;}
        ;

_argList:   COMMA exp _argList  {$$ = A_ExpList($2, $3);}
        |                       {$$ = NULL;}
        ;

seqList :  exp SEMICOLON seqList{$$ = A_ExpList($1, $3);}
        |  exp                  {$$ = A_ExpList($1, NULL);}
        |                       {$$ = NULL;}
        ;

field   :   ID COLON ID         {$$ = A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3));}
        ;

fieldList   :   field _fieldList %prec LOWER {$$ = A_FieldList($1, $2);}
            |                   {$$ = NULL;}
            ;

_fieldList  :   COMMA field _fieldList  {$$ = A_FieldList($2, $3);}
            |                   {$$ = NULL;}
            ;

funcdec :   FUNCTION ID LPAREN fieldList RPAREN COLON ID EQ exp {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol($7), $9);}
		|   FUNCTION ID LPAREN fieldList RPAREN EQ    exp       {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, NULL, $7);}
        ;

funcdecList :   funcdec funcdecList {$$ = A_FundecList($1, $2);}
            |   funcdec %prec LOWER {$$ = A_FundecList($1, NULL);}
            ;

namety      :   TYPE    ID EQ ty    {$$ = A_Namety(S_Symbol($2), $4);}
            ;

nametyList  :   namety  nametyList  {$$ = A_NametyList($1, $2);}
            |   namety %prec LOWER  {$$ = A_NametyList($1, NULL);}
            ;

efield      :   ID      EQ exp      {$$ = A_Efield(S_Symbol($1), $3);}
            ;

efieldList  :   efield _efieldList  {$$ = A_EfieldList($1, $2);}
            |                       {$$ = NULL;}
            ;

_efieldList :   COMMA efield _efieldList {$$ = A_EfieldList($2, $3);}
            |                       {$$ = NULL;}
            ;
