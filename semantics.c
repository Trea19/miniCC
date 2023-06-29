#include "semantics.h"
#include "AST.h"

#define LOOK_FOR_STRUCT 1

static Field_List* var_hash[MAX_HASH_TABLE_LEN];
static Func* func_hash[MAX_HASH_TABLE_LEN];
static Error_List* error_list_head = NULL;
extern int error_flag;

/* hash table */
static int hash_pjw(char* name) {
    unsigned int val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if (i = val & ~0x3fff) {
            val = (val ^ (i >> 12)) & 0x3fff;
        }
    }
    return val;
}

void init_hash_table(){
    for (int i = 0; i < MAX_HASH_TABLE_LEN; i++) {
        var_hash[i] = NULL;
        func_hash[i] = NULL;
    }
    insert_read_func();
    insert_write_func();
}

void insert_read_func(){
    Func* read_func = (Func*)malloc(sizeof(Func));
    // name
    read_func->name = "read"; 
    // return type (int)
    read_func->ret_type = (Type*)malloc(sizeof(Type));
    read_func->ret_type->kind = BASIC;
    read_func->ret_type->basic = INT_TYPE;
    // param (none)
    read_func->param_num = 0;
    // line_num
    read_func->line_num = 0;
    // insert into hash table
    int index = hash_pjw(read_func->name);
    insert_func_hash_table(index, read_func->name, read_func->ret_type, read_func);
}

void insert_write_func(){
    Func* write_func = (Func*)malloc(sizeof(Func));
    // name
    write_func->name = "write";
    // return type (int)
    write_func->ret_type = (Type*)malloc(sizeof(Type));
    write_func->ret_type->kind = BASIC;
    write_func->ret_type->basic = INT_TYPE;
    // param (int)
    write_func->param_num = 1;
    write_func->first_param = malloc(sizeof(Field_List));
    write_func->first_param->is_struct = 0;
    write_func->first_param->wrapped_layer = 1;
    write_func->first_param->type = (Type*)malloc(sizeof(Type));
    write_func->first_param->type->kind = BASIC;
    write_func->first_param->type->basic = INT_TYPE;
    // line_num
    write_func->line_num = 0;
    // insert into hash table
    int index = hash_pjw(write_func->name);
    insert_func_hash_table(index, write_func->name, write_func->ret_type, write_func);
}

// params: hash-index, func-name, return-type, func
Func* insert_func_hash_table(int index, char* func_name, Type* ret_type, Func* func){
    Func* func_ptr = func_hash[index];
    // check if the hash[index] is empty
    if (func_ptr == NULL) {
        func->ret_type = ret_type;
        func->defined = 1;
        func_hash[index] = func;
        return func;
    }
    // not empty, check if the function is declared before
    while (func_ptr != NULL) {
        if (strcmp(func_ptr->name, func_name) == 0) {
            if (func_ptr->defined) { // error: function redefined
                char error_info[MAX_ERROR_INFO_LEN];
                sprintf(error_info, "Error type 4 at Line %d: Redefined function '%s'.\n", func->line_num, func->name);
                add_error_list(4, func->line_num, error_info);
                return func_ptr;
            } 
            else { // function declared before
                if (check_equal_type(func_ptr->ret_type, ret_type) == 0) { // error: return type mismatched
                    char error_info[MAX_ERROR_INFO_LEN];
                    sprintf(error_info, "Error type 19 at Line %d: Inconsistent declaration of function '%s'.\n", func->line_num, func->name);
                    add_error_list(19, func->line_num, error_info);
                    return func_ptr;
                }
                func_ptr->defined = 1;
                return func_ptr;
            }            
        }
        func_ptr = func_ptr->next;
    }
    // insert into hash table
    func->ret_type = ret_type;
    func->defined = 1;
    func->next = func_hash[index];
    func_hash[index] = func;
    return func;
}

Field_List* insert_field_hash_table(int index, char* field_name, Type* typ, ASTNode* field_node, int wrapped_layer, int is_struct){
    Field_List* field_ptr = var_hash[index];
    // check if the hash[index] is empty
    if (field_ptr == NULL) {
        Field_List* new_field = (Field_List*)malloc(sizeof(Field_List));
        strcpy(new_field->name, node->value);
        new_field->type = typ;
        new_field->wrapped_layer = wrapped_layer;
        new_field->is_struct = is_struct;
        new_field->line_num = node->line_num;
        new_field->next = NULL;
        var_hash[index] = new_field;
        return new_field;
    }
    // not empty, check if the field is declared before
    while (field_ptr != NULL) {
        if (strcmp(field_ptr->name, field_name) == 0) {
            // todo: field defined in the outer block can be redefined in the inner block
            char error_info[MAX_ERROR_INFO_LEN];
            sprintf(error_info, "Error type 15 at Line %d: Redefined field '%s'.\n", node->line_num, node->value);
            add_error_list(15, node->line_num, error_info);
            return field_ptr;
        }
        field_ptr = field_ptr->hash_list_index_next;
    }
    // insert into hash table
    Field_List* new_field = (Field_List*)malloc(sizeof(Field_List));
    strcpy(new_field->name, node->value);
    new_field->type = typ;
    new_field->wrapped_layer = wrapped_layer;
    new_field->is_struct = is_struct;
    new_field->next_struct_field = var_hash[index];
    var_hash[index] = new_field;
    return new_field;
}

// params: hash-index, field-name, node, if LOOK_FOR_STRUCT or not
Field_List* find_field_hash_table(int index, char* field_name, ASTNode* node, int look_for_struct){
    Field_List* field_ptr = var_hash[index];
    while (field_ptr != NULL) {
        if (strcmp(field_ptr->name, field_name) == 0) {
            if (look_for_struct) {
                if (field_ptr->is_struct) {
                    return field_ptr;
                }
            }
            else {
                return field_ptr;
            }
        }
        field_ptr = field_ptr->next;
    }
    return NULL;
}

/* semantics_analysis */
void semantics_analysis(ASTNode* root) {
    init_hash_table();
    sem_program(root);
    check_undefined_func();
}

void sem_program(ASTNode* root) {
    sem_ext_def_list(root->first_child);
}

void sem_ext_def_list(ASTNode* root) {
    if (root == NULL) {
        return;
    }
    sem_ext_def(root->first_child);
    sem_ext_def_list(root->first_child->next_sibling);
}

void sem_ext_def(ASTNode* node){
    // get type from semantic specifier
    Type* type = sem_specifier(node->first_child, 0, 0);
    if (strcmp(node->first_child->sibling->name, "ExtDecList") == 0){
        sem_ext_dec_list(node->first_child->sibling, type);
    }
    else if (strcmp(node->first_child->sibling->name, "SEMI") == 0) {
        // do nothing
    }
    else { // FuncDec
        if (strcmp(node->first_child->sibling->sibling->name, "CompSt") == 0){ // define
            Func *func = sem_func_dec(node->first_child->sibling);
            if (func == NULL) {
                return;
            }
            func->ret_type = type;
            insert_func_hash_table(hash_pjw(func->name), func->name, func->ret_type, func);
            sem_comp_st(node->first_child->sibling->sibling, 1, func);
            pop_local_var(1);
        } 
        else { // declare 
            Func *func = sem_func_dec(node->first_child->sibling);
            if (func == NULL) {
                return;
            }
            func->ret_type = type;
            insert_func_hash_table(hash_pjw(func->name), func->name, func->ret_type, func);
            pop_local_var(1);
        }
    }
}

void sem_ext_dec_list(ASTNode* node, Type* type){
    sem_var_dec(node->first_child, type, 0, 0);
    if (node->first_child->next_sibling != NULL) {
        sem_ext_dec_list(node->first_child->sibling->sibling, type);
    }
}

Type* sem_specifier(ASTNode* node, int wrapped_layer, int in_structure) {
    // TYPE
    if (strcmp(node->first_child->name, "TYPE") == 0){
        Type *type = (Type*)malloc(sizeof(Type));
        type->kind = BASIC;
        if (strcmp(node->first_child->value, "int") == 0) {
            type->u.basic = BASIC_INT;
        } else {
            type->u.basic = BASIC_FLOAT;
        }
        return type;
    }
    // StructSpecifier
    return sem_struct_specifier(node->first_child, wrapped_layer, in_structure);
}

Type* sem_struct_specifier(ASTNode* node, int wrapped_layer, int in_structure) {
    if (strcmp(node->first_child->silbing->name, "OptTag") == 0){
        Type* struct_type = (Type*)malloc(sizeof(Type));
        struct_type->kind = STRUCTURE;
        struct_type->u.structure.first_field = sem_def_list(node->first_child->sibling->sibling->sibling, 1, wrapped_layer);
        struct_type->line_num = node->first_child->sibling->sibling->sibling->line_num;
        if (check_duplicate_field(struct_type->u.structure.first_field) == 1) {
            char error_info[MAX_ERROR_INFO_LEN];
            sprintf(error_info, "Error type 15 at Line %d: Redefined field in the structure'%s'.\n", struct_type->line_num, struct_type->u.structure.first_field->name);
            add_error_list(15, struct_type->line_num, error_info);
            return NULL;
        }
        if (node->first_child->sibling->first_child) {
            insert_field_hash_table(hash_pjw(node->first_child->sibling->first_child->value), node->first_child->sibling->first_child->value, struct_type, node->first_child->sibling->first_child, wrapped_layer, 1);
        }
        return struct_type;
    } 
    // strcmp(node->first_child->sibling->name, "Tag") == 0
    ASTNode *tag_node = node->first_child->sibling->first_child;
    FieldList* field = find_field_hash_table(hash_pjw(tag_node->value), tag_node->value, tag_node, LOOK_FOR_STRUCT);
    if (field == NULL) {
        char error_info[MAX_ERROR_INFO_LEN];
        sprintf(error_info, "Error type 17 at Line %d: Undefined structure '%s'.\n", tag_node->line_num, tag_node->value);
        add_error_list(17, tag_node->line_num, error_info);
        return NULL;
    } else {
        return field->type;
    }
}

Field_List* sem_var_dec(ASTNode* node, Type* type, int in_struct, int wrapped_layer){
    if (type == NULL) {
        char error_info[MAX_ERROR_INFO_LEN];
        sprintf(info, "Error type 17 at Line %d: Undefined structure '%s'.\n", node->first_child->line_num, node->first_child->value);
        add_error_list(17, node->first_child->line_num, error_info);
        return NULL;
    }
    if (strcmp(node->first_child->name, "ID") == 0) {
        if (in_struct == 1) {
            Field_List* field = (Field_List*)malloc(sizeof(Field_List));
            strcpy(new_field->name, node->first_child->value);
            field->type = type;
            field->is_struct = 0;
            field->wrapped_layer = wrapped_layer;
            field->line_num = node->first_child->line_num;
            return field;
        } else {
            return insert_field_hash_table(hash_pjw(node->first_child->value), node->first_child->value, type, node->first_child, wrapped_layer, 0);
        }
    } 
    else { // VarDec LB INT RB
        Type* array_type = (Type*)malloc(sizeof(Type));
        array_type->kind = ARRAY;
        array_type->u.array.elem = type;
        array_type->u.array.size = str_to_int(node->first_child->sibling->sibling->value, node->first_child->sibling->sibling->int_type);
        return sem_var_dec(node->first_child, array_type, in_struct, wrapped_layer);
    }
}

Func* sem_func_dec(ASTNode* node){
    Func* func = (Func*)malloc(sizeof(Func));
    strcpy(func->name, node->first_child->value);
    func->line_num = node->first_child->line_num;
    if (strcmp(node->first_child->sibling->sibling->name, "VarList") == 0) {
        func->first_param = sem_var_list(node->first_child->sibling->sibling);
        if (func->first_param == NULL) {
            return NULL;
        } else {
            func->param_num = 1;
            Field_List* param_ptr = func->first_param;
            while (param_ptr->next != NULL) {
                func->param_num++;
                param_ptr = param_ptr->next;
            }
            return func;
        }
    } else { // VarList == NULL
        func->first_param = NULL;
        func->param_num = 0;
        return func;
    }
}

Field_List* sem_var_list(ASTNode* node){
    Field_List* first_param = sem_param_dec(node->first_child);
    if (first_param == NULL) {
        return NULL;
    } else {
        if (node->first_child->sibling != NULL) {
            first_param->next = sem_var_list(node->first_child->sibling->sibling);
        } else {
            first_param->next = NULL;
        }
        return first_param;
    }
}

Field_List* sem_param_dec(ASTNode* node){
    Type* type = sem_specifier(node->first_child, 1, 0);
    if (type == NULL) {
        return NULL;
    } else {
        return sem_var_dec(node->first_child->sibling, type, 0, 1);
    }
}

void sem_comp_st(ASTNode* node, int wrapped_layer, Func* func){
    sem_def_list(node->first_child->sibling, 0, wrapped_layer);
    sem_stmt_list(node->first_child->sibling->sibling, wrapped_layer, func);
    pop_local_var(wrapped_layer);
}

void sem_stmt_list(ASTNode* node, int wrapped_layer, Func* func){
    if (node->first_child == NULL) {
        return;
    } else {
        sem_stmt(node->first_child, wrapped_layer, func);
        sem_stmt_list(node->first_child->sibling, wrapped_layer, func);
    }
}

void sem_stmt(ASTNode* node, int wrapped_layer, Func* func){
    if (strcmp(node->first_child->name, "Exp") == 0) {
        sem_exp(node->first_child);
    } else if (strcmp(node->first_child->name, "CompSt") == 0) {
        sem_comp_st(node->first_child, wrapped_layer + 1, func);
    } else if (strcmp(node->first_child->name, "RETURN") == 0) {
        Type* exp_type = sem_exp(node->first_child->sibling);
        if (check_equal_type(func->ret_type, exp_type) == 0) {
            char error_info[MAX_ERROR_INFO_LEN];
            sprintf(error_info, "Error type 8 at Line %d: Type mismatched for return.\n", node->first_child->line_num);
            add_error_list(8, node->first_child->line_num, error_info);
        }
    } else if (strcmp(node->first_child->name, "IF") == 0) {
        sem_exp(node->first_child->sibling->sibling);
        sem_stmt(node->first_child->sibling->sibling->sibling->sibling, wrapped_layer, func);
        if (node->first_child->sibling->sibling->sibling->sibling->sibling){ // ELSE
            sem_stmt(node->first_child->sibling->sibling->sibling->sibling->sibling->sibling, wrapped_layer, func);
        }
    } else if (strcmp(node->first_child->name, "WHILE") == 0) {
        sem_exp(node->first_child->sibling->sibling);
        sem_stmt(node->first_child->sibling->sibling->sibling->sibling, wrapped_layer, func);
    }
}

Field_List* sem_def_list(ASTNode* node, int in_struct, int wrapped_layer) {
    if (node->first_child == NULL) {
        return NULL;
    } else {
        Field_List* first_def = sem_def(node->first_child, in_struct, wrapped_layer);
        Field_List* cur_ptr = first_def;

        if (first_def == NULL)
            return NULL;
        
        if (in_struct) {
            while(cur_ptr && cur_ptr->next_struct_field != NULL) {
                cur_ptr = cur_ptr->next_struct_field;
            }
            cur_ptr->next_struct_field = sem_def_list(node->first_child->sibling, in_struct, wrapped_layer);
        }
        else {
            sem_def_list(node->first_child->sibling, in_struct, wrapped_layer);
        }
        return first_def;
    }
}

Field_List* sem_def(ASTNode* node, int in_struct, int wrapped_layer) {
    Type* type = sem_specifier(node->first_child, wrapped_layer, in_struct);
    if (type == NULL) {
        return NULL;
    } else {
        return sem_dec_list(node->first_child->sibling, type, in_struct, wrapped_layer);
    }
}

Field_List* sem_dec_list(ASTNode* node, Type* type, int in_struct, int wrapped_layer) {
    Field_List *field = sem_dec(node->first_child, type, in_structure, wrapped_layer);
    if (node->first_child->sibling){
        if (field && in_structure)
            field->next_struct_field = sem_dec_list(node->first_child->sibling->sibling, type, in_structure, wrapped_layer);
        else
            sem_dec_list(node->first_child->sibling->sibling, type, in_structure, wrapped_layer);
    }
    return field;
}

Field_List* sem_dec(ASTNode* node, Type type, int in_struct, int wrapped_layer){
    if (node->first_child->sibling == NULL) {
        return sem_var_dec(node->first_child, type, in_struct, wrapped_layer);
    } 
    else { // VarDec ASSIGNOP Exp
        if (in_struct){
            char info[MAX_ERROR_INFO_LEN];
            sprintf(info, "Initialization of fields in a structure.\n");
            add_error_list(18, info, node->first_child->sibling->line_num);
            Field_List *var_dec_field = sem_var_dec(node->first_child, type, in_struct, wrapped_layer);
            return var_dec_field;
        }
        Field_List *var_dec_field = sem_var_dec(node->first_child, type, in_struct, wrapped_layer);
        if (var_dec_field == NULL)
            return NULL;

        if (check_equal_type(var_dec_field->type, sem_exp(node->first_child->sibling->sibling))){
            return var_dec_field;
        } else {
            char info[MAX_ERROR_INFO_LEN];
                sprintf(info, "Type mismatched for assignment.\n");
                add_error_list(5, info, node->first_child->sibling->line_num);
                return NULL;
        }
    }
}

Type* sem_exp(ASTNode* node){
    //todo
}

/* help function */
void add_error_list(int error_type, int line_num, char* error_info){
    error_flag = 1;
    Error_List* new_error = (Error_List*)malloc(sizeof(Error_List));
       
    new_error->error_type = error_type;
    new_error->line_num = line_num;
    strcpy(new_error->error_info, error_info);

    // insert into error list
    new_error->next = NULL;
    if (error_list_head == NULL) {
        error_list_head = new_error;
    } else {
        Error_List* error_ptr = error_list_head;
        while (error_ptr->next != NULL) {
            error_ptr = error_ptr->next;
        }
        error_ptr->next = new_error;
    }
}

// 0: not equal; 1: equal
int check_equal_type(Type* typ1, Type* typ2){
    if (typ1 == NULL || typ2 == NULL) {
        return 0;
    }
    if (typ1->kind != typ2->kind) {
        return 0;
    }
    if (typ1->kind == BASIC && typ2->kind == BASIC) {
        if (typ1->u.basic != typ2->u.basic) {
            return 0;
        }
        return 1;
    }
    if (typ1->kind == ARRAY && typ2->kind == ARRAY) {
        return check_equal_type(typ1->u.array.elem, typ2->u.array.elem);
    }
    if (typ1->kind == STRUCTURE && typ2->kind == STRUCTURE) {
        Field_List* field_ptr1 = typ1->u.structure.first_field;
        Field_List* field_ptr2 = typ2->u.structure.first_field;

        while(field_ptr1 != NULL && field_ptr2 != NULL) {
            if (check_equal_type(field_ptr1->type, field_ptr2->type) == 0) {
                return 0;
            }
            field_ptr1 = field_ptr1->next;
            field_ptr2 = field_ptr2->next;
        }

        if (field_ptr1 == NULL && field_ptr2 == NULL) {
            return 1;
        } else {
            return 0;
        }
    }
}

// 0: not repeated, 1: repeated
int check_duplicate_field(Type* structure_type){
    Field_List* field_ptr1 = structure_type->u.structure.first_field;
    while (field_ptr1 != NULL) {
        Field_List* field_ptr2 = field_ptr1->next;
        while (field_ptr2 != NULL) {
            if (strcmp(field_ptr1->name, field_ptr2->name) == 0) {
                return 1;
            }
            field_ptr2 = field_ptr2->next;
        }
        field_ptr1 = field_ptr1->next;
    }
    return 0;
}

void check_undefined_func(){
    for (int i = 0; i < MAX_HASH_TABLE_LEN; i ++) {
        if (func_hash[i] != NULL && func_hash[i]->defined == 0) {
            char error_info[MAX_ERROR_INFO_LEN];
            sprintf(error_info, "Error type 18 at Line %d: Undefined function '%s'.\n", func_hash[i]->line_num, func_hash[i]->name);
            add_error_list(18, func_hash[i]->line_num, error_info);
        }
    }
}

// traverse the hash table and pop the local variable(in wrapped_layer)
void pop_local_var(int wrapped_layer){
    for (int i = 0; i < MAX_HASH_TABLE_LEN; i ++) {
        Field_List* field_ptr = var_hash[i];
        if (field_ptr == NULL) {
            continue;
        }
        while (field_ptr != NULL) {
            if (field_ptr->wrapped_layer == wrapped_layer) {
                var_hash[i] = field_ptr->hash_list_index_next;
                field_ptr = var_hash[i];
            } else {
                break;
            }
        }
        while(field_ptr != NULL && field_ptr->hash_list_index_next != NULL) {
            if (field_ptr->hash_list_index_next->wrapped_layer == wrapped_layer) {
                field_ptr->hash_list_index_next = field_ptr->hash_list_index_next->hash_list_index_next;
            } else {
                field_ptr = field_ptr->hash_list_index_next;
            }
        }
    }
}