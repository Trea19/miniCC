%{
  #include <stdlib.h>
  #include <stdio.h>
  #include "AST.h"
  #include "parser.tab.h"

  int error_flag = 0;
  int empty_flag = 1;

  FILE *in;
  ASTNode* root;

  int yycolumn = 1;
  extern YYLTYPE yylloc;
  #define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

/*开放flex yylineno选项，自动记录行号*/
%option yylineno  

/*INT*/
INT_BIN (0[bB][01]+)
INT_BIN_ERROR (0[bB][01]*[2-9a-zA-Z]+[01]*)

INT_OCT (0[0-7]+)
INT_OCT_ERROR (0[0-7]*[89a-zA-Z]+[0-7]*)

INT_DEC (0|[1-9][0-9]*)

INT_HEX (0[xX][a-fA-F0-9]+)
INT_HEX_ERROR 0[xX][a-fA-F0-9]*[g-zG-Z]+[a-fA-F0-9]*

/*FLOAT*/
FLOAT			([0-9]*\.[0-9]+|[0-9]+\.)
FLOAT_E			{FLOAT}[Ee][+-]?[0-9]+
FLOAT_ERROR		({FLOAT}[Ee][+-]?)|({FLOAT}[Ee][+-]?[0-9]*\.[0-9]*)

/*ID*/
ID [_a-zA-Z][a-z_A-Z0-9]*

/*KEY WORD*/
STRUCT struct
RETURN return
IF if
ELSE else
WHILE while
TYPE int|float

/*BOUND SYMBOL*/
SEMI  ;
COMMA ,
LP    \(
RP    \)
LB    \[
RB    \]
LC    \{
RC    \}

/*OPERATOR*/
ASSIGNOP  =
PLUS  \+
MINUS \-
STAR  \*
DIV   \/
AND   &&
OR    \|\|
DOT   \.
NOT   \!
RELOP >|<|>=|<=|==|!=

/*COMMENT*/
COMMENT ("//"[^\n]*)|("/*"([^*]*"*"+[^*/])*[^*]*"*"+"/")

/*SPACE*/
SPACE [ \t]+

/*END OF LINE*/
EOL \n

/*UNDEFINED*/
AERROR .


%%
{SPACE}         {}
{COMMENT}       {}
{EOL}           {yycolumn = 1;}
{INT_DEC}		{yylval.node = create_node("INT", yytext, INT, yylloc.first_line); return INT;}
{INT_BIN}		{yylval.node = create_node("INT", yytext, INT, yylloc.first_line); yylval.node->int_type = 1; return INT; }
{INT_OCT}		{yylval.node = create_node("INT", yytext, INT, yylloc.first_line); yylval.node->int_type = 2; return INT; }
{INT_HEX}		{yylval.node = create_node("INT", yytext, INT, yylloc.first_line); yylval.node->int_type = 2; return INT; }
{FLOAT}			{yylval.node = create_node("FLOAT", yytext, FLOAT, yylloc.first_line); return FLOAT; }
{FLOAT_E}		{yylval.node = create_node("FLOAT", yytext, FLOAT, yylloc.first_line); return FLOAT; }
{INT_BIN_ERROR} {printf("Error type A at Line %d: Illegal binary number \'%s\'.\n", yylineno, yytext); error_flag = 1;}
{INT_OCT_ERROR} {printf("Error type A at Line %d: Illegal octal number \'%s\'.\n", yylineno, yytext); error_flag = 1;}
{INT_HEX_ERROR} {printf("Error type A at Line %d: Illegal hexadecimal number \'%s\'.\n", yylineno, yytext); error_flag = 1;}
{FLOAT_ERROR}	{printf("Error type A at Line %d: Illegal floating point number \'%s\'.\n", yylineno, yytext); error_flag = 1;}
{STRUCT}		{yylval.node = create_node("STRUCT", yytext, STRUCT, yylloc.first_line); return STRUCT; }
{RETURN}		{yylval.node = create_node("RETURN", yytext, RETURN, yylloc.first_line); return RETURN; }
{IF}			{yylval.node = create_node("IF", yytext, IF, yylloc.first_line); return IF; }
{ELSE}			{yylval.node = create_node("ELSE", yytext, ELSE, yylloc.first_line); return ELSE; }
{WHILE}			{yylval.node = create_node("WHILE", yytext, WHILE, yylloc.first_line); return WHILE; }
{TYPE}			{yylval.node = create_node("TYPE", yytext, TYPE, yylloc.first_line); return TYPE; }
{ID}			{yylval.node = create_node("ID", yytext, ID, yylloc.first_line); return ID; }
{SEMI} 			{yylval.node = create_node("SEMI", yytext, SEMI, yylloc.first_line); return SEMI; }
{COMMA}			{yylval.node = create_node("COMMA", yytext, COMMA, yylloc.first_line); return COMMA; }
{ASSIGNOP}		{yylval.node = create_node("ASSIGNOP", yytext, ASSIGNOP, yylloc.first_line); return ASSIGNOP; }
{RELOP}			{yylval.node = create_node("RELOP", yytext, RELOP, yylloc.first_line); return RELOP; }
{PLUS}			{yylval.node = create_node("PLUS", yytext, PLUS, yylloc.first_line); return PLUS; }
{MINUS}			{yylval.node = create_node("MINUS", yytext, MINUS, yylloc.first_line); return MINUS; }
{STAR}			{yylval.node = create_node("STAR", yytext, STAR, yylloc.first_line); return STAR; }
{DIV}			{yylval.node = create_node("DIV", yytext, DIV, yylloc.first_line); return DIV; }
{AND}			{yylval.node = create_node("AND", yytext, AND, yylloc.first_line); return AND; }
{OR}			{yylval.node = create_node("OR", yytext, OR, yylloc.first_line); return OR; }
{DOT}			{yylval.node = create_node("DOT", yytext, DOT, yylloc.first_line); return DOT; }
{NOT}			{yylval.node = create_node("NOT", yytext, NOT, yylloc.first_line); return NOT; }
{LP}			{yylval.node = create_node("LP", yytext, LP, yylloc.first_line); return LP; }
{RP}			{yylval.node = create_node("RP", yytext, RP, yylloc.first_line); return RP; }
{LB}			{yylval.node = create_node("LB", yytext, LB, yylloc.first_line); return LB; }
{RB}			{yylval.node = create_node("RB", yytext, RB, yylloc.first_line); return RB; }
{LC}			{yylval.node = create_node("LC", yytext, LC, yylloc.first_line); return LC; }
{RC}			{yylval.node = create_node("RC", yytext, RC, yylloc.first_line); return RC; }
{AERROR}        {printf("Error type A at Line %d: Mysterious characters \"%s\".\n", yylineno, yytext); error_flag = 1; }

%%

