#include "AST.h"

extern int yylineno;
extern int empty_flag;

// create AST node
// token_type: refer to scanner.l or the enum in parser.tab.h (-1 if it is a non-terminal)
ASTNode* create_node(char *name, char *value, int token_type, int lineno){
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    node->name = (char*)malloc(sizeof(name));
    strcpy(node->name, name);

    node->value = (char*)malloc(sizeof(value));
    strcpy(node->value, value);

    node->row_index = lineno;
    node->token_type = token_type;
    node->term_type = token_type == -1 ? 1 : 0; // 0 -> token; 1 -> non-terminal
   
    return node;
}

// add child node to parent node, count is the number of child nodes, ... is the child nodes
void add_child_sibling(ASTNode *parent, const int count,  ...){
    va_list list; // variable argument list
    va_start(list, count); // initialize list
    ASTNode *last_node; 

    for (int i = 0; i < count; i++){
        ASTNode *node = va_arg(list, ASTNode*); // get the next argument of type ASTNode*
        node->parent = parent;
        if (i == 0){
            parent->first_child = node;
            last_node = node;
        }
        else {
            last_node->sibling = node;
            last_node = node;
        }
    }
    va_end(list); // end list
}

// print AST    
void print_AST(ASTNode *node, int indent){
    if (!node)
        return;

    if (empty_flag == 1){
        printf("Empty File!\n");
        return;
    }

    if (node->term_type == 1 && !node->first_child)  // non-terminal & does not have child-node
        return;
    
    for (int i = 0; i < indent; i++){
        printf(" ");
    }

    if (node->term_type == 1){ // non-terminal
        printf("%s (%d)\n", node->name, node->row_index);
    }
    else{  // token
        if (strcmp(node->name, "INT") == 0)
            printf("%s: %d\n", node->name, str_to_int(node->value, node->int_type));
        else if (strcmp(node->name, "FLOAT") == 0)
            printf("%s: %f\n", node->name, atof(node->value));
        else if (strcmp(node->name, "ID") == 0)
            printf("%s: %s\n", node->name, node->value);
        else if (strcmp(node->name, "TYPE") == 0)
            printf("%s: %s\n", node->name, node->value);
        else
            printf("%s\n", node->name);
    }

    ASTNode *tmp = node->first_child;
    while (tmp){
        print_AST(tmp, indent + 2);
        tmp = tmp -> sibling;
    }
}

// convert string to int, according to the type(0: decimal; 1: binary; 2: octal; 3: hexadecimal)
int str_to_int(char *str, int type){
    if (type == 0)
        return atoi(str);
    else if (type == 1)
        return (int)strtol(str, NULL, 2); 
    else if (type == 2)
        return (int)strtol(str, NULL, 8);
    else 
        return (int)strtol(str, NULL, 16);
}