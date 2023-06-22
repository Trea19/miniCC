#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

int yylex();
int yyparse();
void yyrestart(FILE*);
int yyerror(const char*);

typedef struct ASTNode {
    int term_type; /* 0 -> token; 1 -> non-terminal*/
    int token_type; /* type of tokens */
    int int_type; /* 0 -> Dec; 1 -> Bin; 2 -> Oct; 3 -> Hex */
    char *name;
    char *value;
    int row_index;
    struct ASTNode *parent;
    struct ASTNode *first_child;
    struct ASTNode *sibling;
} ASTNode;

int str_to_int(char *str, int type);
ASTNode* create_node(char* name, char *value, int token_type, int lineno);
void add_child_sibling(ASTNode *parent, const int count, ...);
void print_AST(ASTNode *node, int indent);

#endif