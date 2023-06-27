#ifndef IR_H
#define IR_H

#include "semantics.h"

struct Operand {
    enum {
        OP_VARIABLE,
        OP_CONSTANT,
        OP_ADDRESS,
        OP_LABEL,
        OP_TEMP,
        OP_FUNC,
        OP_RELOP
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
                RELOP_G,
                RELOP_L,
                RELOP_GE,
                RELOP_LE,
                RELOP_E,
                RELOP_NE
            } kind;
        } relop;
        struct {
            int no;
            int val_kind;
            struct Field_List *field;
            int ref_hidden;
            struct Func *parent_func;
            int size;
            int offset;
        } address;
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
        IR_REF_ASSIGN,
        IR_DEREF_ASSIGN,
        IR_ASSIGN_TO_DEREF,
        IR_GOTO,
        IR_IF,
        IR_RETURN,
        IR_DEC,
        IR_ARG,
        IR_CALL,
        IR_PARAM,
        IR_READ,
        IR_WRITE
    } kind;
    union {
        struct {
            struct Operand *result;
        } nonop;
        struct {
            struct Operand *result, *op;
        } sinop;
        struct {
            struct Operand *result, *op1, *op2;
        } binop;
        struct {
            struct Operand *result, *op1, *relop, *op2;
        } trinop;
    } u;
    struct InterCode *prev;
    struct InterCode *next;
};

// todo

#endif
