#include "semantics.h"
#include "debug.h"

#define QUERY_STRUCT_IN_HASH 1
#define NOT_QUERY_STRUCT_IN_HASH 0

static Field_List* var_hash[MAX_HASH_TABLE_LEN];
static Func* func_hash[MAX_HASH_TABLE_LEN];
static Error_List* error_head = NULL;
extern int error_flag;

/* init */
void init_hash_table(){
    for (int i = 0; i < MAX_HASH_TABLE_LEN; i ++){
        var_hash[i] = NULL;
        func_hash[i] = NULL;
    }
    insert_read_write_func("read");
    insert_read_write_func("write");
}

void insert_read_write_func(char* name) {
    Func* func = (Func*)malloc(sizeof(Func));
    func->line_num = 0;
    // ret_type
    Type* type = (Type*)malloc(sizeof(Type));
    type->kind = BASIC;
    type->u.basic = BASIC_INT;
    if (strcmp(name, "read") == 0){
        func->param_size = 0;
    }
    else { // write_func
        func->param_size = 1;
        // param only support int
        Field_List* field = (Field_List*)malloc(sizeof(Field_List));
        field->is_structure = 0;
        field->wrapped_layer = 1;
        field->type = type;
        field->line_num = 0;
        func->first_param = field;
    }
    strcpy(func->name, name);
    insert_func_hash_table(hash_pjw(func->name), func->name, type, func);
}

/* semantics analysis */
void semantics_analysis(AST_Node* root){
    init_hash_table();
    sem_program(root);
}

// Program ::= ExtDefList 
void sem_program(AST_Node* root){
    sem_ext_def_list(root->first_child);
}

// ExtDefList ::= ExtDef ExtDefList | ε
void sem_ext_def_list(AST_Node* node){
    if (node && node->first_child){
        sem_ext_def(node->first_child);
        sem_ext_def_list(node->first_child->sibling);
    }
}

// ExtDef ::= Specifier ExtDecList SEMI |
//            Specifier SEMI |
//            Specifier FunDec CompSt |
//            Specifier FunDec SEMI 
void sem_ext_def(AST_Node* node){
    assert(node && node->first_child);
    Type* type = sem_specifier(node->first_child, 0, 0);
    if (strcmp(node->first_child->sibling->name, "ExtDecList") == 0){
        sem_ext_dec_list(node->first_child->sibling, type);
    }
    else if (strcmp(node->first_child->sibling->name, "SEMI") == 0){
        return;
    }
    else {
        assert(strcmp(node->first_child->sibling->name, "FunDec") == 0);
        Func* func = sem_fun_dec(node->first_child->sibling);
        if (!func)
            return;
        if (strcmp(node->first_child->sibling->sibling->name, "SEMI") == 0){
            insert_func_dec_hash_table(hash_pjw(func->name), func->name, type, func);
        }
        else{
            assert(strcmp(node->first_child->sibling->sibling->name, "CompSt") == 0);
            func->return_type = type;
            insert_func_hash_table(hash_pjw(func->name), func->name, type, func);
            sem_comp_st(node->first_child->sibling->sibling, 1, func);
        }
        pop_local_var(1);
    }
}

// ExtDecList ::= VarDec | VarDec COMMA ExtDecList 
void sem_ext_dec_list(AST_Node* node, Type* type){
    assert(node && node->first_child);
    sem_var_dec(node->first_child, type, 0, 0);
    if (node->first_child->sibling)
        sem_ext_dec_list(node->first_child->sibling->sibling, type);
}

// Specifier ::= TYPE | StructSpecifier 
Type* sem_specifier(AST_Node* node, int wrapped_layer, int in_structure){
    assert(node);
    // TYPE (int | float)
    if (strcmp(node->first_child->name, "TYPE") == 0){
        Type* type = (Type*)malloc(sizeof(Type));
        type->kind = BASIC;
        if (strcmp(node->first_child->value, "int") == 0)
            type->u.basic = BASIC_INT;
        else
            type->u.basic = BASIC_FLOAT;
        return type;
    }
    // StructSpecifier
    assert(strcmp(node->first_child->name, "StructSpecifier") == 0);
    return sem_struct_specifier(node->first_child, wrapped_layer, in_structure);
}

// StructSpecifier ::= STRUCT Tag | 
//                     STRUCT OptTag LC DefList RC |
Type* sem_struct_specifier(AST_Node* node, int wrapped_layer, int in_structure){
    assert(node && node->first_child && strcmp(node->first_child->name, "STRUCT") == 0);
    // STRUCT Tag
    if (strcmp(node->first_child->sibling->name, "Tag") == 0){
        // Tag ::= ID
        AST_Node* tag_node = node->first_child->sibling->first_child;
        unsigned hash_index = hash_pjw(tag_node->value);
        Field_List* field = query_field_hash_table(hash_index, tag_node->value, tag_node, QUERY_STRUCT_IN_HASH);
        if (field)
            return field->type;
        return NULL;
    }
    // STRUCT OptTag LC DefList RC
    assert(strcmp(node->first_child->sibling->name, "OptTag") == 0);
    Type* structure_type = (Type *)malloc(sizeof(Type));
    structure_type->kind = STRUCTURE;
    structure_type->u.structure.first_field = sem_def_list(node->first_child->sibling->sibling->sibling, 1, wrapped_layer);
    structure_type->line_num = node->first_child->sibling->sibling->sibling->row_index;
    if (!check_field_duplicate(structure_type)){
        return NULL;
    }
    if (node->first_child->sibling->first_child){
        unsigned hash_index = hash_pjw(node->first_child->sibling->first_child->value);
        insert_field_hash_table(hash_index, node->first_child->sibling->first_child->value, structure_type, node->first_child->sibling->first_child, wrapped_layer, 1);
    }
    return structure_type;
}

Field_List *sem_var_dec(AST_Node *node, Type *type, int in_structure, int wrapped_layer){
    assert(node && node->first_child);
    if (!type){
        char info[MAX_ERROR_INFO_LEN];
        sprintf(info, "Undefined structure.\n");
        add_error_list(17, info, node->first_child->row_index);
        return NULL;
    }
    if (strcmp(node->first_child->name, "ID") == 0){
        if (in_structure){
            Field_List *new_field = (Field_List *)malloc(sizeof(Field_List));
            new_field->type = type;
            new_field->is_structure = 0;
            new_field->wrapped_layer = wrapped_layer;
            new_field->line_num = node->first_child->row_index;
            strcpy(new_field->name, node->first_child->value);
            return new_field;
        }
        else{
            unsigned hash_index = hash_pjw(node->first_child->value);
            return insert_field_hash_table(hash_index, node->first_child->value, type, node->first_child, wrapped_layer, 0);
        }
    }
    else{
        Type *array_type = (Type *)malloc(sizeof(Type));
        array_type->kind = ARRAY;
        array_type->u.array.size = str_to_int(node->first_child->sibling->sibling->value, node->first_child->sibling->sibling->int_type);
        array_type->u.array.elem = type;
        return sem_var_dec(node->first_child, array_type, in_structure, wrapped_layer);
    }
}

Func *sem_fun_dec(AST_Node *node){
    assert(node && node->first_child);
    Func *func = (Func *)malloc(sizeof(Func));
    strcpy(func->name, node->first_child->value);
    func->line_num = node->first_child->row_index;
    if (strcmp(node->first_child->sibling->sibling->name, "VarList") == 0){
        Field_List *params = sem_var_list(node->first_child->sibling->sibling);
        if (!params)
            return NULL;
        func->first_param = params;
        Field_List *cur = func->first_param;
        func->param_size = 0;
        while (cur){
            func->param_size ++;
            cur = cur->next_param;
        }
        return func;
    }
    else{
        func->param_size = 0;
        func->first_param = NULL;
        return func;
    }
}

Field_List *sem_var_list(AST_Node *node){
    assert(node && node->first_child);
    Field_List *field = sem_param_dec(node->first_child);
    if (!field)
        return NULL;
    if (node->first_child->sibling){
        field->next_param = sem_var_list(node->first_child->sibling->sibling);
    }
    return field;
}

Field_List *sem_param_dec(AST_Node *node){
    assert(node && node->first_child && node->first_child->sibling);
    Type *type = sem_specifier(node->first_child, 1, 0);
    return sem_var_dec(node->first_child->sibling, type, 0, 1);
}

void sem_comp_st(AST_Node *node, int wrapped_layer, Func *func){
    assert(node && node->first_child && node->first_child->sibling && node->first_child->sibling->sibling);
    sem_def_list(node->first_child->sibling, 0, wrapped_layer);
    sem_stmt_list(node->first_child->sibling->sibling, wrapped_layer, func);
    pop_local_var(wrapped_layer);
}

void sem_stmt_list(AST_Node *node, int wrapped_layer, Func *func){
    assert(node);
    if (node->first_child){
        sem_stmt(node->first_child, wrapped_layer, func);
        sem_stmt_list(node->first_child->sibling, wrapped_layer, func);
    }
}

void sem_stmt(AST_Node *node, int wrapped_layer, Func *func){
    assert(node && node->first_child);
    if (strcmp(node->first_child->name, "CompSt") == 0){
        sem_comp_st(node->first_child, wrapped_layer + 1, func);
    }
    if (strcmp(node->first_child->name, "Exp") == 0){
        sem_exp(node->first_child);
    }
    if (strcmp(node->first_child->name, "RETURN") == 0){
        Type *type = sem_exp(node->first_child->sibling);
        if (type && !check_type_equal(type, func->return_type)){
            char info[MAX_ERROR_INFO_LEN];
            sprintf(info, "Type mismatched for return.\n");
            add_error_list(8, info, node->first_child->sibling->row_index);
        }
    }
    if (strcmp(node->first_child->name, "IF") == 0){
        sem_exp(node->first_child->sibling->sibling);
        sem_stmt(node->first_child->sibling->sibling->sibling->sibling, wrapped_layer, func);
        if (node->first_child->sibling->sibling->sibling->sibling->sibling){
            sem_stmt(node->first_child->sibling->sibling->sibling->sibling->sibling->sibling, wrapped_layer, func);
        }
    }
    if (strcmp(node->first_child->name, "WHILE") == 0){
        sem_exp(node->first_child->sibling->sibling);
        sem_stmt(node->first_child->sibling->sibling->sibling->sibling, wrapped_layer, func);
    }
}

Field_List *sem_def_list(AST_Node *node, int in_structure, int wrapped_layer){
    assert(node);
    if (node->first_child){
        Field_List *field = sem_def(node->first_child, in_structure, wrapped_layer);
        Field_List *cur = field; // insert field_lists of a def after the front def
        if (!cur && in_structure)
            return NULL;
        if (in_structure){
            while(cur && cur->next_struct_field){
                cur = cur->next_struct_field;
            }
            cur->next_struct_field = sem_def_list(node->first_child->sibling, in_structure, wrapped_layer);
        }
        else
            sem_def_list(node->first_child->sibling, in_structure, wrapped_layer);
        return field;
    }
    return NULL;
}

Field_List *sem_def(AST_Node *node, int in_structure, int wrapped_layer){
    assert(node);
    Type *type = sem_specifier(node->first_child, wrapped_layer, in_structure);
    return sem_dec_list(node->first_child->sibling, type, in_structure, wrapped_layer);
}

Field_List *sem_dec_list(AST_Node *node, Type *type, int in_structure, int wrapped_layer){
    assert(node && node->first_child);
    Field_List *field = sem_dec(node->first_child, type, in_structure, wrapped_layer);
    if (node->first_child->sibling){
        if (field && in_structure)
            field->next_struct_field = sem_dec_list(node->first_child->sibling->sibling, type, in_structure, wrapped_layer);
        else
            sem_dec_list(node->first_child->sibling->sibling, type, in_structure, wrapped_layer);
    }
    return field;
}

Field_List *sem_dec(AST_Node *node, Type *type, int in_structure, int wrapped_layer){
    assert(node && node->first_child);
    if (node->first_child->sibling){
        if (in_structure){
            char info[MAX_ERROR_INFO_LEN];
            sprintf(info, "initialization of fields in a structure.\n");
            add_error_list(15, info, node->first_child->sibling->row_index);
            Field_List *var_dec_field = sem_var_dec(node->first_child, type, in_structure, wrapped_layer);
            return var_dec_field;
        }
        Field_List *var_dec_field = sem_var_dec(node->first_child, type, in_structure, wrapped_layer);
        if (var_dec_field){
            if (check_type_equal(var_dec_field->type, sem_exp(node->first_child->sibling->sibling))){
                return var_dec_field;
            }
            else{
                char info[MAX_ERROR_INFO_LEN];
                sprintf(info, "Type mismatched for assignment.\n");
                add_error_list(5, info, node->first_child->sibling->row_index);
                return NULL;
            }
        }
        return NULL;
    }
    else{
        Field_List *var_dec_field = sem_var_dec(node->first_child, type, in_structure, wrapped_layer);
        return var_dec_field;
    }
}

Type *sem_exp(AST_Node *node){
    assert(node && node->first_child);
    if (strcmp(node->first_child->name, "Exp") == 0){
        if (strcmp(node->first_child->sibling->name, "ASSIGNOP") == 0){
            Type *type1 = sem_exp(node->first_child);
            Type *type2 = sem_exp(node->first_child->sibling->sibling);
            if (!(type1 && type2))
                return NULL;
            if (check_type_equal(type1, type2)){
                //check left-value
                AST_Node *child_node = node->first_child;
                if ((strcmp(child_node->first_child->name, "ID") == 0 && !child_node->first_child->sibling) 
                    || (strcmp(child_node->first_child->name, "Exp") == 0 && strcmp(child_node->first_child->sibling->name, "LB") == 0) 
                    || (strcmp(child_node->first_child->name, "Exp") == 0 && strcmp(child_node->first_child->sibling->name, "DOT") == 0)){
                    return type1;    
                }
                else{
                    char info[MAX_ERROR_INFO_LEN];
                    sprintf(info, "The left-hand side of an assignment must be a variable.\n");
                    add_error_list(6, info, node->first_child->sibling->row_index);
                    return NULL;
                }
            }
            char info[MAX_ERROR_INFO_LEN];
            sprintf(info, "Type mismatched for assignment.\n");
            add_error_list(5, info, node->first_child->sibling->row_index);
            return NULL;
        }
        else if (strcmp(node->first_child->sibling->name, "AND") == 0
            || strcmp(node->first_child->sibling->name, "OR") == 0){
            Type *type1 = sem_exp(node->first_child);
            Type *type2 = sem_exp(node->first_child->sibling->sibling);
            if (!(type1 && type2))
                return NULL;
            if (check_type_equal(type1, type2)){
                //only INT can do logical computation
                if (type1->kind == BASIC && type1->u.basic == BASIC_INT){
                    return type1;
                }
                else{
                    char info[MAX_ERROR_INFO_LEN];
                    sprintf(info, "Only INT varaibles can do logical computation.\n");
                    add_error_list(7, info, node->first_child->sibling->row_index);
                    return NULL;
                }
            }
            char info[MAX_ERROR_INFO_LEN];
            sprintf(info, "Type mismatched for logical operation.\n");
            add_error_list(7, info, node->first_child->sibling->row_index);
            return NULL;
        }
        else if (strcmp(node->first_child->sibling->name, "PLUS") == 0
            || strcmp(node->first_child->sibling->name, "MINUS") == 0
            || strcmp(node->first_child->sibling->name, "STAR") == 0
            || strcmp(node->first_child->sibling->name, "DIV") == 0
            || strcmp(node->first_child->sibling->name, "RELOP") == 0){
            Type *type1 = sem_exp(node->first_child);
            Type *type2 = sem_exp(node->first_child->sibling->sibling);
            if (!(type1 && type2))
                return NULL;
            if (check_type_equal(type1, type2)){
                if (strcmp(node->first_child->sibling->name, "RELOP") == 0){
                    Type *type = (Type *)malloc(sizeof(Type));
                    type->kind = BASIC;
                    type->u.basic = BASIC_INT;
                    return type;
                }
                else{
                    return type1;
                }
            }
            char info[MAX_ERROR_INFO_LEN];
            sprintf(info, "Type mismatched for arithmatic operation.\n");
            add_error_list(7, info, node->first_child->sibling->row_index);
            return NULL;
        }
        else if (strcmp(node->first_child->sibling->name, "LB") == 0 && strcmp(node->first_child->sibling->sibling->name, "Exp") == 0){
            Type *index_type = sem_exp(node->first_child->sibling->sibling);
            if (!index_type)
                return NULL;
            if (index_type->kind == BASIC && index_type->u.basic == BASIC_INT){
                Type *type = sem_exp(node->first_child);
                if (!type)
                    return NULL;
                if (type->kind == ARRAY){
                    return type->u.array.elem;
                }
                else{
                    char info[MAX_ERROR_INFO_LEN];
                    sprintf(info, "The variable is not an array.\n");
                    add_error_list(10, info, node->first_child->row_index);
                    return NULL; 
                }
            }
            else{
                char info[MAX_ERROR_INFO_LEN];
                sprintf(info, "The size of array can only be an integer.\n");
                add_error_list(12, info, node->first_child->sibling->row_index);
                return NULL;                
            }
        }
        else if (strcmp(node->first_child->sibling->name, "DOT") == 0){
            Type *type = sem_exp(node->first_child);
            if (!type)
                return NULL;
            if (type->kind == STRUCTURE){
                Field_List *cur = type->u.structure.first_field;
                while (cur){
                    if (strcmp(cur->name, node->first_child->sibling->sibling->value) == 0){
                        return cur->type;
                    }
                    cur = cur->next_struct_field;
                }
                char info[MAX_ERROR_INFO_LEN];
                sprintf(info, "Non-existent field '%s'.\n", node->first_child->sibling->sibling->value);
                add_error_list(14, info, node->first_child->sibling->sibling->row_index);
                return NULL;    
            }
            char info[MAX_ERROR_INFO_LEN];
            sprintf(info, "Illegal use of '.'.\n");
            add_error_list(13, info, node->first_child->sibling->row_index);
            return NULL; 
        }
    }
    else if (strcmp(node->first_child->name, "LP") == 0){
        return sem_exp(node->first_child->sibling);
    }
    else if (strcmp(node->first_child->name, "MINUS") == 0){
        return sem_exp(node->first_child->sibling);
    }
    else if (strcmp(node->first_child->name, "NOT") == 0){
        Type *type = sem_exp(node->first_child->sibling);
        if (!type)
            return NULL;
        if (type->kind == BASIC && type->u.basic == BASIC_INT){
            Type *new_type = (Type *)malloc(sizeof(Type));
            new_type->kind = BASIC;
            new_type->u.basic = BASIC_INT;
            return new_type;
        }
        char info[MAX_ERROR_INFO_LEN];
        sprintf(info, "Only INT varaibles can do logical computation.\n");
        add_error_list(7, info, node->first_child->sibling->row_index);
        return NULL;
    }
    else if (strcmp(node->first_child->name, "ID") == 0){
        if (!node->first_child->sibling){
            unsigned hash_index = hash_pjw(node->first_child->value);
            Field_List *field = query_field_hash_table(hash_index, node->first_child->value, node->first_child, NOT_QUERY_STRUCT_IN_HASH);
            if (!field){
                char info[MAX_ERROR_INFO_LEN];
                sprintf(info, "Undefined variable '%s'.\n", node->first_child->value);
                add_error_list(1, info, node->first_child->row_index);
                return NULL;
            }
            return field->type;
        }
        unsigned hash_index = hash_pjw(node->first_child->value);
        Func *func = query_func_hash_table(hash_index, node->first_child->value);
        Field_List *var = query_field_hash_table(hash_index, node->first_child->value, NULL, NOT_QUERY_STRUCT_IN_HASH);
        
        if (!func || func->defined == 0){
            if (!func && var){
                char info[MAX_ERROR_INFO_LEN];
                sprintf(info, "'%s' is not a function.\n", node->first_child->value);
                add_error_list(11, info, node->first_child->sibling->row_index);
                return NULL;
            }
            else{
                char info[MAX_ERROR_INFO_LEN];
                sprintf(info, "Undefined function '%s'\n", node->first_child->value);
                add_error_list(2, info, node->first_child->sibling->row_index);
                return NULL;
            }
        }
        if (strcmp(node->first_child->sibling->sibling->name, "Args") == 0){
            Type *true_params_type = sem_args(node->first_child->sibling->sibling);
            if (!true_params_type)
                return NULL;
            if (check_params_equal(func->first_param, true_params_type)){
                return func->return_type;
            }
            return NULL;
        }
        else{
            assert(strcmp(node->first_child->sibling->sibling->name, "RP") == 0);
            if (func->param_size == 0){
                return func->return_type;
            }
            char info[MAX_ERROR_INFO_LEN];
            sprintf(info, "Function arguments are not applicable.\n");
            add_error_list(9, info, node->first_child->row_index);
            return NULL;
        }
    }
    else if (strcmp(node->first_child->name, "INT") == 0){
        Type *type = (Type *)malloc(sizeof(Type));
        type->kind = BASIC;
        type->u.basic = BASIC_INT;
        return type;
    }
    else if (strcmp(node->first_child->name, "FLOAT") == 0){
        Type *type = (Type *)malloc(sizeof(Type));
        type->kind = BASIC;
        type->u.basic = BASIC_FLOAT;
        return type;
    }
    return NULL;
}

Type *sem_args(AST_Node *node){
    Type *type = sem_exp(node->first_child);
    if (!type)
        return NULL;
    type->line_num = node->first_child->row_index;
    if (node->first_child->sibling){
        type->next_actual_param = sem_args(node->first_child->sibling->sibling);
    }
    return type;
}

Field_List *query_field_hash_table(unsigned hash_index, char *str, AST_Node *node, int look_for_structure){
    Field_List *field_now = var_hash[hash_index];
    while(field_now != NULL){
        if (strcmp(field_now->name, str) == 0 && field_now->is_structure == look_for_structure)
            return field_now;
        field_now = field_now->hash_list_next;
    }
    return NULL;
}

Field_List *insert_field_hash_table(unsigned hash_index, char *str, Type *type, AST_Node *node, int wrapped_layer, int is_structure){
    Field_List *new_field;
    Field_List *cur = var_hash[hash_index];
    while (cur){
        if (strcmp(cur->name, str) == 0){
            if (cur->wrapped_layer >= wrapped_layer){
                char info[MAX_ERROR_INFO_LEN];
                if (is_structure){
                    sprintf(info, "redefinition of '%s'.\n", node->value);
                    add_error_list(16, info, node->row_index);
                }
                else{
                    sprintf(info, "redefinition of '%s'.\n", node->value);
                    add_error_list(3, info, node->row_index);
                }
                return NULL;
            }
            else{
                break;
            }
        }
        cur = cur->hash_list_next;
    }
    new_field = malloc(sizeof(Field_List));
    strcpy(new_field->name, node->value);
    new_field->type = type;
    new_field->wrapped_layer = wrapped_layer;
    new_field->is_structure = is_structure;
    new_field->line_num = node->row_index;
    new_field->next_struct_field = var_hash[hash_index];
    var_hash[hash_index] = new_field;
    return new_field;            
}

Func *insert_func_hash_table(unsigned hash_index, char *str, Type *return_type, Func *func){
    Func *cur = func_hash[hash_index];
    if (cur == NULL){
        func->return_type = return_type;
        func->defined = 1;
        func_hash[hash_index] = func;
        return func;
    }
    while (cur){
        if (strcmp(cur->name, str) == 0){
            if (cur->defined){
                char info[MAX_ERROR_INFO_LEN];
                sprintf(info, "Redefined function '%s'.\n", func->name);
                add_error_list(4, info, func->line_num);
                return NULL;
            }
            else{
                if (!check_type_equal(return_type, cur->return_type) || !check_func_params_equal(func->first_param, cur->first_param)){
                    char info[MAX_ERROR_INFO_LEN];
                    sprintf(info, "Inconsistent declaration of function '%s'.\n", func->name);
                    add_error_list(19, info, func->line_num);
                    return NULL;
                }
                cur->defined = 1;
                return cur;
            }
        }
        cur = cur->next; 
    }
    // if the list is not null and there are functions whose names are equal with func
    func->return_type = return_type;
    func->defined = 1;
    func->next = func_hash[hash_index];
    func_hash[hash_index] = func;
    return func;
}

Func *query_func_hash_table(unsigned hash_index, char *str){
    Func *cur = func_hash[hash_index];
    if (cur == NULL)
        return NULL;
    while (cur){
        if (strcmp(str, cur->name) == 0)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

Func *insert_func_dec_hash_table(unsigned hash_index, char *str, Type *return_type, Func *func){
    Func *cur = func_hash[hash_index];
    while (cur){
        if (strcmp(cur->name, str) == 0){
            if (!check_type_equal(return_type, cur->return_type) || !check_func_params_equal(func->first_param, cur->first_param)){
                char info[MAX_ERROR_INFO_LEN];
                sprintf(info, "Inconsistent declaration of function '%s'.\n", func->name);
                add_error_list(19, info, func->line_num);
                return NULL;
            }
            return cur;
        }
        cur = cur->next;
    }
    func->defined = 0;
    func->return_type = return_type;
    func->next = func_hash[hash_index];
    func_hash[hash_index] = func;
    return func;
}

// 0: not equal, 1: equal
int check_type_equal(Type* type1, Type* type2){
    if (!type1 || !type2)
        return 0;
    if (type1->kind != type2->kind)
        return 0;
    if (type1->kind == BASIC){
        if (type1->u.basic == type2->u.basic)
            return 1;
        else
            return 0;
    }
    else if (type1->kind == ARRAY){
        return check_type_equal(type1->u.array.elem, type2->u.array.elem);
    }
    else if (type1->kind == STRUCTURE){
        return check_struct_equal_type(type1, type2);
    }
    else{
        assert(0);
    }
    return 0;
}

// 0: not equal, 1: equal
int check_field_duplicate(Type* structure_type){
    assert(structure_type && structure_type->kind == STRUCTURE);
    Field_List *cur = structure_type->u.structure.first_field;
    while (cur){
        Field_List *cur2 = cur->next_struct_field;
        while (cur2){
            if (strcmp(cur->name, cur2->name) == 0){
                char info[MAX_ERROR_INFO_LEN];
                sprintf(info, "Redefined field '%s'.\n", cur->name);
                add_error_list(15, info, cur->line_num);
                return 0;
            }
            cur2 = cur2->next_struct_field;
        }
        cur = cur->next_struct_field;
    }
    return 1;
}

// 0: not equal, 1: equal
int check_params_equal(Field_List* field_list, Type* type){
    Field_List *cur = field_list;
    Type *cur_type = type;
    while (cur && cur_type){
        if (!check_type_equal(cur->type, cur_type)){
            char info[MAX_ERROR_INFO_LEN];
            sprintf(info, "Function arguments are not applicable.\n");
            add_error_list(9, info, cur->line_num);
            return 0;
        }
        cur = cur->next_param;
        cur_type = cur_type->next_actual_param;
    }
    if (cur || cur_type){
        char info[MAX_ERROR_INFO_LEN];
        sprintf(info, "Function arguments are not applicable.\n");
        add_error_list(9, info, cur->line_num);
        return 0;
    }
    return 1;
}

// 0: not equal, 1: equal
int check_func_params_equal(Field_List* field1, Field_List* field2){
    Field_List *cur1 = field1;
    Field_List *cur2 = field2;
    while (cur1 && cur2){
        if (!check_type_equal(cur1->type, cur2->type)){
            return 0;
        }
        cur1 = cur1->next_param;
        cur2 = cur2->next_param;
    }
    if (cur1 || cur2)
        return 0;
    return 1;
}

// 0: not equal, 1: equal
int check_struct_equal_type(Type *type1, Type *type2){
    Field_List *cur1 = type1->u.structure.first_field;
    Field_List *cur2 = type2->u.structure.first_field;

    while (cur1 && cur2){
        if (!check_type_equal(cur1->type, cur2->type))
            return 0;
        cur1 = cur1->next_struct_field;
        cur2 = cur2->next_struct_field;
    }
    if (cur1 || cur2){
        return 0;
    }
    return 1;
}

void pop_local_var(int wrapped_layer){
    for (int i = 0; i < MAX_HASH_TABLE_LEN; i ++){
        Field_List* cur = var_hash[i];
        Field_List* pre = NULL;
        while (cur){
            if (cur->wrapped_layer == wrapped_layer){
                if (pre){
                    pre->hash_list_next = cur->hash_list_next;
                    cur = pre->hash_list_next;
                }
                else{
                    var_hash[i] = cur->hash_list_next;
                    cur = var_hash[i];
                }
            }
            else{
                pre = cur;
                cur = cur->hash_list_next;
            }
        }
    }
}

void add_error_list(int type, char *info, int line_num){
    error_flag = 1;
    Error_List* error = (Error_List*)malloc(sizeof(Error_List));
    error->type = type;
    strcpy(error->info, info);
    error->line_num = line_num;
    error->next = NULL;
    if (!error_head){
        error_head = error;
    }
    else{
        error->next = error_head;
        error_head = error;
    }
}

void print_error_list(){
    Error_List* cur = error_head;
    while (cur){
        printf("Error type %d at Line %d: %s", cur->type, cur->line_num, cur->info);
        cur = cur->next;
    }
}

