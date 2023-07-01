#ifndef SEMANTICS_H
#define SEMANTICS_H

#define MAX_NAME_LEN 256
#define MAX_ERROR_INFO_LEN 512
#define MAX_HASH_TABLE_LEN 16384
#define BASIC_INT 1
#define BASIC_FLOAT 2
#define MAX_FIELD_NUM 256

#include "AST.h"

struct Type_ {
    enum {
        BASIC, ARRAY, STRUCTURE
    } kind;

    union {
        int basic;
        struct {
            struct Type_* elem;
            int size;
        } array;
        struct {
            struct Field_List_* first_field;
            //struct Type_* first_flat;
            int size;
        } structure;
    } u;
    struct Type_* next_ret_type; // multiple return type in a CompSt
    struct Type_* next_actual_param; // next actual param
    struct Type_* next_flat; // struct type to non-struct flat type list
    int line_num; // multiple return type in a CompSt
};

struct Field_List_ {
    char name[MAX_NAME_LEN];  
    struct Type_* type; 
    struct Field_List_* next_struct_field;
    struct Field_List_* hash_list_index_next;
    struct Field_List_* next_param;

    int wrapped_layer;
    int is_struct;
    int line_num;
    int offset;
    struct Operand *op;
};

struct Func_ {
    char name[MAX_NAME_LEN];
    struct Type_* ret_type;
    int defined;
    int param_num;
    struct Field_List_* first_param;
    int line_num;
    struct Func_* next; // list in hash_table[index]
    struct Operand *op;
    int stack_size;
    int top_offset;
};

struct Sem_Error_List_ {
    int error_type;
    int line_num;
    char info[MAX_ERROR_INFO_LEN];
    struct Sem_Error_List_* next;
};

typedef struct Type_ Type;
typedef struct Field_List_ Field_List;
typedef struct Func_ Func;
typedef struct Sem_Error_List_ Error_List;

/* hash table */
int hash_pjw(char*);
void init_hash_table();
void insert_read_func();
void insert_write_func();
Func* insert_func_hash_table(int, char*, Type*, Func*);
Func* find_func_hash_table(int, char*);
Field_List* insert_field_hash_table(int, char*, Type*, ASTNode*, int, int);
Field_List* find_field_hash_table(int, char*, ASTNode*, int);


/* semantics_analysis */
void semantics_analysis(ASTNode*);

/* semantics of High-level Definitions */
void sem_program(ASTNode*);
void sem_ext_def_list(ASTNode*);
void sem_ext_def(ASTNode*);
void sem_ext_dec_list(ASTNode*, Type*);

/* semantics of Specifiers */
Type* sem_specifier(ASTNode*, int, int);
Type* sem_struct_specifier(ASTNode*, int, int);

/* semantics of Declarators */
Field_List* sem_var_dec(ASTNode*, Type*, int, int);
Func* sem_fun_dec(ASTNode*);
Field_List* sem_var_list(ASTNode*);
Field_List* sem_param_dec(ASTNode*);

/* semantics of Statements */
void sem_comp_st(ASTNode*, int, Func*);
void sem_stmt_list(ASTNode*, int, Func*);
void sem_stmt(ASTNode*, int, Func*);

/* semantics of Local Definitions */
Field_List* sem_def_list(ASTNode*, int, int);
Field_List* sem_def(ASTNode*, int, int);
Field_List* sem_dec_list(ASTNode*, Type*, int, int);
Field_List* sem_dec(ASTNode*, Type*, int, int);

/* semantics of expressions */
Type* sem_exp(ASTNode*);
Type* sem_args(ASTNode*);

/* helper functions */
//Func* insert_func_dec_hash_table(unsigned, char*, Type*, Func*);
int check_equal_type(Type*, Type*);
int check_duplicate_field(Type*);
int check_equal_params(Field_List*, Type*);
void check_undefined_func();
void pop_local_var(int);

// int check_struct_equal_type_naive(Type*, Type*);
// int check_struct_equal_type(Type*, Type*);


//int check_twofunc_equal_params(Field_List*, Field_List*);


//Type* struct_type_to_list(Field_List*);

/* error report list */
void add_error_list(int, int, char*);
void print_error_list();

/* for debug */
//void print_field_list(int);

#endif
