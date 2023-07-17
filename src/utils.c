#include "utils.h"

extern int yycolumn;
extern int yylineno;
extern int empty_flag;

/* AST */
AST_Node* create_node(char *name, char *value, int token_type, int lineno){
    AST_Node *node = (AST_Node *)malloc(sizeof(AST_Node));
    node->name = (char *)malloc(strlen(name) + 1);
    strncpy(node->name, name, strlen(name) + 1);
    node->value = (char *)malloc(strlen(value) + 1);
    strncpy(node->value, value, strlen(value) + 1);
    node->row_index = lineno;
    node->token_type = token_type;
    node->term_type = token_type == -1 ? 1 : 0;
    
    return node;
}

void add_child_sibling(AST_Node *parent, const int count,  ...){
    va_list ap;
    va_start(ap, count);
    AST_Node *last_node;
    for (int i = 0; i < count; i++){
        AST_Node *node = va_arg(ap, AST_Node*);
        node->parent = parent;
        if (i == 0){
            parent->first_child = node;
            last_node = node;
        }
        else{
            last_node->sibling = node;
            last_node = node;
        }
    }
    va_end(ap);
}

void print_AST(AST_Node *node, int indent){
    if (!node)
        return;
    if (empty_flag == 1){
        printf("Empty file!\n");
        return;
    }
    if (node->term_type == 1 && !node->first_child)
        return;
        
    for (int i = 0; i < indent; i++){
        printf(" ");
    }
    if (node->term_type == 1){
        printf("%s (%d)\n", node->name, node->row_index);
    }
    else{
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
    AST_Node *tmp = node->first_child;
    while (tmp){
        print_AST(tmp, indent + 2);
        tmp = tmp -> sibling;
    }
}

/* help functions */
// str2int
int str_to_int(char* str, int type){
    if (type == 0)
        return atoi(str);
    else if (type == 1) // OCT
        return (int)strtol(str, NULL, 8); 
    else // HEX
        return (int)strtol(str, NULL, 16);
}

// update column in comment blocks
void update_column(char* text){
    int len = strlen(text);
    int chline_flag = 0;
    int i, j;
    for (i = len - 1; i >= 0; i --){
        if (text[i] == '\n'){
            chline_flag = 1;
            break;
        }
    }
    if (chline_flag == 1){
        yycolumn = 1;
        for (j = i; j < len; j++){
            yycolumn++;
        }
    } else {
        yycolumn = len;
    }
}

// hash_pjw
unsigned hash_pjw(char* name){
    unsigned val = 0, i;
    for (; *name; ++name){
        val = (val << 2) + *name;
        if (i = val & ~0x3fff) val = (val ^ (i >> 12)) & 0x3fff;
    }  
    return val;
}

