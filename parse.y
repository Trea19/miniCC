%{
#include <stdio.h>
#include <stdlib.h>
int yylex(void);
void yyerror(const char*);
#define YYSTYPE char *
%}

%token T_Int T_Void T_Return T_Print T_ReadInt T_While
%token T_If T_Else T_Break T_Continue T_Le T_Ge T_Eq T_Ne
%token T_And T_Or T_IntConstant T_StringConstant T_Identifier

%left '='
%left T_Or
%left T_And
%left T_Eq T_Ne
%left '<' '>' T_Le T_Ge
%left '+' '-'
%left '*' '/' '%'
%left '!'

%%

Program:
    /* empty */             { /* empty */ }
|   Program FunctionDef        { /* empty */ }
;

FunctionDef:
    ReturnType FuncName '(' Args ')' '{' VarDecls Statements '}'    {printf("ENDFUNC\n\n"); }
;

ReturnType:
    T_Int                   { /* empty */ }
|   T_Void                  { /* empty */ }
;

FuncName:
    T_Identifier            { printf("FUNC @%s:\n", $1);}
;

Args:
    /* empty */             { /* empty */ }
|   _Args                   {printf("\n\n"); }
;

_Args:
    T_Int T_Identifier      { printf("\targ %s", $2); }
|   _Args ',' T_Int T_Identifier
                            { printf(", %s", $4); }
;

VarDecls:
    /* empty */             { /* empty */ }
|   VarDecls VarDecl ';'    { printf("\n\n"); }
;

VarDecl:
    T_Int T_Identifier      { printf("\tvar %s", $2); }
|   VarDecl ',' T_Identifier
                            { printf(", %s", $3); }
;

Statements:
    /* empty */             { /* empty */ }
|   Statements Statement              { /* empty */ }
;

Statement:
    AssignStatement              { printf("finish assign statement\n"); }
|   PrintStatement               { printf("finish print statement\n");}
|   CallStatement                { printf("finish call statement\n");}
|   ReturnStatement              { printf("finish return statement\n"); }
|   IfStatement                  { printf("finish if statement\n"); }
|   WhileStatement               { printf("finish while statement\n"); }
|   BreakStatement               { printf("finish break statement\n"); }
|   ContinueStatement            { printf("finish continue statement\n"); }
;

AssignStatement:
    T_Identifier '=' Expr ';'
                            { printf("\tpop %s\n\n", $1); }
;

PrintStatement:
    T_Print '(' T_StringConstant PActuals ')' ';'
                            { printf("\tprint %s\n\n", $3); }
;

PActuals:
    /* empty */             { /* empty */ }
|   PActuals ',' Expr       { /* empty */ }
;

CallStatement:
    CallExpr ';'            { printf("\tpop\n\n"); }
;

CallExpr:
    T_Identifier '(' Actuals ')'
                            { printf("\t$%s\n", $1); }
;

Actuals:
    /* empty */             { /* empty */ }
|   Expr PActuals           { /* empty */ }
;

ReturnStatement:
    T_Return Expr ';'       { printf("\tret ~\n\n"); }
|   T_Return ';'            { printf("\tret\n\n"); }
;

IfStatement:
    If TestExpr StatementsBlock
                            { /*empty*/ }
|   If TestExpr StatementsBlock Else StatementsBlock
                            { /*empty*/ }
;

TestExpr:
    '(' Expr ')'            { printf("finish testExpression\n");}
;

StatementsBlock:
    '{' Statements '}'           { printf("finish statementsBlock\n"); }
;

If:
    T_If            { /*empty*/}
;


Else:
    T_Else          { /* empty */ }
;

WhileStatement:
    While TestExpr StatementsBlock
                    { printf("finish whileStatement\n");}
;

While:
    T_While         { /*empty*/}
;

BreakStatement:
    T_Break ';'     { /*empty*/}
;

ContinueStatement:
    T_Continue ';'  { /*empty*/}
;

Expr:
    Expr '+' Expr           { printf("\tadd\n"); }
|   Expr '-' Expr           { printf("\tsub\n"); }
|   Expr '*' Expr           { printf("\tmul\n"); }
|   Expr '/' Expr           { printf("\tdiv\n"); }
|   Expr '%' Expr           { printf("\tmod\n"); }
|   Expr '>' Expr           { printf("\tcmpgt\n"); }
|   Expr '<' Expr           { printf("\tcmplt\n"); }
|   Expr T_Ge Expr          { printf("\tcmpge\n"); }
|   Expr T_Le Expr          { printf("\tcmple\n"); }
|   Expr T_Eq Expr          { printf("\tcmpeq\n"); }
|   Expr T_Ne Expr          { printf("\tcmpne\n"); }
|   Expr T_Or Expr          { printf("\tor\n"); }
|   Expr T_And Expr         { printf("\tand\n"); }
|   '-' Expr %prec '!'      { printf("\tneg\n"); }
|   '!' Expr                { printf("\tnot\n"); }
|   T_IntConstant           { printf("\tpush %s\n", $1); }
|   T_Identifier            { printf("\tpush %s\n", $1); }
|   ReadInt                 { /* empty */ }
|   CallExpr                { /* empty */ }
|   '(' Expr ')'            { /* empty */ }
;

ReadInt:
    T_ReadInt '(' T_StringConstant ')'
                            { printf("\treadint %s\n", $3); }
;

%%

int main() {
    return yyparse();
}
