#ifndef SEMANTICS_H
#define SEMANTICS_H

#define MAX_NAME_LEN 256
#define MAX_HASH_TABLE_LEN 16384
#define BASIC_INT 1
#define BASIC_FLOAT 2
#define MAX_ERROR_INFO_LEN 512
#define MAX_FIELD_NUM 256

#include "utils.h"
#include "ir.h"

struct Type {
    enum {
        BASIC, ARRAY, STRUCTURE
    } kind;
    union {
        int basic;
        struct {
            struct Type* elem;
            int size;
        } array;
        struct {
            struct Field_List* first_field;
            int size;
        } structure;
    } u;
    struct Type* next_ret_type; 
    struct Type* next_actual_param; 
    struct Type* next_flat; // struct type to non-struct flat type list ?
    int line_num;
};

struct Field_List {
    char name[MAX_NAME_LEN];
    struct Type* type;
    struct Field_List* next_struct_field;
    struct Field_List* hash_list_next;
    struct Field_List* next_param;
    int wrapped_layer;
    int is_structure;
    int line_num;
    int offset;
    struct Operand* op;
};

struct Func {
    char name[MAX_NAME_LEN];
    struct Type* return_type;
    int defined;
    int param_size;
    struct Field_List* first_param;
    int line_num;
    struct Func* next; // list in hash table
    struct Operand* op;
    int stack_size;
    int top_offset;
};

struct Sem_Error_List {
    int type; // error_type
    int line_num;
    char info[MAX_ERROR_INFO_LEN];
    struct Sem_Error_List* next;
};

typedef struct Func Func;
typedef struct Type Type;
typedef struct Field_List Field_List;
typedef struct Sem_Error_List Error_List;

/* init */
void init_hash_table();
void insert_read_write_func(char *);

/* semantics analysis */
void semantics_analysis(AST_Node *root);
void sem_program(AST_Node *); 
void sem_ext_def_list(AST_Node *);
void sem_ext_def(AST_Node *);
void sem_ext_dec_list(AST_Node *, Type *);
Type* sem_specifier(AST_Node *, int, int);
Type* sem_struct_specifier(AST_Node *, int, int);
Field_List *sem_var_dec(AST_Node *, Type *, int, int);
Func *sem_fun_dec(AST_Node *);
Field_List *sem_var_list(AST_Node *);
Field_List *sem_param_dec(AST_Node *);
void sem_comp_st(AST_Node *, int, Func *);
void sem_stmt_list(AST_Node *, int, Func *);
void sem_stmt(AST_Node *, int, Func *);
Field_List *sem_def_list(AST_Node *, int, int);
Field_List *sem_def(AST_Node *, int, int);
Field_List *sem_dec_list(AST_Node *, Type *, int, int);
Field_List *sem_dec(AST_Node *, Type *, int, int);
Type *sem_exp(AST_Node *);
Type *sem_args(AST_Node *);

/* help functions */
Field_List *insert_field_hash_table(unsigned, char *, Type *, AST_Node *, int, int);
Field_List *query_field_hash_table(unsigned, char *, AST_Node *, int);
Func *insert_func_hash_table(unsigned, char *, Type *, Func *);
Func *query_func_hash_table(unsigned, char *);
Func *insert_func_dec_hash_table(unsigned, char *, Type *, Func *);
int check_equal_type(Type *, Type *);
int check_struct_equal_type_naive(Type *, Type *);
int check_duplicate_field(Type *);
int check_equal_params(Field_List *, Type *);
int check_twofunc_equal_params(Field_List *, Field_List *);
void pop_local_var(int);
void add_error_list(int, char *, int);
void print_error_list();

#endif

