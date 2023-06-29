#include "semantics.h"
#include "AST.h"

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
Func* insert_func_hash_table(unsigned index, char* func_name, Type* ret_type, Func* func){
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