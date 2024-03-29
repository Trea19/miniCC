%{
  #include <stdio.h>
  #include "syntax.tab.h"
  #include "utils.h"

  FILE* in;
  AST_Node* root;

  int error_flag = 0;
  int empty_flag = 1;

  int yycolumn = 1;
  extern YYLTYPE yylloc;
  #define YY_USER_ACTION \
   yylloc.first_line = yylloc.last_line = yylineno; \
   yylloc.first_column = yycolumn; \
   yylloc.last_column = yycolumn + yyleng - 1; \
   yycolumn += yyleng;
%}

%option yylineno

SPACE             [ \t]+
CHLINE            \n+
COMMENT_LINE      "//"[^\n]*
COMMENT           "/*"([^*]*"*"+[^*/])*[^*]*"*"+"/"
STRUCT            struct
RETURN            return
IF                if
ELSE              else
WHILE             while
TYPE              int|float
OCTINT            (0[0-7]+)
OCTINT_ERROR      (0[0-7]*[89a-zA-Z]+[0-7]*)
HEXINT            (0[xX][0-9a-fA-F]+)
HEXINT_ERROR      (0[xX][a-fA-F0-9]*[g-zG-Z]+[a-fA-F0-9]*)
INT               ((0|[1-9][0-9]*))
FLOAT             ([0-9]*\.[0-9]+|[0-9]+\.)
FLOAT_E           {FLOAT}[Ee][+-]?[0-9]+
FLOAT_ERROR       ({FLOAT}[Ee][+-]?)|({FLOAT}[Ee][+-]?[0-9]*\.[0-9]*)
ID                [_a-zA-Z][_a-zA-Z0-9]*
SEMI              ;
COMMA             ,
ASSIGNOP          =
RELOP             >|<|>=|<=|==|!=
PLUS              \+
MINUS             \-
STAR              \*
DIV               \/
AND               &&
OR                \|\|
DOT               \.
NOT               !
LP                \(
RP                \)
LB                \[
RB                \]
LC                \{
RC                \}

%%

{SPACE}           { /* ignore space */ }
{CHLINE}          { yycolumn = 1; }
{COMMENT}         { update_column(yytext); }
{COMMENT_LINE}    { /* ignore comment_line */}

{STRUCT}          { yylval.node = create_node("STRUCT", yytext, STRUCT, yylloc.first_line); return STRUCT; }
{RETURN}          { yylval.node = create_node("RETURN", yytext, RETURN, yylloc.first_line); return RETURN; }
{IF}              { yylval.node = create_node("IF", yytext, IF, yylloc.first_line); return IF; }
{ELSE}            { yylval.node = create_node("ELSE", yytext, ELSE, yylloc.first_line); return ELSE; }
{WHILE}           { yylval.node = create_node("WHILE", yytext, WHILE, yylloc.first_line); return WHILE; }
{TYPE}            { yylval.node = create_node("TYPE", yytext, TYPE, yylloc.first_line); return TYPE; }
{OCTINT}          { yylval.node = create_node("INT", yytext, INT, yylloc.first_line); yylval.node->int_type = 1; return INT; }
{HEXINT}          { yylval.node = create_node("INT", yytext, INT, yylloc.first_line); yylval.node->int_type = 2; return INT; }
{INT}             { yylval.node = create_node("INT", yytext, INT, yylloc.first_line); return INT; }
{FLOAT}           { yylval.node = create_node("FLOAT", yytext, FLOAT, yylloc.first_line); return FLOAT; }
{FLOAT_E}         { yylval.node = create_node("FLOAT", yytext, FLOAT, yylloc.first_line); return FLOAT; }
{ID}              { yylval.node = create_node("ID", yytext, ID, yylloc.first_line); return ID; }
{SEMI}            { yylval.node = create_node("SEMI", yytext, SEMI, yylloc.first_line); return SEMI; }
{COMMA}           { yylval.node = create_node("COMMA", yytext, COMMA, yylloc.first_line); return COMMA; }
{ASSIGNOP}        { yylval.node = create_node("ASSIGNOP", yytext, ASSIGNOP, yylloc.first_line); return ASSIGNOP; }
{RELOP}           { yylval.node = create_node("RELOP", yytext, RELOP, yylloc.first_line); return RELOP; }
{PLUS}            { yylval.node = create_node("PLUS", yytext, PLUS, yylloc.first_line); return PLUS; }
{MINUS}           { yylval.node = create_node("MINUS", yytext, MINUS, yylloc.first_line); return MINUS; }
{STAR}            { yylval.node = create_node("STAR", yytext, STAR, yylloc.first_line); return STAR; }
{DIV}             { yylval.node = create_node("DIV", yytext, DIV, yylloc.first_line); return DIV; }
{AND}             { yylval.node = create_node("AND", yytext, AND, yylloc.first_line); return AND; }
{OR}              { yylval.node = create_node("OR", yytext, OR, yylloc.first_line); return OR; }
{DOT}             { yylval.node = create_node("DOT", yytext, DOT, yylloc.first_line); return DOT; }
{NOT}             { yylval.node = create_node("NOT", yytext, NOT, yylloc.first_line); return NOT; }
{LP}              { yylval.node = create_node("LP", yytext, LP, yylloc.first_line); return LP; }
{RP}              { yylval.node = create_node("RP", yytext, RP, yylloc.first_line); return RP; }
{LB}              { yylval.node = create_node("LB", yytext, LB, yylloc.first_line); return LB; }
{RB}              { yylval.node = create_node("RB", yytext, RB, yylloc.first_line); return RB; }
{LC}              { yylval.node = create_node("LC", yytext, LC, yylloc.first_line); return LC; }
{RC}              { yylval.node = create_node("RC", yytext, RC, yylloc.first_line); return RC; }
{FLOAT_ERROR}     { printf("Error type A at Line %d: Illegal floating point number \'%s\'.\n", yylineno, yytext); error_flag = 1; }
{OCTINT_ERROR}    { printf("Error type A at Line %d: Illegal octal number \'%s\'.\n", yylineno, yytext); error_flag = 1; }
{HEXINT_ERROR}    { printf("Error type A at Line %d: Illegal hexadecimal number \'%s\'.\n", yylineno, yytext); error_flag = 1; }
.                 { printf("Error type A at Line %d: Mysterious characters \"%s\".\n", yylineno, yytext); error_flag = 1; }

%%

