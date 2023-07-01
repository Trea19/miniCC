#include "ir.h"
#include "assert.h"
#include "semantics.h"

#define MAX_ARG_LIST_HEAD_NUM 512
#define STACK_SIZE (8 * 1024)
#define LOOKUP_STRUCT 1

InterCode* ir_head = NULL;
static Operand* op_head = NULL;
Operand* read_op = NULL;
Operand* write_op = NULL;
static Operand* fall_label = NULL;

static int label_no; // global label number
static int temp_no; // global temp number
static int var_no; // global variable number

static int arg_list_head_index;
static Operand* arg_list_head[MAX_ARG_LIST_HEAD_NUM];
static Field_List* var_hash[MAX_HASH_TABLE_LEN];
static Func* func_hash[MAX_HASH_TABLE_LEN];

void ir_generate(ASTNode* root){
    ir_init_hash_table();
    ir_program(root);
    // todo: code optimization
}

void ir_init_hash_table(){
    label_no = 1;
    temp_no = 1;
    var_no = 1;
    arg_list_head_index = 0;
    for (int i = 0; i < MAX_ARG_LIST_HEAD_NUM; i++){
        arg_list_head[i] = NULL;
    }
    for (int i = 0; i < MAX_HASH_TABLE_LEN; i++){
        var_hash[i] = NULL;
        func_hash[i] = NULL;
    }
    ir_insert_read_func();
    ir_insert_write_func();
    fall_label = make_fall_label();
}

void ir_insert_read_func(){
    Func *func = malloc(sizeof(Func));
    func->line_num = 0;
    func->top_offset = 0;
    func->stack_size = STACK_SIZE;
    // ret_type
    Type *type = malloc(sizeof(type));
    type->kind = BASIC;
    type->u.basic = BASIC_INT;
    // param
    func->param_num = 0;
    // func_name
    char *name = "read";
    strncpy(func->name, name, strlen(name));
    ir_insert_func_hash_table(hash_pjw(func->name), func->name, type, func);
}

void ir_insert_write_func(){
    Func *func = malloc(sizeof(Func));
    func->line_num = 0;
    func->top_offset = 0;
    func->stack_size = STACK_SIZE;
    // ret_type & param_type
    Type *type = malloc(sizeof(type));
    type->kind = BASIC;
    type->u.basic = BASIC_INT;
    // param
    func->param_num = 1;
    Field_List *field = malloc(sizeof(Field_List));
    field->is_struct = 0;
    field->wrapped_layer = 1;
    field->type = type;
    field->line_num = 0;
    func->first_param = field;
    // func_name
    char *name = "write";
    strncpy(func->name, name, strlen(name));
    ir_insert_func_hash_table(hash_pjw(func->name), func->name, type, func);
}

void ir_program(ASTNode* root){
    ir_ext_def_list(root->first_child);
}

InterCode* ir_ext_def_list(ASTNode* node){
    if (node == NULL)
        return NULL;
    InterCode* code1 = ir_ext_def(node->first_child);
    InterCode* code2 = ir_ext_def_list(node->first_child->sibling);
    return ir_link(code1, code2);
}

InterCode* ir_ext_def(ASTNode* node){
    // don't support global variable
    if (strcmp(node->first_child->sibling->name, "SEMI") == 0) {
        ir_specifier(node->first_child, 0, 0); // params: wrapped_layer = 0, in_struct = 0
        return NULL;
    }
    if (strcmp(node->first_child->sibling->name, "FunDec") == 0) {
        if (strcmp(node->first_child->sibling->sibling->name, "SEMI") == 0) { 
            printf("Warning: function declaration is not supported.\n");
            return NULL; // todo: function declaration
        }
        Type *ret_type = ir_specifier(node->first_child, 0, 0); // params: wrapped_layer = 0, in_struct = 0
        InterCode* fun_code = ir_fun_dec(node->first_child->sibling, ret_type);
        InterCode* comp_code = ir_comp_st(node->first_child->sibling->sibling, 1); // params: wrapped_layer = 1
        return ir_link(fun_code, comp_code);
    }
    return NULL;
}

Type* ir_specifier(ASTNode* node, int wrapped_layer, int in_struct){
    if (node == NULL)
        return NULL;
    // int or float
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
    return ir_struct_specifier(node->first_child, wrapped_layer, in_struct);
}

Type* ir_struct_specifier(ASTNode* node, int wrapped_layer, int in_struct){
    if (node == NULL)
        return NULL;
    // StructSpecifier: STRUCT OptTag LC DefList RC
    if (strcmp(node->first_child->sibling->name, "OptTag") == 0){
        Type* struct_type = (Type*)malloc(sizeof(Type));
        struct_type->kind = STRUCTURE;
        struct_type->u.structure.first_field = ir_def_list(node->first_child->sibling->sibling->sibling, wrapped_layer);
        struct_type->u.structure.size = build_size_offset(struct_type);
        struct_type->line_num = node->first_child->sibling->sibling->sibling->line_num;
        if (node->first_child->sibling->first_child){
            int hash_index = hash_pjw(node->first_child->sibling->first_child->value);
            ir_insert_field_hash_table(hash_index, node->first_child->sibling->first_child->value, struct_type, node->first_child->sibling->first_child, wrapped_layer, 1); // params: is_struct
        }
        return struct_type;
    }
    // StructSpecifier: STRUCT Tag
    else {
        ASTNode* tag_node = node->first_child->sibling->first_child;
        int hash_index = hash_pjw(tag_node->value);
        Field_List* field = ir_find_field_hash_table(hash_index, tag_node->value, tag_node, LOOKUP_STRUCT);
        if (field == NULL)
            return NULL;
        return field->type;
    }
}

Field_List* ir_var_dec(ASTNode* node, Type* type, int in_structure, int wrapped_layer) {
    if (node == NULL) 
        return NULL;
    if (strcmp(node->first_child->name, "ID") == 0){ // VarDec: ID
        if (in_structure){ // in structure
            Field_List* new_field = (Field_List*)malloc(sizeof(Field_List));
            new_field->type = type;
            new_field->is_struct = 0;
            new_field->wrapped_layer = wrapped_layer;
            new_field->line_num = node->first_child->line_num;
            strcpy(new_field->name, node->first_child->value);
            return new_field;
        }
        else{ 
            int hash_index = hash_pjw(node->first_child->value);
            return ir_insert_field_hash_table(hash_index, node->first_child->value, type, node->first_child, wrapped_layer, 0);
        }
    }
    else{ // VarDec: VarDec LB INT RB
        Type* array_type = (Type*)malloc(sizeof(Type));
        array_type->kind = ARRAY;
        array_type->u.array.size = str_to_int(node->first_child->sibling->sibling->value, node->first_child->sibling->sibling->int_type);
        array_type->u.array.elem = type;
        return ir_var_dec(node->first_child, array_type, in_structure, wrapped_layer);
    }
}

InterCode* ir_fun_dec(ASTNode* node, Type* ret_type){
    if (node == NULL)
        return NULL;
    Func* func = (Func *)malloc(sizeof(Func));
    strncpy(func->name, node->first_child->value, strlen(node->first_child->value));
    func->line_num = node->first_child->line_num;
    func->top_offset = 0;
    func->stack_size = STACK_SIZE;
    func->ret_type = ret_type;
    func->defined = 1;
    // ID LP VarList RP
    if (strcmp(node->first_child->sibling->sibling->name, "VarList") == 0){
       Field_List *params = ir_var_list(node->first_child->sibling->sibling);
        if (params == NULL)
            return NULL;
        
        func->first_param = params;
        Field_List* func_ptr = func->first_param;
        func->param_num = 0;
        while (func_ptr != NULL){
            func->param_num ++;
            func_ptr = func_ptr->next_param;
        }

        ir_insert_func_hash_table(hash_pjw(func->name), func->name, ret_type, func);
        InterCode* code1 = make_ir(IR_FUNCTION, func->op, NULL, NULL, NULL);
        func_ptr = func->first_param;
        while (func_ptr != NULL) {
            code1 = ir_link(code1, make_ir(IR_PARAM, func_ptr->op, NULL, NULL, NULL));
            func_ptr = func_ptr->next_param;
        }
        return code1;
    }
    else { // ID LP RP
        func->param_num = 0;
        func->first_param = NULL;
        ir_insert_func_hash_table(hash_pjw(func->name), func->name, ret_type, func);
        return make_ir(IR_FUNCTION, func->op, NULL, NULL, NULL);
    }

}

Field_List* ir_var_list(ASTNode* node) {
    if (node == NULL)
        return NULL;
    Field_List* field = ir_param_dec(node->first_child);
    if (field == NULL)
        return NULL;
    if (node->first_child->sibling != NULL){
        field->next_param = ir_var_list(node->first_child->sibling->sibling);
    }
    return field;
}

Field_List* ir_param_dec(ASTNode* node) {
    if (node == NULL)
        return NULL;
    Type *type = ir_specifier(node->first_child, 1, 0);
    return ir_var_dec(node->first_child->sibling, type, 0, 1);
}

InterCode* ir_comp_st(ASTNode* node, int wrapped_layer) {
    if (node == NULL)
        return NULL;
    InterCode* code1 = ir_def_list(node->first_child->sibling, wrapped_layer);
    InterCode* code2 = ir_stmt_list(node->first_child->sibling->sibling, wrapped_layer);
    return ir_link(code1, code2);
}

InterCode* ir_stmt_list(ASTNode* node, int wrapped_layer){
    if (node == NULL)
        return NULL;
    if (node->first_child){
        InterCode* code1 = ir_stmt(node->first_child, wrapped_layer);
        InterCode* code2 = ir_stmt_list(node->first_child->sibling, wrapped_layer);
        return ir_link(code1, code2);
    }
    return NULL;
}

InterCode *ir_stmt(ASTNode* node, int wrapped_layer) {
    if (node == NULL)
        return NULL;
    
    // Exp SEMI
    if (strcmp(node->first_child->name, "Exp") == 0) {
        return ir_exp(node->first_child, NULL);
    }
    // CompSt
    if (strcmp(node->first_child->name, "CompSt") == 0) {
        return ir_comp_st(node->first_child, wrapped_layer + 1);
    }
    // RETURN Exp SEMI
    if (strcmp(node->first_child->name, "RETURN") == 0) {
        // return constant
        if (all_constant(node->first_child->sibling)) {
            return make_ir(IR_RETURN, make_constant(get_constant(node->first_child->sibling)), NULL, NULL, NULL);
        }
        // return ID
        if (is_id(node->first_child->sibling)) {
            return make_ir(IR_RETURN, get_id(node->first_child->sibling), NULL, NULL, NULL);
        }
        // return Exp
        Operand *t1 = make_temp();
        InterCode *code1 = ir_exp(node->first_child->sibling, t1);
        InterCode *code2 = make_ir(IR_RETURN, t1, NULL, NULL, NULL);
        return ir_link(code1, code2);
    }
    if (strcmp(node->first_child->name, "IF") == 0) {
        if (node->first_child->sibling->sibling->sibling->sibling->sibling) { // IF LP Exp RP Stmt ELSE [label1] Stmt [label2]
            Operand* label1 = make_label(); 
            Operand* label2 = make_label();
            InterCode* code1 = ir_cond(node->first_child->sibling->sibling, fall_label, label1);
            InterCode* code2 = ir_stmt(node->first_child->sibling->sibling->sibling->sibling, wrapped_layer);
            InterCode* code3 = ir_stmt(node->first_child->sibling->sibling->sibling->sibling->sibling->sibling, wrapped_layer);
            return
                ir_link(
                    ir_link(
                        ir_link(
                            ir_link(
                                ir_link(
                                    code1,
                                    code2
                                ),
                                make_ir(IR_GOTO, label2, NULL, NULL, NULL)
                            ),
                            make_ir(IR_LABEL, label1, NULL, NULL, NULL)
                        ),
                        code3
                    ),
                    make_ir(IR_LABEL, label2, NULL, NULL, NULL)
                );
        }
        else { // IF LP Exp RP Stmt [label1]
            Operand *label1 = make_label();
            InterCode *code1 = ir_cond(node->first_child->sibling->sibling, fall_label, label1);
            InterCode *code2 = ir_stmt(node->first_child->sibling->sibling->sibling->sibling, wrapped_layer);
            return 
                ir_link(
                    ir_link(
                        code1,
                        code2
                    ), 
                    make_ir(IR_LABEL, label1, NULL, NULL, NULL)
                );
        }
    }
    // WHILE LP [label1] Exp RP Stmt [label2]
    if (strcmp(node->first_child->name, "WHILE") == 0) {
        Operand *label1 = make_label();
        Operand *label2 = make_label();
        InterCode *code1 = ir_cond(node->first_child->sibling->sibling, fall_label, label2);
        InterCode *code2 = ir_stmt(node->first_child->sibling->sibling->sibling->sibling, wrapped_layer);
        return
            ir_link(
                ir_link(
                    ir_link(
                        ir_link(
                            make_ir(IR_LABEL, label1, NULL, NULL, NULL),
                            code1
                        ),
                        code2
                    ),
                    make_ir(IR_GOTO, label1, NULL, NULL, NULL)
                ),
                make_ir(IR_LABEL, label2, NULL, NULL, NULL)
            );
    }
    return NULL;
}

InterCode* ir_def_list(ASTNode* node, int wrapped_layer) {
    if (node == NULL)
        return NULL;
    if (node->first_child != NULL){
        InterCode* code1 = ir_def(node->first_child, wrapped_layer);
        InterCode* code2 = ir_def_list(node->first_child->sibling, wrapped_layer);
        return ir_link(code1, code2);
    }
    return NULL;
}

InterCode* ir_def(ASTNode* node, int wrapped_layer) {
    if (node == NULL)
        return NULL;
    Type* type = ir_specifier(node->first_child, wrapped_layer, 0);
    return ir_dec_list(node->first_child->sibling, type, wrapped_layer);
}

InterCode* ir_dec_list(ASTNode* node, Type* type, int wrapped_layer) {
    if (node == NULL)
        return NULL;
    InterCode* code1 = ir_dec(node->first_child, type, wrapped_layer);
    if (node->first_child->sibling != NULL){ // Dec COMMA DecList
        InterCode* code2 = ir_dec_list(node->first_child->sibling->sibling, type, wrapped_layer);
        return ir_link(code1, code2);
    }
    return code1;
}

InterCode* ir_dec(ASTNode* node, Type* type, int wrapped_layer) {
    if (node == NULL || type == NULL)
        return NULL;

    if (node->first_child->sibling){ // VarDec ASSIGNOP Exp
        Field_List *variable = ir_var_dec(node->first_child, type, 0, wrapped_layer);
        if (all_constant(node->first_child->sibling->sibling)) { // Exp is constant
            return make_ir(IR_ASSIGN, variable->op, make_constant(get_constant(node->first_child->sibling->sibling)), NULL, NULL);
        }
        if (is_id(node->first_child->sibling->sibling)) { // Exp is id
            return make_ir(IR_ASSIGN, variable->op, get_id(node->first_child->sibling->sibling), NULL, NULL);
        }
        // Exp is not constant or id
        Operand* t1 = make_temp();
        InterCode* code1 = ir_exp(node->first_child->sibling->sibling, t1); // t1 = Exp
        InterCode* code2 = make_ir(IR_ASSIGN, variable->op, t1, NULL, NULL); // variable = t1
        return ir_link(code1, code2);
    }
    else{ // VarDec
        Field_List *variable = ir_var_dec(node->first_child, type, 0, wrapped_layer);
        if (variable->type->kind == STRUCTURE) { // struct
            variable->op = make_addr(variable->op, 0); // ref_hidden = 0
            int size = variable->type->u.structure.size;
            Operand *op_size = make_constant(size);
            InterCode *code = make_ir(IR_DEC, variable->op, op_size, NULL, NULL); // DEC struct size
            return code;
        }
        else if (variable->type->kind == ARRAY) { // array
            variable->op = make_addr(variable->op, 0);
            int size = size_of_array_type(variable->type);
            Operand* op_size = make_constant(size);
            InterCode *code = make_ir(IR_DEC, variable->op, op_size, NULL, NULL); // DEC array size
            return code;
        }
        return NULL;
    }
}

InterCode* ir_exp(ASTNode* node, Operand* place) {
    if (node == NULL)
        return NULL;
    if (all_constant(node)) { // Exp is constant
        if (place == NULL)
            return NULL;
        Operand* constant = make_constant(get_constant(node));
        return make_ir(IR_ASSIGN, place, constant, NULL, NULL);
    }

    if (strcmp(node->first_child->name, "Exp") == 0 || strcmp(node->first_child->name, "NOT") == 0) {
        // Exp ...
        if (strcmp(node->first_child->name, "Exp") == 0) { 
            if (strcmp(node->first_child->sibling->name, "ASSIGNOP") == 0){ // Exp ASSIGNOP Exp
                Field_List* variable = NULL;
                // ID ASSIGNOP Exp
                if (strcmp(node->first_child->first_child->name, "ID") == 0 && node->first_child->first_child->sibling == NULL) {
                    // variable = left-value ID
                    variable = ir_find_field_hash_table(hash_pjw(node->first_child->first_child->value), node->first_child->first_child->value, node->first_child->first_child, 0);
                    assert(variable);
                    // Exp2 is constant
                    if (all_constant(node->first_child->sibling->sibling)) { 
                        if (place == NULL) {
                            return make_ir(IR_ASSIGN, variable->op, make_constant(get_constant(node->first_child->sibling->sibling)), NULL, NULL);
                        }
                        else {
                            return ir_link(make_ir(IR_ASSIGN, variable->op, make_constant(get_constant(node->first_child->sibling->sibling)), NULL, NULL), 
                                           make_ir(IR_ASSIGN, place, variable->op, NULL, NULL));
                        }
                    }

                    // Exp2 is ID
                    if (is_id(node->first_child->sibling->sibling)) { 
                        if (place == NULL) {
                            return make_ir(IR_ASSIGN, variable->op, get_id(node->first_child->sibling->sibling), NULL, NULL);
                        }
                        else {
                            return ir_link(make_ir(IR_ASSIGN, variable->op, get_id(node->first_child->sibling->sibling), NULL, NULL), 
                                           make_ir(IR_ASSIGN, place, variable->op, NULL, NULL));
                        }
                    }

                    // Exp2 is not constant or ID
                    Operand* t1 = make_temp();
                    InterCode* code1 = ir_exp(node->first_child->sibling->sibling, t1); // t1 = Exp2
                    InterCode* code2 = NULL;
                    if (place == NULL) {
                        code2 = make_ir(IR_ASSIGN, variable->op, t1, NULL, NULL); // variable = t1
                    }
                    else {
                        code2 = ir_link(make_ir(IR_ASSIGN, variable->op, t1, NULL, NULL), 
                                        make_ir(IR_ASSIGN, place, variable->op, NULL, NULL));
                    }
                    return ir_link(code1, code2);
                }
                // Exp (Arrays or Structures) ASSIGNOP Exp
                else {
                    Operand *t1 = make_temp();
                    Operand *t2 = make_temp();
                    InterCode* code1 = ir_exp(node->first_child, t1); // t1 = Exp1
                    InterCode* code2 = ir_exp(node->first_child->sibling->sibling, t2); // t2 = Exp2
                    InterCode* code3 = make_ir(IR_ASSIGN, t1, t2, NULL, NULL); // t1 = t2
                    if (place == NULL) {
                        return ir_link(ir_link(code1, code2), code3);  
                    }
                    else {
                        InterCode* code4 = make_ir(IR_ASSIGN, place, t1, NULL, NULL); // place = t1
                        return ir_link(ir_link(ir_link(code1, code2), code3), code4);
                    }
                }
            }
            // Exp PLUS Exp || Exp MINUS Exp || Exp STAR Exp || Exp DIV Exp
            else if (strcmp(node->first_child->sibling->name, "PLUS") == 0
                || strcmp(node->first_child->sibling->name, "MINUS") == 0
                || strcmp(node->first_child->sibling->name, "STAR") == 0
                || strcmp(node->first_child->sibling->name, "DIV") == 0){
                int kind = 0;
                if (strcmp(node->first_child->sibling->name, "PLUS") == 0) {
                    kind = IR_ADD;
                }
                else if (strcmp(node->first_child->sibling->name, "MINUS") == 0) {
                    kind = IR_SUB;
                }
                else if (strcmp(node->first_child->sibling->name, "STAR") == 0) {
                    kind = IR_MUL;
                }
                else {
                    kind = IR_DIV;
                }
                // Exp1 is constant and Exp2 is constant
                if (all_constant(node->first_child) && all_constant(node->first_child->sibling->sibling)) {
                    if (place == NULL)
                        return NULL;
                    int result = get_constant(node->first_child);
                    switch (kind) {
                        case IR_ADD: result += get_constant(node->first_child->sibling->sibling); break;
                        case IR_SUB: result -= get_constant(node->first_child->sibling->sibling); break;
                        case IR_MUL: result *= get_constant(node->first_child->sibling->sibling); break;
                        case IR_DIV: result /= get_constant(node->first_child->sibling->sibling);  break;
                        default: assert(0);
                    }
                    // place = make_constant(result);
                    return make_ir(IR_ASSIGN, place, make_constant(result), NULL, NULL);
                }
                // not both constant
                Operand *t1 = NULL;
                Operand *t2 = NULL;
                InterCode *code1 = NULL;
                InterCode *code2 = NULL;
                // case 1: Exp1 is constant
                if (all_constant(node->first_child)) {
                    Operand *constant = make_constant(get_constant(node->first_child));
                    if (is_id(node->first_child->sibling->sibling)) // Exp2 is ID
                        t1 = get_id(node->first_child->sibling->sibling); // t1 = Exp2
                    else { // Exp2 is not constant or ID
                        t1 = make_temp();
                        code1 = ir_exp(node->first_child->sibling->sibling, t1); // t1 = Exp2
                    }
                    if (place != NULL)
                        code2 = make_ir(kind, place, constant, t1, NULL);
                    return ir_link(code1, code2);
                }
                // case 2: Exp2 is constant
                else if (all_constant(node->first_child->sibling->sibling)){
                    if (is_id(node->first_child)) { // Exp1 is ID
                        t1 = get_id(node->first_child);
                    }
                    else { // Exp1 is not constant or ID
                        t1 = make_temp();
                        code1 = ir_exp(node->first_child, t1);
                    }
                    // Exp2 is constant
                    Operand *constant = make_constant(get_constant(node->first_child->sibling->sibling));
                    if (place != NULL)
                        code2 = make_ir(kind, place, t1, constant, NULL);
                    return ir_link(code1, code2);
                }
                // case3: both not constant
                // Exp1
                if (is_id(node->first_child)) {
                    t1 = get_id(node->first_child);
                } else {
                    t1 = make_temp();
                    code1 = ir_exp(node->first_child, t1);
                }
                // Exp2
                if (is_id(node->first_child->sibling->sibling)) {
                    t2 = get_id(node->first_child->sibling->sibling);
                } else {
                    t2 = make_temp();
                    code2 = ir_exp(node->first_child->sibling->sibling, t2);
                }
                InterCode *code3 = NULL;
                if (place != NULL)
                    code3 = make_ir(kind, place, t1, t2, NULL);
                return ir_link(ir_link(code1, code2), code3);
            }
            // Exp LB Exp RB, array
            else if (strcmp(node->first_child->sibling->name, "LB") == 0 && strcmp(node->first_child->sibling->sibling->name, "Exp") == 0){
                Operand *t1 = NULL;
                InterCode *code1 = NULL;
                // Exp1 is ID
                if (strcmp(node->first_child->first_child->name, "ID") == 0 && node->first_child->first_child->sibling == NULL) {
                    Field_List *variable = ir_find_field_hash_table(hash_pjw(node->first_child->first_child->value), node->first_child->first_child->value, node->first_child->first_child, 0);
                    assert(variable);
                    t1 = variable->op; // t1 = &variable
                }
                else { // Exp1 is not ID
                    t1 = make_temp();
                    code1 = ir_exp(node->first_child, t1);
                }
                // Exp2 is constant
                if (all_constant(node->first_child->sibling->sibling)) {
                    Type *type = ir_exp_type(node);
                    assert(type);
                    Operand *array_offset = make_constant(get_constant(node->first_child->sibling->sibling) * size_of_array_type(type));
                    InterCode *code2;
                    if (type->kind != BASIC && place != NULL){
                        code2 = make_ir(IR_ADD, place, t1, array_offset, NULL); // place = t1 + array_offset
                    }
                    // tobe_translated_Exp ASSiGNOP Exp, and tobe_translated_Exp is array
                    else if (type->kind == BASIC && node->sibling && strcmp(node->sibling->name, "ASSIGNOP") == 0){ 
                        Operand *t4 = make_temp();
                        Operand *t5 = make_temp();
                        if (place == NULL)
                            code2 = ir_link(
                                        ir_link(
                                            make_ir(IR_ADD, t4, t1, array_offset, NULL),  // t4 = t1 + array_offset
                                            ir_exp(node->sibling->sibling, t5) // t5 = Exp2
                                        ), 
                                        make_ir(IR_ASSIGN_TO_DEREF, t4, t5, NULL, NULL) // *t4 = t5
                                    );
                        else
                            code2 = ir_link(
                                        ir_link(
                                            ir_link(
                                                make_ir(IR_ADD, t4, t1, array_offset, NULL), // t4 = t1 + array_offset
                                                ir_exp(node->sibling->sibling, t5) // t5 = Exp2
                                            ), 
                                            make_ir(IR_ASSIGN_TO_DEREF, t4, t5, NULL, NULL) // *t4 = t5
                                        ), 
                                        make_ir(IR_DEREF_ASSIGN, place, t4, NULL, NULL) // place = *t4
                                    );
                    }
                    else if (place != NULL){
                        Operand *t4 = make_temp();
                        code2 = ir_link(make_ir(IR_ADD, t4, t1, array_offset, NULL),  // t4 = t1 + array_offset
                                        make_ir(IR_DEREF_ASSIGN, place, t4, NULL, NULL)); // place = *t4
                    }
                    return ir_link(code1, code2);
                }
                // Exp2 is not constant
                Operand* t2 = NULL; // t2 = &Exp2
                InterCode* code2 = NULL;
                if (is_id(node->first_child->sibling->sibling)) { // Exp2 is ID
                    t2 = get_id(node->first_child->sibling->sibling);
                }
                else {
                    t2 = make_temp();
                    code2 = ir_exp(node->first_child->sibling->sibling, t2);
                }
                Operand* t3 = make_temp(); // t3 = t2 * size_of_array
                Type *type = ir_exp_type(node);
                assert(type);
                InterCode* code3 = make_ir(IR_MUL, t3, t2, make_constant(size_of_array_type(type)), NULL); // t3 = t2 * size_of_array
                
                InterCode* code4 = NULL; // get the address of array element
                if (type->kind != BASIC && place != NULL){
                    code4 = make_ir(IR_ADD, place, t1, t3, NULL); // place = t1 + t3
                }
                // tobe_translated_Exp ASSiGNOP Exp, and tobe_translated_Exp is array
                else if (node->sibling && strcmp(node->sibling->name, "ASSIGNOP") == 0){
                    Operand *t4 = make_temp();
                    Operand *t5 = make_temp();
                    if (place == NULL)
                        code4 = ir_link(
                                    ir_link(
                                        make_ir(IR_ADD, t4, t1, t3, NULL),  // t4 = t1 + t3, address of array element
                                        ir_exp(node->sibling->sibling, t5)  // t5 = Exp2
                                    ), 
                                    make_ir(IR_ASSIGN_TO_DEREF, t4, t5, NULL, NULL) // *t4 = t5
                                );
                    else
                        code4 = ir_link(
                                    ir_link(
                                        ir_link(
                                            make_ir(IR_ADD, t4, t1, t3, NULL), 
                                            ir_exp(node->sibling->sibling, t5)
                                        ), 
                                        make_ir(IR_ASSIGN_TO_DEREF, t4, t5, NULL, NULL)
                                    ), 
                                    make_ir(IR_DEREF_ASSIGN, place, t4, NULL, NULL) // place = *t4
                                );
                }
                else if (place != NULL){
                    Operand *t4 = make_temp();
                    code4 = ir_link(make_ir(IR_ADD, t4, t1, t3, NULL),  // t4 = t1 + t3, address of array element
                                    make_ir(IR_DEREF_ASSIGN, place, t4, NULL, NULL)); // place = *t4
                }
                return
                    ir_link(
                        ir_link(
                            ir_link(
                                code1,
                                code2
                            ),
                            code3
                        ),
                        code4
                    );
            }
            // Exp is structure
            // Exp -> Exp DOT ID
            else if (strcmp(node->first_child->sibling->name, "DOT") == 0){
                Type *structure_type = ir_exp_type(node->first_child);
                Type *type = ir_exp_type(node);
                assert(structure_type && structure_type->kind == STRUCTURE);
                assert(type);
                Operand *t1 = make_temp();
                InterCode *code1 = ir_exp(node->first_child, t1); // t1 = &Exp
                Field_List *cur_field = structure_type->u.structure.first_field;
                while (cur_field != NULL) {
                    if (strcmp(cur_field->name, node->first_child->sibling->sibling->value) == 0) {
                        break;
                    }
                    cur_field = cur_field->next_struct_field;
                }
                assert(cur_field);
                Operand *offset = make_constant(cur_field->offset);
                InterCode *code2 = NULL;
                if (type->kind != BASIC && place != NULL) {
                    code2 = make_ir(IR_ADD, place, t1, offset, NULL); // place = t1 + offset
                }
                else if (node->sibling && strcmp(node->sibling->name, "ASSIGNOP") == 0) {
                    Operand *t2 = make_temp();
                    Operand *t3 = make_temp();
                    if (place == NULL) {
                        code2 = ir_link(
                                    ir_link(
                                        make_ir(IR_ADD, t2, t1, offset, NULL), 
                                        ir_exp(node->sibling->sibling, t3)
                                    ), 
                                    make_ir(IR_ASSIGN_TO_DEREF, t2, t3, NULL, NULL)
                                );
                    }
                    else {
                        code2 = ir_link(
                                    ir_link(
                                        ir_link(
                                            make_ir(IR_ADD, t2, t1, offset, NULL), 
                                            ir_exp(node->sibling->sibling, t3)
                                        ), 
                                        make_ir(IR_ASSIGN_TO_DEREF, t2, t3, NULL, NULL)
                                    ), 
                                    make_ir(IR_DEREF_ASSIGN, place, t2, NULL, NULL)
                                );
                    }
                }
                else if (place != NULL){
                    Operand *t2 = make_temp();
                    code2 = ir_link(make_ir(IR_ADD, t2, t1, offset, NULL), 
                                    make_ir(IR_DEREF_ASSIGN, place, t2, NULL, NULL));
                }
                return ir_link(code1, code2);
            }
        }
        // Exp AND Exp || Exp OR Exp || NOT Exp || Exp RELOP Exp
        if ((strcmp(node->first_child->name, "Exp") == 0 && strcmp(node->first_child->sibling->name, "AND") == 0)
            || (strcmp(node->first_child->name, "Exp") == 0 && strcmp(node->first_child->sibling->name, "OR") == 0)
            || (strcmp(node->first_child->name, "Exp") == 0 && strcmp(node->first_child->sibling->name, "RELOP") == 0)
            || (strcmp(node->first_child->name, "NOT") == 0)){
            if (place == NULL)
                return NULL;
            Operand *label1 = make_label();
            Operand *label2 = make_label();
            InterCode *code0 = make_ir(IR_ASSIGN, place, make_constant(0), NULL, NULL); // place = 0
            InterCode *code1 = ir_cond(node, label1, label2); // if (Exp) goto label1 else goto label2
            InterCode *code2 = ir_link(
                                    make_ir(IR_LABEL, label1, NULL, NULL, NULL), // label1:
                                    make_ir(IR_ASSIGN, place, make_constant(1), NULL, NULL) // place = 1
                                );
            return
                ir_link(
                    ir_link(
                        ir_link(
                            code0,
                            code1
                        ),
                        code2
                    ),
                    make_ir(IR_LABEL, label2, NULL, NULL, NULL)
                );
        }
        assert(0);
        return NULL;
    }
    // Exp -> LP Exp RP
    else if (strcmp(node->first_child->name, "LP") == 0){
        return ir_exp(node->first_child->sibling, place);
    }
    // Exp -> MINUS Exp
    else if (strcmp(node->first_child->name, "MINUS") == 0){
        if (all_constant(node->first_child->sibling)) {
            if (place != NULL) {
                return make_ir(IR_ASSIGN, place, make_constant(-get_constant(node->first_child->sibling)), NULL, NULL);
            }
            return NULL;
        }
        Operand *t1 = make_temp();
        InterCode *code1 = ir_exp(node->first_child->sibling, t1);
        InterCode *code2 = NULL;
        if (place != NULL)
            code2 = make_ir(IR_SUB, place, make_constant(0), t1, NULL);
        return ir_link(code1, code2);
    }
    // Exp -> ID || ID LP RP || ID LP Args RP
    else if (strcmp(node->first_child->name, "ID") == 0){
        // ID
        if (node->first_child->sibling == NULL) {
            Field_List *variable = ir_find_field_hash_table(hash_pjw(node->first_child->value), node->first_child->value, node->first_child, 0);
            assert(variable);
            if (place != NULL)
                return make_ir(IR_ASSIGN, place, variable->op, NULL, NULL);
            return NULL;
        }
        // ID LP Args RP
        if (strcmp(node->first_child->sibling->sibling->name, "Args") == 0){
            Func *function = ir_find_func_hash_table(hash_pjw(node->first_child->value), node->first_child->value);
            arg_list_head[++ arg_list_head_index] = NULL;
            InterCode *code1 = ir_args(node->first_child->sibling->sibling);
            if (strcmp(function->name, "write") == 0) {
                assert(arg_list_head[arg_list_head_index]);
                return ir_link(code1, make_ir(IR_WRITE, arg_list_head[arg_list_head_index], NULL, NULL, NULL));
            }
            Operand *cur = arg_list_head[arg_list_head_index];
            arg_list_head_index --;
            InterCode *code2 = NULL; // args
            while (cur != NULL) {
                code2 = ir_link(code2, make_ir(IR_ARG, cur, NULL, NULL, NULL));
                cur = cur->next_arg;
            }
            InterCode *code3 = NULL;
            if (place != NULL) {
                code3 = make_ir(IR_CALL, place, function->op, NULL, NULL); // place = call function
            }
            else {
                Operand *tmp = make_temp();
                code3 = make_ir(IR_CALL, tmp, function->op, NULL, NULL); // tmp = call function
            }
            return ir_link(ir_link(code1, code2), code3);
        }
        else { // ID LP RP
            Func *function = ir_find_func_hash_table(hash_pjw(node->first_child->value), node->first_child->value);
            if (strcmp(function->name, "read") == 0) {
                if (place != NULL)
                    return make_ir(IR_READ, place, NULL, NULL, NULL);
                else {
                    Operand *tmp = make_temp();
                    return make_ir(IR_READ, tmp, NULL, NULL, NULL);
                }
            }
            if (place != NULL)
                return make_ir(IR_CALL, place, function->op, NULL, NULL);
            else {
                Operand *tmp = make_temp();
                return make_ir(IR_CALL, tmp, function->op, NULL, NULL);
            }
        }
    }
    else if (strcmp(node->first_child->name, "INT") == 0){
        if (place != NULL) {
            Operand *value = make_constant(atoi(node->first_child->value));
            return make_ir(IR_ASSIGN, place, value, NULL, NULL);
        }
        return NULL;
    }
    else if (strcmp(node->first_child->name, "FLOAT") == 0){
        assert(0); // undefined
        return NULL;
    }
    return NULL;
}

// Args -> Exp COMMA Args | Exp
InterCode* ir_args(ASTNode *node) {
    if (node == NULL) {
        return NULL;
    }
    Operand *t1 = NULL;
    InterCode *code1 = NULL;
    if (all_constant(node->first_child)) {
        t1 = make_constant(get_constant(node->first_child));
    }
    else if (is_id(node->first_child)) {
        t1 = get_id(node->first_child);
    }
    else {
        t1 = make_temp();
        code1 = ir_exp(node->first_child, t1);
    }
    // insert t1 to arg_list_head[arg_list_head_index]
    t1->next_arg = arg_list_head[arg_list_head_index];
    arg_list_head[arg_list_head_index] = t1;
    assert(arg_list_head[arg_list_head_index]);    
    if (node->first_child->sibling == NULL) {
        return code1;
    }
    else {
        InterCode *code2 = ir_args(node->first_child->sibling->sibling);
        return ir_link(code1, code2);
    }
}

InterCode* ir_cond(ASTNode *node, Operand *label_true, Operand *label_false) {
    if (node == NULL)
        return NULL;

    if (strcmp(node->first_child->name, "Exp") == 0) {
        if (strcmp(node->first_child->sibling->name, "RELOP") == 0) { // Exp RELOP Exp
            Operand *t1 = NULL;
            Operand *t2 = NULL;
            InterCode *code1 = NULL;
            InterCode *code2 = NULL;
            // Exp1, get t1 and code1
            if (all_constant(node->first_child)) { // Exp1 is constant
                t1 = make_constant(get_constant(node->first_child));
            }
            else if (is_id(node->first_child)) { // Exp1 is ID
                t1 = get_id(node->first_child);
            }
            else { // Exp
                t1 = make_temp();
                code1 = ir_exp(node->first_child, t1);
            }
            // Exp2, get t2 and code2
            if (all_constant(node->first_child->sibling->sibling)) {
                t2 = make_constant(get_constant(node->first_child->sibling->sibling));
            }
            else if (is_id(node->first_child->sibling->sibling)) {
                t2 = get_id(node->first_child->sibling->sibling);
            }
            else {
                t2 = make_temp();
                code2 = ir_exp(node->first_child->sibling->sibling, t2);
            }
    
            InterCode *code3 = NULL; 
            // make Operand relop
            int relop_kind = -1;
            if (strcmp(node->first_child->sibling->value, ">") == 0) relop_kind = RELOP_G;
            else if (strcmp(node->first_child->sibling->value, "<") == 0) relop_kind = RELOP_L;
            else if (strcmp(node->first_child->sibling->value, ">=") == 0) relop_kind = RELOP_GE;
            else if (strcmp(node->first_child->sibling->value, "<=") == 0) relop_kind = RELOP_LE;
            else if (strcmp(node->first_child->sibling->value, "==") == 0) relop_kind = RELOP_E;
            else if (strcmp(node->first_child->sibling->value, "!=") == 0) relop_kind = RELOP_NE;
            Operand *relop = make_relop(relop_kind);
            // make code3
            if (label_true->u.label.no != -1 && label_false->u.label.no != -1) {
                code3 = ir_link(make_ir(IR_IF, label_true, t1, t2, relop), make_ir(IR_GOTO, label_false, NULL, NULL, NULL));
            }
            else if (label_true->u.label.no != -1) {
                code3 = make_ir(IR_IF, label_true, t1, t2, relop);
            }
            else if (label_false->u.label.no != -1) {
                code3 = make_ir(IR_IF, label_false, t1, t2, relop_reverse(relop));
            }
            return ir_link(ir_link(code1, code2), code3);
        }
        // Exp AND Exp
        else if (strcmp(node->first_child->sibling->name, "AND") == 0) {
            // Exp1
            InterCode *code1 = NULL;
            Operand *label1 = NULL;
            if (label_false->u.label.no != -1) { // if Exp1 false, goto label_false
                code1 = ir_cond(node->first_child, fall_label, label_false);
            }
            else { // if Exp1 true, goto label1
                label1 = make_label();
                code1 = ir_cond(node->first_child, fall_label, label1);
            }
            // Exp2
            InterCode *code2 = ir_cond(node->first_child->sibling->sibling, label_true, label_false);
            if (label_false->u.label.no != -1) {
                return ir_link(code1, code2);
            }
            else {
                return ir_link(ir_link(code1, code2), make_ir(IR_LABEL, label1, NULL, NULL, NULL));
            }
        }
        // Exp OR Exp
        else if (strcmp(node->first_child->sibling->name, "OR") == 0) {
            // Exp1
            InterCode *code1 = NULL;
            Operand *label1 = NULL;
            if (label_true->u.label.no != -1) {
                // if Exp1 true, goto label_true
                code1 = ir_cond(node->first_child, label_true, fall_label);
            }
            else { // if Exp1 false, goto label1
                label1 = make_label();
                code1 = ir_cond(node->first_child, label1, fall_label);
            }
            // Exp2
            // if Exp2 true, goto label_true, else goto label_false
            InterCode *code2 = ir_cond(node->first_child->sibling->sibling, label_true, label_false);
            if (label_true->u.label.no != -1) {
                return ir_link(code1, code2);
            }
            else {
                return ir_link(ir_link(code1, code2), make_ir(IR_LABEL, label1, NULL, NULL, NULL));
            }
        }
    }
    // NOT Exp
    if (strcmp(node->first_child->name, "NOT") == 0) {
        return ir_cond(node->first_child->sibling, label_false, label_true);
    }
    else { // Constant, ID, other Exp
        Operand *t1 = NULL;
        InterCode *code1 = NULL;
        if (all_constant(node)) { // Constant
            t1 = make_constant(get_constant(node));
        }
        else if (is_id(node)) { // ID
            t1 = get_id(node);
        }
        else { // other Exp (but its first child is not Exp or NOT)
            t1 = make_temp();
            code1 = ir_exp(node, t1);
        }
        
        InterCode *code3 = NULL;
            if (label_true->u.label.no != -1 && label_false->u.label.no != -1) {
                code3 = ir_link(make_ir(IR_IF, label_true, t1, make_constant(0), make_relop(RELOP_NE)), 
                                make_ir(IR_GOTO, label_false, NULL, NULL, NULL));
            }
            else if (label_true->u.label.no != -1) {
                code3 = make_ir(IR_IF, label_true, t1, make_constant(0), make_relop(RELOP_NE));
            }
            else if (label_false->u.label.no != -1) {
                code3 = make_ir(IR_IF, label_false, t1, make_constant(0), make_relop(RELOP_E));
            }
            return ir_link(code1, code3);
    }
}

/* help function */
Func* ir_insert_func_hash_table(int hash_index, char* func_name, Type *ret_type, Func *func){
    Func *func_ptr = func_hash[hash_index];
    if (func_ptr == NULL){
        func->ret_type = ret_type;
        func->defined = 1;
        Operand *op = make_func(func);
        func->op = op;
        func_hash[hash_index] = func;
        return func;
    }
    func->ret_type = ret_type;
    func->defined = 1;
    Operand *op = make_func(func);
    func->op = op;
    func->next = func_hash[hash_index];
    func_hash[hash_index] = func;
    return func_hash[hash_index];
}

Func* ir_find_func_hash_table(int hash_index, char* func_name){
    Func *cur = func_hash[hash_index];
    if (cur == NULL)
        return NULL;
    while (cur){
        if (strcmp(func_name, cur->name) == 0)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

Field_List* ir_insert_field_hash_table(int hash_index, char* field_name, Type* type, ASTNode* node, int wrapped_layer, int is_struct){
    Field_List* new_field = malloc(sizeof(Field_List));
    strcpy(new_field->name, field_name);
    new_field->type = type;
    new_field->wrapped_layer = wrapped_layer;
    new_field->is_struct = is_struct;
    new_field->line_num = node->line_num;
    Operand *op = make_var(new_field);
    if (type->kind != BASIC)
        op = make_addr(op, 1);
    new_field->op = op;
    new_field->hash_list_index_next = var_hash[hash_index];
    var_hash[hash_index] = new_field;
    return new_field;            
}

Field_List* ir_find_field_hash_table(int hash_index, char* field_name, ASTNode* node, int look_for_structure){
    Field_List* field_ptr = var_hash[hash_index];
    while(field_ptr != NULL){
        if (strcmp(field_ptr->name, field_name) == 0 && field_ptr->is_struct == look_for_structure)
            return field_ptr;
        field_ptr = field_ptr->hash_list_index_next;
    }
    return NULL;
}

Operand* make_var(Field_List* field){
    Operand *op = malloc(sizeof(Operand));
    op->kind = OP_VARIABLE;
    op->u.var.no = var_no++;
    op->u.var.field = field;
    op->u.var.parent_func = NULL;
    op->u.var.offset = -1;
    insert_operand(op);
    return op;
}

Operand* make_addr(Operand* var, int ref_hidden){
    Operand *addr = malloc(sizeof(Operand));
    addr->kind = OP_ADDRESS;
    addr->u.addr.no = var->u.var.no;
    addr->u.addr.val_kind = var->kind;  
    addr->u.addr.field = var->u.var.field;
    addr->u.addr.ref_hidden = ref_hidden;
    addr->u.addr.parent_func = NULL;
    addr->u.addr.offset = -1;
    addr->u.addr.size = -1;
    insert_operand(addr);
    return addr;
}

Operand* make_func(Func* func){
    Operand *op = malloc(sizeof(Operand));
    op->kind = OP_FUNC;
    op->u.func.func = func;
    insert_operand(op);
    if (strcmp(func->name, "read") == 0)
        read_op = op;
    else if (strcmp(func->name, "write") == 0)
        write_op = op;
    return op;
}

Operand* make_constant(int val) {
    Operand *new_constant = malloc(sizeof(Operand));
    new_constant->kind = OP_CONSTANT;
    new_constant->u.constant.val = val;
    insert_operand(new_constant);
    return new_constant;
}

Operand* make_temp() {
    Operand* new_temp = malloc(sizeof(Operand));
    new_temp->kind = OP_TEMP;
    new_temp->u.temp.no = temp_no ++;
    new_temp->u.temp.parent_func = NULL;
    new_temp->u.temp.offset = -1;
    insert_operand(new_temp);
    return new_temp;
}

Operand* make_relop(int relop_kind) {
    Operand *new_relop = malloc(sizeof(Operand));
    new_relop->u.relop.kind = relop_kind;
    new_relop->kind = OP_RELOP;
    insert_operand(new_relop);
    return new_relop;
}

Operand* make_label() {
    Operand *new_label = malloc(sizeof(Operand));
    new_label->kind = OP_LABEL;
    new_label->u.label.no = label_no ++;
    insert_operand(new_label);
    return new_label;
}

Operand* make_fall_label(){
    Operand* new_label = malloc(sizeof(Operand));
    new_label->kind = OP_LABEL;
    new_label->u.label.no = -1;
    return new_label;
}

void insert_operand(Operand* op){
    op->next_list_op = op_head;
    op_head = op;
}

InterCode* ir_link(InterCode* code1, InterCode* code2){
    if (code1 == NULL && code2 == NULL) {
        return NULL;
    }
    if (code1 == NULL && code2 != NULL) {
        if (ir_head == NULL)
            ir_head = code2;
        return code2;
    }
    if (code1 != NULL && code2 == NULL) {
        if (ir_head == NULL)
            ir_head = code1;
        return code1;
    }
    
    if (ir_head == NULL)
        ir_head = code1;
    InterCode* code1_tail = code1;
    while (code1_tail->next != NULL) {
        code1_tail = code1_tail->next;
    }
    code1_tail->next = code2;
    code2->prev = code1_tail;
    return code1;
}

InterCode* make_ir(int kind, Operand* res, Operand* op1, Operand* op2, Operand* relop){
    InterCode* code = malloc(sizeof(InterCode));
    code->kind = kind;
    switch(kind) {
        // no operand
        case IR_LABEL:
        case IR_FUNCTION:
        case IR_GOTO:
        case IR_RETURN:
        case IR_ARG:
        case IR_PARAM:
        case IR_READ:
        case IR_WRITE:
            code->u.no_op.result = res;
            break;
        // single operand
        case IR_ASSIGN:
        case IR_REF_ASSIGN:
        case IR_DEREF_ASSIGN:
        case IR_ASSIGN_TO_DEREF:
        case IR_DEC:
        case IR_CALL:
            code->u.sin_op.result = res;
            code->u.sin_op.op = op1;
            break;
        // double operand
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
            code->u.bin_op.result = res;
            code->u.bin_op.op1 = op1;
            code->u.bin_op.op2 = op2;
            break;
        // triple operand
        case IR_IF:
            code->u.tri_op.op1 = op1;
            code->u.tri_op.op2 = op2;
            code->u.tri_op.relop = relop;
            code->u.tri_op.result = res;
            break;
        default:
            break;
    }
    if (ir_head == NULL)
        ir_head = code;
    return code;
}

// get size_offset of structure
int build_size_offset(Type* structure_type) {
    Field_List* field = structure_type->u.structure.first_field;
    int offset = 0;
    while (field != NULL) {
        field->offset = offset;
        if (field->type->kind == BASIC) {
            offset += 4;
        }
        else if (field->type->kind == ARRAY) {
            offset += size_of_array_type(field->type);
        }
        else { //STRUCTURE
            field->type->u.structure.size = build_size_offset(field->type);
            offset += field->type->u.structure.size;
        }
        field = field->next_struct_field;
    }
    return offset;
}

int size_of_array_type(Type* type) {
    int size = 1;
    Type* cur_type = type;
    while (cur_type && cur_type->kind == ARRAY) {
        size *= cur_type->u.array.size;
        cur_type = cur_type->u.array.elem;
    }
    assert(cur_type);
    if (cur_type->kind == BASIC) {
        size *= 4;
    }
    else {
        assert(cur_type->kind == STRUCTURE);
        size *= cur_type->u.structure.size;
    }
    return size;
}

// int size_of_array(ASTNode* node) {
//     int size = 1;
//     Type *type = ir_exp_type(node);
//     Type *cur_type = type;
//     while (cur_type && cur_type->kind == ARRAY) {
//         size *= cur_type->u.array.size;
//         cur_type = cur_type->u.array.elem;
//     }
//     assert(cur_type);
//     if (cur_type->kind == BASIC) {
//         size *= 4;
//     }
//     else {
//         assert(cur_type->kind == STRUCTURE);
//         size *= cur_type->u.structure.size;
//     }
//     return size;
// }

// 0: not all constant, 1: all constant
int all_constant(ASTNode* node) {
    // Exp PLUS Exp || Exp MINUS Exp || Exp STAR Exp || Exp DIV Exp
    if (strcmp(node->first_child->name, "Exp") == 0 && node->first_child->sibling && 
        (strcmp(node->first_child->sibling->name, "PLUS") == 0 || strcmp(node->first_child->sibling->name, "MINUS") == 0
        || strcmp(node->first_child->sibling->name, "STAR") == 0 || strcmp(node->first_child->sibling->name, "DIV") == 0)) {
        if (!all_constant(node->first_child) || !all_constant(node->first_child->sibling->sibling)) {
            return 0;
        }
        return 1;
    }
    // MINUS Exp || LP Exp RP
    if (strcmp(node->first_child->name, "MINUS") == 0 || strcmp(node->first_child->name, "LP") == 0) {
        if (!all_constant(node->first_child->sibling)) {
            return 0;
        }
        return 1;
    }
    // INT 
    if (strcmp(node->first_child->name, "INT") == 0) {
        return 1;
    }
    return 0;
}

int get_constant(ASTNode* node) {
    // Exp PLUS Exp || Exp MINUS Exp || Exp STAR Exp || Exp DIV Exp
    if (strcmp(node->first_child->name, "Exp") == 0 && node->first_child->sibling) {
        if (strcmp(node->first_child->sibling->name, "PLUS") == 0) {
            return get_constant(node->first_child) + get_constant(node->first_child->sibling->sibling);
        }
        if (strcmp(node->first_child->sibling->name, "MINUS") == 0) {
            return get_constant(node->first_child) - get_constant(node->first_child->sibling->sibling);
        }
        if (strcmp(node->first_child->sibling->name, "STAR") == 0) {
            return get_constant(node->first_child) * get_constant(node->first_child->sibling->sibling);
        }
        if (strcmp(node->first_child->sibling->name, "DIV") == 0) {
            int divider = get_constant(node->first_child->sibling->sibling);
            if (divider == 0) { // check: divide-by-zero
                printf("Error: Divide by zero.\n");
                exit(1);
            }
            return get_constant(node->first_child) / get_constant(node->first_child->sibling->sibling);
        }
        return 0;
    }
    // MINUS Exp
    if (strcmp(node->first_child->name, "MINUS") == 0) {
        return -get_constant(node->first_child->sibling);
    }
    // LP Exp RP
    if (strcmp(node->first_child->name, "LP") == 0) {
        return get_constant(node->first_child->sibling);
    }
    // INT
    if (strcmp(node->first_child->name, "INT") == 0) {
        return atoi(node->first_child->value);
    }
    return 0;
}

// 0: not ID, 1: is ID
int is_id(ASTNode* node) {
    if (node->first_child && strcmp(node->first_child->name, "ID") == 0 && node->first_child->sibling == NULL) 
        return 1;
    return 0;
}

Operand* get_id(ASTNode* node) {
    Field_List *variable = ir_find_field_hash_table(hash_pjw(node->first_child->value), node->first_child->value, node->first_child, 0);
    return variable->op;
}

Operand* relop_reverse(Operand* relop) {
    Operand *new_relop = malloc(sizeof(Operand));
    new_relop->kind = OP_RELOP;
    new_relop->next_list_op = relop->next_list_op;
    new_relop->next_arg = relop->next_arg;
    switch (relop->u.relop.kind){
        case RELOP_G: new_relop->u.relop.kind = RELOP_LE; break;
        case RELOP_L: new_relop->u.relop.kind = RELOP_GE; break;
        case RELOP_GE: new_relop->u.relop.kind = RELOP_L; break;
        case RELOP_LE: new_relop->u.relop.kind = RELOP_G; break;
        case RELOP_E: new_relop->u.relop.kind = RELOP_NE; break;
        case RELOP_NE: new_relop->u.relop.kind = RELOP_E; break;
        default: break;
    }
    return new_relop;
}

Type* ir_exp_type(ASTNode *node){
    if (node == NULL) return NULL;
    if (strcmp(node->first_child->name, "Exp") == 0){
        // Exp ASSIGNOP Exp
        if (strcmp(node->first_child->sibling->name, "ASSIGNOP") == 0){
            Type *type = ir_exp_type(node->first_child);
            return type;
        }
        // Exp AND Exp || Exp OR Exp
        else if (strcmp(node->first_child->sibling->name, "AND") == 0
                || strcmp(node->first_child->sibling->name, "OR") == 0){
            Type *type = ir_exp_type(node->first_child);
            return type;
        }
        // Exp PLUS Exp || Exp MINUS Exp || Exp STAR Exp || Exp DIV Exp || Exp RELOP Exp
        else if (strcmp(node->first_child->sibling->name, "PLUS") == 0
            || strcmp(node->first_child->sibling->name, "MINUS") == 0
            || strcmp(node->first_child->sibling->name, "STAR") == 0
            || strcmp(node->first_child->sibling->name, "DIV") == 0
            || strcmp(node->first_child->sibling->name, "RELOP") == 0){
            Type *type = ir_exp_type(node->first_child);
            return type;
        }
        // Exp LB Exp RB
        else if (strcmp(node->first_child->sibling->name, "LB") == 0 && strcmp(node->first_child->sibling->sibling->name, "Exp") == 0){
            Type *type = ir_exp_type(node->first_child);
            assert(type);
            return type->u.array.elem;
        }
        // Exp DOT ID
        else if (strcmp(node->first_child->sibling->name, "DOT") == 0){
            Type *type = ir_exp_type(node->first_child);
            assert(type && type->kind == STRUCTURE);
            Field_List *cur = type->u.structure.first_field;
            while (cur){
                if (strcmp(cur->name, node->first_child->sibling->sibling->value) == 0){
                    return cur->type;
                }
                cur = cur->next_struct_field;
            }
            assert(0);
            return NULL;
        }
    }
    // LP Exp RP
    else if (strcmp(node->first_child->name, "LP") == 0){
        return ir_exp_type(node->first_child->sibling);
    }
    // MINUS Exp
    else if (strcmp(node->first_child->name, "MINUS") == 0){
        return ir_exp_type(node->first_child->sibling);
    }
    // NOT Exp
    else if (strcmp(node->first_child->name, "NOT") == 0){
        // Type *type = ir_exp_type(node->first_child->sibling);
        Type *new_type = (Type *)malloc(sizeof(Type));
        new_type->kind = BASIC;
        new_type->u.basic = BASIC_INT;
        return new_type;
    }
    // ID || ID LP RP || ID LP Args RP
    else if (strcmp(node->first_child->name, "ID") == 0){
        if (!node->first_child->sibling){
            int hash_index = hash_pjw(node->first_child->value);
            Field_List *field = ir_find_field_hash_table(hash_index, node->first_child->value, node->first_child, 0);
            return field->type;
        }
        int hash_index = hash_pjw(node->first_child->value);
        Func *func = ir_find_func_hash_table(hash_index, node->first_child->value);
        return func->ret_type;
    }
    // INT
    else if (strcmp(node->first_child->name, "INT") == 0){
        Type *type = (Type *)malloc(sizeof(Type));
        type->kind = BASIC;
        type->u.basic = BASIC_INT;
        return type;
    }
    // FLOAT
    else if (strcmp(node->first_child->name, "FLOAT") == 0){
        Type *type = (Type *)malloc(sizeof(Type));
        type->kind = BASIC;
        type->u.basic = BASIC_FLOAT;
        return type;
    }
    return NULL;
}


/* gen_ir */
void ir_to_file(FILE *fp) {
    InterCode *cur = ir_head;
    while (cur) {
        fprintf(fp, "%s", show_ir(cur));
        cur = cur->next;
    }
}
char* show_ir(InterCode* code) {
    assert(code);
    char *buffer = malloc(50 * sizeof(char));
    switch (code->kind) {
    case IR_LABEL: sprintf(buffer, "LABEL %s :\n", show_op(code->u.no_op.result)); break;
    case IR_FUNCTION: sprintf(buffer, "FUNCTION %s :\n", show_op(code->u.no_op.result)); break;
    case IR_ASSIGN: sprintf(buffer, "%s := %s\n", show_op(code->u.sin_op.result), show_op(code->u.sin_op.op)); break;
    case IR_ADD: sprintf(buffer, "%s := %s + %s\n", show_op(code->u.bin_op.result), show_op(code->u.bin_op.op1), show_op(code->u.bin_op.op2)); break;
    case IR_SUB: sprintf(buffer, "%s := %s - %s\n", show_op(code->u.bin_op.result), show_op(code->u.bin_op.op1), show_op(code->u.bin_op.op2)); break;
    case IR_MUL: sprintf(buffer, "%s := %s * %s\n", show_op(code->u.bin_op.result), show_op(code->u.bin_op.op1), show_op(code->u.bin_op.op2)); break;
    case IR_DIV: sprintf(buffer, "%s := %s / %s\n", show_op(code->u.bin_op.result), show_op(code->u.bin_op.op1), show_op(code->u.bin_op.op2)); break;
    case IR_REF_ASSIGN: sprintf(buffer, "%s := &%s\n", show_op(code->u.sin_op.result), show_op(code->u.sin_op.op)); break;
    case IR_DEREF_ASSIGN: sprintf(buffer, "%s := *%s\n", show_op(code->u.sin_op.result), show_op(code->u.sin_op.op)); break;
    case IR_ASSIGN_TO_DEREF: sprintf(buffer, "*%s := %s\n", show_op(code->u.sin_op.result), show_op(code->u.sin_op.op)); break;
    case IR_GOTO: sprintf(buffer, "GOTO %s\n", show_op(code->u.no_op.result)); break;
    case IR_IF: sprintf(buffer, "IF %s %s %s GOTO %s\n", show_op(code->u.tri_op.op1), show_op(code->u.tri_op.relop), show_op(code->u.tri_op.op2), show_op(code->u.tri_op.result)); break;
    case IR_RETURN: sprintf(buffer, "RETURN %s\n", show_op(code->u.no_op.result)); break;
    case IR_DEC: sprintf(buffer, "DEC v%d %d\n", code->u.sin_op.result->u.addr.no, code->u.sin_op.op->u.constant.val); break;
    case IR_ARG: sprintf(buffer, "ARG %s\n", show_op(code->u.no_op.result)); break;
    case IR_CALL: sprintf(buffer, "%s := CALL %s\n", show_op(code->u.sin_op.result), show_op(code->u.sin_op.op)); break;
    case IR_PARAM: sprintf(buffer, "PARAM %s\n", show_op(code->u.no_op.result)); break;
    case IR_READ: sprintf(buffer, "READ %s\n", show_op(code->u.no_op.result)); break;
    case IR_WRITE: sprintf(buffer, "WRITE %s\n", show_op(code->u.no_op.result)); break;
    default:
        break;
    }
    return buffer;
}

char *show_op(Operand *op) {
    assert(op);
    char *buffer = malloc(50 * sizeof(char));
    switch (op->kind) {
    case OP_VARIABLE: sprintf(buffer, "v%d", op->u.var.no); break;
    case OP_CONSTANT: sprintf(buffer, "#%d", op->u.constant.val); break;
    case OP_TEMP: sprintf(buffer, "t%d", op->u.temp.no); break;
    case OP_LABEL: sprintf(buffer, "label%d", op->u.label.no); break;
    case OP_FUNC: sprintf(buffer, "%s", op->u.func.func->name); break;
    case OP_RELOP: 
        switch (op->u.relop.kind) {
        case RELOP_G: sprintf(buffer, ">"); break;
        case RELOP_L: sprintf(buffer, "<"); break;
        case RELOP_GE: sprintf(buffer, ">="); break;
        case RELOP_LE: sprintf(buffer, "<="); break;
        case RELOP_E: sprintf(buffer, "=="); break;
        case RELOP_NE: sprintf(buffer, "!="); break;
        default: assert(0); break;
        }
        break;
    case OP_ADDRESS: 
        if (op->u.addr.ref_hidden == 0) {
            if (op->u.addr.val_kind == OP_TEMP) sprintf(buffer, "&t%d", op->u.addr.no);
            else sprintf(buffer, "&v%d", op->u.addr.no);
        }
        else {
            if (op->u.addr.val_kind == OP_TEMP) sprintf(buffer, "t%d", op->u.addr.no);
            else sprintf(buffer, "v%d", op->u.addr.no);
        }
        break;
    default: assert(0); break;
    }
    return buffer;
}