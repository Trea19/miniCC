#ifndef IR_H
#define IR_H

#include "semantics.h"

struct Operand {
    enum {
        OP_VARIABLE,
        OP_LABEL,
        OP_TEMP,
        OP_CONSTANT,
        OP_FUNC,
        OP_RELOP, 
        OP_ADDRESS,
    } kind;
    union {
        struct {
            int no;
            struct Field_List *field;
            struct Func *parent_func;
            int offset;
        } var;
        struct {
            int no;
        } label;
        struct {
            int no;
            struct Func *parent_func;
            int offset;
        } temp;
        struct {
            int val;
        } constant;
        struct {
            struct Func *func;
        } func;
        struct {
            enum {
                RELOP_G, // >
                RELOP_L, // <
                RELOP_GE, // >=
                RELOP_LE, // <=
                RELOP_E, // == 
                RELOP_NE // !=
            } kind;
        } relop;
        struct {
            int no;
            int val_kind;
            struct Field_List* field;
            int ref_hidden;
            struct Func *parent_func;
            int size;
            int offset;
        } addr;
    } u;
    struct Operand *next_arg;
    struct Operand *next_list_op;
};

struct InterCode {
    enum {
        IR_LABEL,
        IR_FUNCTION,
        IR_ASSIGN,
        IR_ADD,
        IR_SUB,
        IR_MUL,
        IR_DIV,
        IR_REF_ASSIGN, // *x = y
        IR_DEREF_ASSIGN, // x = *y
        IR_ASSIGN_TO_DEREF, // x = &y
        IR_GOTO,
        IR_IF,
        IR_RETURN,
        IR_DEC, // allocate space for array or structure
        IR_ARG, // push argument
        IR_CALL,
        IR_PARAM, 
        IR_READ,
        IR_WRITE
    } kind;
    union {
        struct {
            struct Operand *result;
        } no_op;
        struct {
            struct Operand *result, *op;
        } sin_op;
        struct {
            struct Operand *result, *op1, *op2;
        } bin_op;
        struct {
            struct Operand *result, *op1, *relop, *op2;
        } tri_op;
    } u;
    struct InterCode* prev;
    struct InterCode* next;
};

typedef struct Operand Operand;
typedef struct InterCode InterCode;

/* ir functions */
void ir_generate(ASTNode*);
void ir_init_hash_table();
void ir_insert_read_func();
void ir_insert_write_func();
void ir_program(ASTNode*);
InterCode* ir_ext_def_list(ASTNode*);
InterCode* ir_ext_def(ASTNode*);
Type* ir_specifier(ASTNode*, int, int);
Type* ir_struct_specifier(ASTNode*, int, int);
Field_List* ir_var_dec(ASTNode*, Type*, int, int);
InterCode* ir_fun_dec(ASTNode*, Type*);
Field_List* ir_var_list(ASTNode*);
Field_List* ir_param_dec(ASTNode*);
InterCode* ir_comp_st(ASTNode*, int);
InterCode* ir_stmt_list(ASTNode*, int);
InterCode* ir_stmt(ASTNode*, int);
InterCode* ir_cond(ASTNode*, Operand*, Operand*); //node, label_true, label_false
InterCode* ir_def_list(ASTNode*, int); // node, wrapped_layer
InterCode* ir_def(ASTNode*, int); // node, wrapped_layer
InterCode* ir_dec_list(ASTNode*, Type*, int); // node, type, wrapped_layer
InterCode* ir_dec(ASTNode*, Type*, int); // node, type, wrapped_layer
InterCode* ir_exp(ASTNode*, Operand*); // node, place
Type* ir_exp_type(ASTNode*);
InterCode* ir_args(ASTNode*);



// struct Field_List *ir_def_list_structure(ASTNode* node, int wrapped_layer);
// struct Field_List *ir_def_structure(ASTNode* node, int wrapped_layer);
// struct Field_List *ir_dec_list_structure(ASTNode* node, struct Type *type, int wrapped_layer);
// struct Field_List *ir_dec_structure(ASTNode* node, struct Type *type, int wrapped_layer);

/* help functions */

InterCode* ir_link(InterCode*, InterCode*);
Field_List* ir_insert_field_hash_table(int, char*, Type*, ASTNode*, int, int);
Field_List* ir_find_field_hash_table(int, char*, ASTNode*, int);
Func* ir_insert_func_hash_table(int, char*, Type*, Func*);
Func* ir_find_func_hash_table(int, char*);
int build_size_offset(Type*);
int size_of_array_type(Type*);
Operand* make_var(Field_List*);
Operand* make_addr(Operand*, int);
Operand* make_temp();
Operand* make_func(Func*); // func
Operand* make_relop(int); // kind
Operand* make_label();
Operand* make_fall_label();
int all_constant(ASTNode*);
int get_constant(ASTNode* );
Operand* make_constant(int);
InterCode* make_ir(int, Operand*, Operand*, Operand*2, Operand*); // (kind, res, op1, op2,relop)
Operand* get_id(ASTNode*);
int is_id(ASTNode*);
Operand* relop_reverse(Operand*);


// int size_of_array(ASTNode*);

// void ir_to_file(FILE *fp);
// char *show_ir(InterCode* code);
// char *show_op(Operand *op);
// void replace_label(int new_label_no, int old_label_no);
// void replace_temp(int new_temp_no, int old_temp_no);
// void post_optimize();

#endif // IR_H