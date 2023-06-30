%{
#include <stdio.h>   
#include "AST.h"
#include "lex.yy.c"
#define YYERROR_VERBOSE

extern int error_flag;
extern int empty_flag;
extern ASTNode* root;
%}

%union{
    ASTNode* node;
}

/* tokens */
%token <node> INT FLOAT ID TYPE
%token <node> ASSIGNOP RELOP
%token <node> PLUS MINUS STAR DIV
%token <node> AND OR NOT
%token <node> DOT
%token <node> LP RP LB RB LC RC SEMI COMMA
%token <node> STRUCT RETURN IF ELSE WHILE

/* non-terminals */
%type <node> Program ExtDefList ExtDef ExtDecList
%type <node> Specifier StructSpecifier OptTag Tag
%type <node> VarDec FunDec VarList ParamDec
%type <node> CompSt StmtList Stmt
%type <node> DefList Def DecList Dec
%type <node> Exp Args

/* precedence & associativity */
%left COMMA
%right  ASSIGNOP
%left   OR
%left   AND
%left   RELOP
%left   PLUS MINUS
%left   STAR DIV
%right  NOT UMINUS
%left   LP RP LB RB DOT 
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE IF WHILE RETURN


%%

/* High-level Definitions */
Program : 
    ExtDefList { 
        $$ = create_node("Program", "", -1, @$.first_line); 
        root = $$; 
        add_child_sibling($$, 1, $1); 
    };

ExtDefList : 
    ExtDef ExtDefList { 
        empty_flag = 0; 
        $$ = create_node("ExtDefList", "", -1, @$.first_line); 
        add_child_sibling($$, 2, $1, $2); 
    }
    |  { 
        $$ = create_node("ExtDefList", "", -1, @$.first_line); 
    };
  
ExtDef : 
    Specifier ExtDecList SEMI { 
        $$ = create_node("ExtDef", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Specifier SEMI { 
        $$ = create_node("ExtDef", "", -1, @$.first_line); 
        add_child_sibling($$, 2, $1, $2); 
    }
    | Specifier FunDec CompSt { 
        $$ = create_node("ExtDef", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Specifier FunDec SEMI {
        $$ = create_node("ExtDef", "", -1, @$.first_line);
        add_child_sibling($$, 3, $1, $2, $3);
    } 
    | error SEMI {
    };

ExtDecList : 
    VarDec { 
        $$ = create_node("ExtDecList", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    }
    | VarDec COMMA ExtDecList { 
        $$ = create_node("ExtDecList", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    };

/* Specifiers */
Specifier : 
    TYPE { 
        $$ = create_node("Specifier", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    }
    | StructSpecifier { 
        $$ = create_node("Specifier", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    };

StructSpecifier : 
    STRUCT OptTag LC DefList RC { 
        $$ = create_node("StructSpecifier", "", -1, @$.first_line); 
        add_child_sibling($$, 5, $1, $2, $3, $4, $5); 
    }
    | STRUCT Tag { 
        $$ = create_node("StructSpecifier", "", -1, @$.first_line); 
        add_child_sibling($$, 2 , $1, $2); 
    }
    | STRUCT OptTag LC error RC {
    };

OptTag : 
    ID { 
        $$ = create_node("OptTag", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    }
    | { 
        $$ = create_node("OptTag", "", -1, @$.first_line); 
    };

Tag : 
    ID { 
        $$ = create_node("Tag", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    };

/* Declarators */
VarDec : 
    ID { 
        $$ = create_node("VarDec", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    }
    | VarDec LB INT RB { 
        $$ = create_node("VarDec", "", -1, @$.first_line); 
        add_child_sibling($$, 4, $1, $2, $3, $4); 
    }
    | VarDec LB error RB {
    };

FunDec : 
    ID LP VarList RP { 
        $$ = create_node("FunDec", "", -1, @$.first_line); 
        add_child_sibling($$, 4, $1, $2, $3, $4); 
    }
    | ID LP RP { 
        $$ = create_node("FunDec", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | ID LP error RP {
    };

VarList : 
    ParamDec COMMA VarList { 
        $$ = create_node("VarList", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | ParamDec { 
        $$ = create_node("VarList", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    };

ParamDec : 
    Specifier VarDec { 
        $$ = create_node("ParamDec", "", -1, @$.first_line); 
        add_child_sibling($$, 2, $1, $2); 
    }
	| error RP {
	}
    | error SEMI {
    };

/* Statements */
CompSt : 
    LC DefList StmtList RC { 
        $$ = create_node("CompSt", "", -1, @$.first_line); 
        add_child_sibling($$, 4, $1, $2, $3, $4); 
    }
    | LC error RC {
    };

StmtList : 
    Stmt StmtList { 
        $$ = create_node("StmtList", "", -1, @$.first_line); 
        add_child_sibling($$, 2, $1, $2); 
    }
    | { 
        $$ = create_node("StmtList", "", -1, @$.first_line); 
    };

Stmt : 
    Exp SEMI { 
        $$ = create_node("Stmt", "", -1, @$.first_line); 
        add_child_sibling($$, 2, $1, $2); 
    }
    | CompSt { 
        $$ = create_node("Stmt", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    }
    | RETURN Exp SEMI { 
        $$ = create_node("Stmt", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE { 
        $$ = create_node("Stmt", "", -1, @$.first_line); 
        add_child_sibling($$, 5, $1, $2, $3, $4, $5); 
    }
    | IF LP Exp RP Stmt ELSE Stmt { 
        $$ = create_node("Stmt", "", -1, @$.first_line); 
        add_child_sibling($$, 7, $1, $2, $3, $4, $5, $6, $7); 
    }
    | WHILE LP Exp RP Stmt { 
        $$ = create_node("Stmt", "", -1, @$.first_line); 
        add_child_sibling($$, 5, $1, $2, $3, $4, $5); 
    }
    | RETURN error SEMI {
    }
    | IF LP error RP Stmt %prec LOWER_THAN_ELSE {
    }
    | WHILE LP error RP Stmt {
    };

/* Local Definitions */
DefList : 
    Def DefList { 
        $$ = create_node("DefList", "", -1, @$.first_line); 
        add_child_sibling($$, 2, $1, $2); 
    }
    | { 
        $$ = create_node("DefList", "", -1, @$.first_line); 
    };

Def : 
    Specifier DecList SEMI { 
        $$ = create_node("Def", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | error SEMI {
    };

DecList : 
    Dec { 
        $$ = create_node("DecList", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    }
    | Dec COMMA DecList { 
        $$ = create_node("DecList", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    };

Dec : 
    VarDec { 
        $$ = create_node("Dec", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    }
    | VarDec ASSIGNOP Exp { 
        $$ = create_node("Dec", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    };

/* Expressions */
Exp : 
    Exp ASSIGNOP Exp { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Exp AND Exp { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Exp OR Exp { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Exp RELOP Exp { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Exp PLUS Exp { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Exp MINUS Exp { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Exp STAR Exp { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Exp DIV Exp { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | LP Exp RP { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | MINUS Exp %prec UMINUS{ 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 2, $1, $2); 
    }
    | NOT Exp { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 2, $1, $2); 
    }
    | ID LP Args RP { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 4, $1, $2, $3, $4); 
    }
    | ID LP RP { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Exp LB Exp RB { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 4, $1, $2, $3, $4); 
    }
    | Exp DOT ID { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | ID { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    }
    | INT { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    }
    | FLOAT { 
        $$ = create_node("Exp", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    }
    | ID LP error RP {
    }
    | Exp LB error RB {
    }
    | LP error RP {
    };

Args : 
    Exp COMMA Args { 
        $$ = create_node("Args", "", -1, @$.first_line); 
        add_child_sibling($$, 3, $1, $2, $3); 
    }
    | Exp { 
        $$ = create_node("Args", "", -1, @$.first_line); 
        add_child_sibling($$, 1, $1); 
    };
%%

int yyerror(const char* msg) {
    error_flag = 1;
    printf("Error type B at Line %d: %s.\n", yylineno, msg);
    return 0;
}