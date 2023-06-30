#include "ir.h"

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
    func->param_size = 0;
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
    func->param_size = 1;
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
        struct_type->u.structure.field_list = ir_def_list(node->first_child->sibling->sibling->sibling, wrapped_layer);
        struct_type->u.structure.size = build_size_offset(struct_type);
        struct_type->line_num = node->first_child->sibling->sibling->sibling->line_num;
        if (node->first_child->sibling->first_child){
            int hash_index = hash_pjw(node->first_child->sibling->first_child->value);
            ir_insert_field_hash_table(hash_index, node->first_child->sibling->first_child->value, structure_type, node->first_child->sibling->first_child, wrapped_layer, 1); // params: is_struct
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
            new_field->is_structure = 0;
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
        if (params == NULL)-
            return NULL;
        
        func->first_param = params;
        Field_List* func_ptr = func->first_param;
        func->param_size = 0;
        while (func_ptr != NULL){
            func->param_size ++;
            func_ptr = func_ptr->next_param;
        }

        ir_insert_func_hash_table(hash_pjw(func->name), func->name, ret_type, func);
        InterCode* code1 = make_ir(IR_FUNCTION, func->op, NULL, NULL, NULL);
        func_ptr = func->first_param;
        while (cur != NULL) {
            code1 = ir_link(code1, make_ir(IR_PARAM, func_ptr->op, NULL, NULL, NULL));
            func_ptr = func_ptr->next_param;
        }
        return code1;
    }
    else { // ID LP RP
        func->param_size = 0;
        func->first_param = NULL;
        ir_insert_func_hash_table(hash_pjw(func->name), func->name, ret_type, func);
        return make_ir(IR_FUNC, func->op, NULL, NULL, NULL);
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

InterCode *ir_stmt(AST_Node* node, int wrapped_layer) {
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
        if (node->first_child->sibling->sibling->sibling->sibling->sibling) { // IF LP Exp RP Stmt ELSE Stmt
            Operand* label1 = make_label(); 
            Operand* label2 = make_label();
            InterCode* code1 = ir_cond(node->first_child->sibling->sibling, fall_label, label1); 
            InterCode* code2 = ir_stmt(node->first_child->sibling->sibling->sibling->sibling, wrapped_layer);
            InterCode* code3 = ir_stmt(node->first_child->sibling->sibling->sibling->sibling->sibling->sibling, wrapped_layer);
            return
                bind(
                    bind(
                        bind(
                            bind(
                                bind(
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
        else { // IF LP Exp RP Stmt
            Operand *label1 = make_label();
            InterCode *code1 = ir_cond(node->first_child->sibling->sibling, fall_label, label1);
            InterCode *code2 = ir_stmt(node->first_child->sibling->sibling->sibling->sibling, wrapped_layer);
            return 
                bind(
                    bind(
                        code1,
                        code2
                    ), 
                    make_ir(IR_LABEL, label1, NULL, NULL, NULL)
                );
        }
    }
    if (strcmp(node->first_child->name, "WHILE") == 0) {
        Operand *label1 = make_label();
        Operand *label3 = make_label();
        InterCode *code1 = ir_cond(node->first_child->sibling->sibling, fall_label, label3);
        InterCode *code2 = ir_stmt(node->first_child->sibling->sibling->sibling->sibling, wrapped_layer);
        return
            bind(
                bind(
                    bind(
                        bind(
                            make_ir(IR_LABEL, label1, NULL, NULL, NULL),
                            code1
                        ),
                        code2
                    ),
                    make_ir(IR_GOTO, label1, NULL, NULL, NULL)
                ),
                make_ir(IR_LABEL, label3, NULL, NULL, NULL)
            );
    }
    assert(0);
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

// todo
InterCode* ir_dec(ASTNode* node, Type* type, int wrapped_layer) {
    if (node == NULL)
        return NULL;
    if (type == NULL)
        return NULL;
    
    if (node->first_child->sibling){ // VarDec
        
    }
    else{ // VarDec ASSIGNOP Exp
       
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
        // todo 7/1
        else if (strcmp(node->first_child->sibling->name, "AND") == 0) {
            InterCode *code1 = NULL;
            Operand *label1 = NULL;
            if (label_false->u.label.no != -1) {
                code1 = ir_cond(node->first_child, fall_label, label_false);
            }
            else {
                label1 = make_label();
                code1 = ir_cond(node->first_child, fall_label, label1);
            }
            InterCode *code2 = ir_cond(node->first_child->sibling->sibling, label_true, label_false);
            if (label_false->u.label.no != -1) {
                return bind(code1, code2);
            }
            else {
                return bind(bind(code1, code2), make_ir(IR_LABEL, label1, NULL, NULL, NULL));
            }
        }
        else if (strcmp(node->first_child->sibling->name, "OR") == 0) {
            InterCode *code1 = NULL;
            Operand *label1 = NULL;
            if (label_true->u.label.no != -1) {
                code1 = ir_cond(node->first_child, label_true, fall_label);
            }
            else {
                label1 = make_label();
                code1 = ir_cond(node->first_child, label1, fall_label);
            }
            InterCode *code2 = ir_cond(node->first_child->sibling->sibling, label_true, label_false);
            if (label_true->u.label.no != -1) {
                return bind(code1, code2);
            }
            else {
                return bind(bind(code1, code2), make_ir(IR_LABEL, label1, NULL, NULL, NULL));
            }
        }
    }
    if (strcmp(node->first_child->name, "NOT") == 0) {
        return ir_cond(node->first_child->sibling, label_false, label_true);
    }
    else {
        Operand *t1 = NULL;
        InterCode *code1 = NULL;
        if (all_constant(node)) {
            t1 = make_constant(get_constant(node));
        }
        else if (is_id(node)) {
            t1 = get_id(node);
        }
        else {
            t1 = make_temp();
            code1 = ir_exp(node, t1);
        }
        InterCode *code3 = NULL;
            if (label_true->u.label.no != -1 && label_false->u.label.no != -1) {
                code3 = bind(make_ir(IR_IF, label_true, t1, make_constant(0), 
                    make_relop(RELOP_NE)), make_ir(IR_GOTO, label_false, NULL, NULL, NULL));
            }
            else if (label_true->u.label.no != -1) {
                code3 = make_ir(IR_IF, label_true, t1, make_constant(0), make_relop(RELOP_NE));
            }
            else if (label_false->u.label.no != -1) {
                code3 = make_ir(IR_IF, label_false, t1, make_constant(0), make_relop(RELOP_E));
            }
            return bind(code1, code3);
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
        if (strcmp(field_now->name, field_name) == 0 && field_ptr->is_struct == look_for_structure)
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
    new_var->u.var.parent_func = NULL;
    new_var->u.var.offset = -1;
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
    op->kind = FUNCTION;
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

Operand *make_fall_label(){
    Operand* new_label = malloc(sizeof(Operand));
    new_label->kind = LABEL;
    new_label->u.label_no = -1;
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
            code->u.nonop.result = res;
            break;
        // single operand
        case IR_ASSIGN:
        case IR_REF_ASSIGN:
        case IR_DEREF_ASSIGN:
        case IR_ASSIGN_TO_DEREF:
        case IR_DEC:
        case IR_CALL:
            code->u.sinop.result = res;
            code->u.sinop.op = op1;
            break;
        // double operand
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
            code->u.binop.result = res;
            code->u.binop.op1 = op1;
            code->u.binop.op2 = op2;
            break;
        // triple operand
        case IR_IF:
            code->u.trinop.op1 = op1;
            code->u.trinop.op2 = op2;
            code->u.trinop.relop = relop;
            code->u.trinop.result = res;
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