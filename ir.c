#include "compiler.h"

/**********************中间代码**************************/
// 临时变量以及标签
// 初始化临时变量和标签的数组为-1
void init_tempvar_lable(){
    // int i;
    // for (i = 0; i < 100; i++)
    // {
    //     // -1表示该临时变量\标签还未被使用
    //     tempvar[i] = -1;
    //     lables[i] = -1;
    // }
    memset(tempvar, -1, sizeof(tempvar));
    memset(lables, -1, sizeof(lables));
}

// 创建一个新的临时变量
Operand new_tempvar(){
    int i;
    for (i = 0; i < 100; i ++){
        if (tempvar[i] == -1){
            tempvar[i] = 1;
            break;
        }
    }
    if (i >= 100) // 分配的临时变量达到上限
        return NULL;

    Operand result = new_Operand();
    result->kind = TEMPVAR;
    result->operand.tempvar = i + 1;
    temp_Operands[i] = result;

    return result;
}

// 创建一个新的lable
Operand new_lable(){
    int i;
    for (int i = 0; i < 100; i++){
        if (lables[i] == -1){
            lables[i] = 1;
            break;
        }
    }
    if (i >= 100)
        return NULL;

    Operand result = new_Operand();
    result->kind = LABLE;
    result->operand.lable = i + 1;

    return result;
}

// 创建一个新的操作数
Operand new_Operand(){
    Operand result = (Operand)malloc(sizeof(OperandStru));
    result->value = -10000;
    return result;
}

// 创建一个新的变量
Operand new_Variable(char *name){
    Operand result = new_Operand();
    result->kind = VARIABLE;
    result->operand.name = name;

    return result;
}

// 创建一个新的整型常量
Operand new_Const(float value){
    Operand result = new_Operand();
    result->kind = CONSTANT;
    result->operand.value = (int)value;
    result->value = (int)value;

    return result;
}

// 创建一条新的中间代码
InterCode new_Code(){
    InterCode result = (InterCode)malloc(sizeof(InterCodeStru));
    result->kind = _NULL;
    result->prev = NULL;
    result->next = NULL;

    return result;
}

// 创建一条lable声明的中间代码
InterCode new_lable_Code(Operand lable){
    InterCode result = new_Code();
    result->kind = _LABLE;
    result->operands.var = lable;

    return result;
}

// 创建一条跳转语句的中间代码
InterCode new_goto_Code(Operand lable){
    InterCode result = new_Code();
    result->kind = _GOTO;
    result->operands.jump.lable = lable;

    return result;
}

// 创建一条赋值的中间代码
InterCode new_assign_Code(Operand left, Operand right){
    left->value = right->value;
    InterCode result = new_Code();
    result->kind = _ASSIGN;
    result->operands.assign.left = left;
    result->operands.assign.right = right;

    return result;
}

// 打印一条中间代码
void print_Code(InterCode code){
    if (code == NULL){
        printf("Error, InterCode is NULL\n");
        return;
    }
    switch (code->kind){
    case _NULL: // 代码为空
        break;
    case _LABLE: // 定义标号
        printf("LABLE ");
        print_Operand(code->operands.var);
        break;
    case _FUNCTION: // 定义函数
        printf("FUNCTION ");
        print_Operand(code->operands.var);
        break;
    case _ASSIGN: // =
        print_Operand(code->operands.assign.left);
        printf(" := ");
        print_Operand(code->operands.assign.right);
        break;
    case _ADD: // +
        print_Operand(code->operands.binop.result);
        printf(" := ");
        print_Operand(code->operands.binop.op1);
        printf(" + ");
        print_Operand(code->operands.binop.op2);
        break;
    case _SUB: // -
        print_Operand(code->operands.binop.result);
        printf(" := ");
        print_Operand(code->operands.binop.op1);
        printf(" - ");
        print_Operand(code->operands.binop.op2);
        break;
    case _MUL: // *
        print_Operand(code->operands.binop.result);
        printf(" := ");
        print_Operand(code->operands.binop.op1);
        printf(" * ");
        print_Operand(code->operands.binop.op2);
        break;
    case _DIV: // /
        print_Operand(code->operands.binop.result);
        printf(" := ");
        print_Operand(code->operands.binop.op1);
        printf(" / ");
        print_Operand(code->operands.binop.op2);
        break;
    case _GOTO: // 无条件跳转
        printf("GOTO ");
        print_Operand(code->operands.jump.lable);
        break;
    case _IFGOTO: // 判断跳转
        printf("IF ");
        print_Operand(code->operands.jump.op1);
        printf(" %s ", code->operands.jump.relop);
        print_Operand(code->operands.jump.op2);
        printf(" GOTO ");
        print_Operand(code->operands.jump.lable);
        break;
    case _RETURN: // 函数返回
        printf("RETURN ");
        print_Operand(code->operands.var);
        break;
    case _ARG: // 传实参
        printf("ARG ");
        print_Operand(code->operands.var);
        break;
    case _CALL: // 函数调用
        if (code->operands.assign.left == NULL){
            printf("CALL ");
        } else {
            print_Operand(code->operands.assign.left);
            printf(" := CALL ");
        }
        print_Operand(code->operands.assign.right);
        break;
    case _PARAM: // 函数参数声明
        printf("PARAM ");
        print_Operand(code->operands.var);
        break;
    case _READ: // 从控制台读取x
        printf("READ ");
        print_Operand(code->operands.var);
        break;
    case _WRITE: // 向控制台打印x
        printf("WRITE ");
        print_Operand(code->operands.var);
        break;
    default:
        printf("Code Error");
        break;
    }
    if (code->kind != _NULL)
        printf("\n");
}

// 打印一个操作数
void print_Operand(Operand op){
    if (op == NULL){
        printf("Error, Operand is NULL\n");
        return;
    }
    switch (op->kind){
    case VARIABLE:
    case FUNC:
        printf("%s", op->operand.name);
        break;
    case TEMPVAR:
        printf("t%d", op->operand.tempvar);
        break;
    case LABLE:
        printf("lable%d", op->operand.lable);
        break;
    case CONSTANT:
        printf("#%d", op->operand.value);
        break;
    case ADDRESS:
        printf("&%s", op->operand.name);
        break;
    case VALUE:
        printf("#%s", op->operand.name);
        break;
    default:
        printf("Operand Error");
        break;
    }
}

// 打印一段中间代码
void print_Codes(InterCode codes){
    printf("\n中间代码打印：\n");
    InterCode temp = codes;
    while (temp){
        print_Code(temp);
        temp = temp->next;
    }
    printf("打印完毕\n");
}

// 获取链表的尾部
InterCode get_Tail(InterCode codes){
    InterCode temp = codes;
    while (temp->next){
        temp = temp->next;
    }
    return temp;
}

// 在链表末尾加上另一条链表
InterCode add_Codes(int num, ...){
    // 参数列表，详见 stdarg.h 用法
    va_list list;
    // 初始化参数列表
    va_start(list, num);
    // 拼接中间代码
    InterCode code = va_arg(list, InterCode);
    InterCode temp = new_Code();
    InterCode tail = new_Code();
    for (int i = 1; i < num; i++){
        tail = get_Tail(code);
        temp = va_arg(list, InterCode);
        tail->next = temp;
        temp->prev = tail;
    }
    return code;
}


/*
整体程序的翻译模式
Program:ExtDefList 
*/
InterCode translate_Program(tnode Program){
    // ExtDefList
    if (Program->ncld == 1){
        return translate_ExtDefList((Program->cld)[0]);
    }
    return new_Code();
}

/*
ExtDefList:零个或多个ExtDef
ExtDefList:
    ExtDef ExtDefList 
	| (empty)
*/
InterCode translate_ExtDefList(tnode ExtDefList){
    // ExtDef ExtDefList
    if (ExtDefList->ncld == 2){
        InterCode code1 = translate_ExtDef((ExtDefList->cld)[0]);
        InterCode code2 = translate_ExtDefList((ExtDefList->cld)[1]);
        return add_Codes(2, code1, code2);
    }
    return new_Code();
}

/*
ExtDef:全局变量/结构体/函数定义
ExtDef: 
`   Specifire ExtDecList SEMI 
	|Specifire SEMI	
	|Specifire FunDec Compst
*/
InterCode translate_ExtDef(tnode ExtDef){
    // Specifire ExtDecList SEMI
    if (ExtDef->ncld == 3 && !strcmp((ExtDef->cld)[1]->name, "ExtDecList")){
        // 全局变量声明不进行处理
    }
    // Specifire SEMI
    else if (ExtDef->ncld == 2 && !strcmp((ExtDef->cld)[1]->name, "SEMI")){
        // 函数外结构体定义不进行处理
    }
    // Specifire FunDec Compst
    else if (ExtDef->ncld == 3 && !strcmp((ExtDef->cld)[1]->name, "FunDec")){
        InterCode code1 = translate_FunDec((ExtDef->cld)[1]);
        InterCode code2 = translate_CompSt((ExtDef->cld)[2]);
        return add_Codes(2, code1, code2);
    }
    return new_Code();
}

/*
变量、函数声明的翻译模式
FunDec:
    ID LP VarList RP 
	|ID LP RP 
*/
InterCode translate_FunDec(tnode FunDec) {
    // ID LP VarList RP
    if (FunDec->ncld == 4){
        Operand function = new_Variable((FunDec->cld)[0]->content);
        InterCode code1 = new_Code();
        code1->kind = _FUNCTION;
        code1->operands.var = function;
        InterCode code2 = translate_VarList((FunDec->cld)[2]);
        return add_Codes(2, code1, code2);
    }
    // ID LP RP
    else if (FunDec->ncld == 3){
        Operand function = new_Variable((FunDec->cld)[0]->content);
        InterCode code1 = new_Code();
        code1->kind = _FUNCTION;
        code1->operands.var = function;
        return code1;
    }
    return new_Code();
}

/*
函数参数列表
VarList:
    ParamDec COMMA VarList 
	|ParamDec 
*/
InterCode translate_VarList(tnode VarList){
    // ParamDec
    if (VarList->ncld == 1){
        return translate_ParamDec((VarList->cld)[0]);
    }
    // ParamDec COMMA VarList
    else if (VarList->ncld == 3){
        InterCode code1 = translate_ParamDec((VarList->cld)[0]);
        InterCode code2 = translate_VarList((VarList->cld)[2]);
        return add_Codes(2, code1, code2);
    }
    return new_Code();
}

/*
函数的某一个传入参数
ParamDec:
    Specifire VarDec
*/
InterCode translate_ParamDec(tnode ParamDec){
    // Specifire VarDec
    if (ParamDec->ncld == 2){
        // VarDec:ID 或 VarDec LB INT RB 
        // 以下忽略数组的情况
        tnode ID = ((ParamDec->cld)[1]->cld)[0];
        InterCode code1 = new_Code();
        code1->kind = _PARAM;
        code1->operands.var = new_Variable(ID->content);
        return code1;
    }
    return new_Code();
}

/*
函数体翻译
Compst:
    LC DefList StmtList RC
*/
InterCode translate_CompSt(tnode ComSt){
    if (ComSt->ncld == 4) {
        InterCode code1 = translate_DefList((ComSt->cld)[1]);
        InterCode code2 = translate_StmtList((ComSt->cld)[2]);
        return add_Codes(2, code1, code2);
    }
    return new_Code();
}

/*
函数体内的语句列表
StmtList:
    Stmt StmtList
	| （empty）
*/
InterCode translate_StmtList(tnode StmtList){
    if (StmtList->ncld == 2){
        InterCode code1 = translate_Stmt((StmtList->cld)[0]);
        InterCode code2 = translate_StmtList((StmtList->cld)[1]);
        return add_Codes(2, code1, code2);
    }
    return new_Code();
}

/*
函数体内单条语句的翻译模式
Stmt:
    Exp SEMI
	|Compst  //语句块
	|RETURN Exp SEMI //返回语句
    |IF LP Exp RP Stmt %prec LOWER_THAN_ELSE  // if-then 语句
    |IF LP Exp RP Stmt ELSE Stmt %prec ELSE  // if-then-else语句
	|WHILE LP Exp RP Stmt  // while语句
*/
InterCode translate_Stmt(tnode Stmt){
    // Exp SEMI
    if (Stmt->ncld == 2 && !strcmp((Stmt->cld)[1]->name, "SEMI")){
        return translate_Exp((Stmt->cld)[0], NULL);
    }
    // Compst
    else if (Stmt->ncld == 1 && !strcmp((Stmt->cld)[0]->name, "Compst")){
        return translate_CompSt((Stmt->cld)[0]);
    }
    // RETURN Exp SEMI
    else if (Stmt->ncld == 3 && !strcmp((Stmt->cld)[0]->name, "RETURN")){
        // 中间代码优化
        Operand existOp = get_Operand((Stmt->cld)[1]);
        if (existOp == NULL){
            Operand t1 = new_tempvar();
            InterCode code1 = translate_Exp((Stmt->cld)[1], t1);
            InterCode code2 = new_Code();
            code2->kind = _RETURN;
            code2->operands.var = t1;
            return add_Codes(2, code1, code2);
        } else {
            InterCode code1 = new_Code();
            code1->kind = _RETURN;
            code1->operands.var = existOp;
            return code1;
        }
    }
    // IF LP Exp RP Stmt
    else if (Stmt->ncld == 5 && !strcmp((Stmt->cld)[0]->name, "IF")){
        Operand lable1 = new_lable();
        Operand lable2 = new_lable();
        InterCode code1 = translate_Cond((Stmt->cld)[2], lable1, lable2);
        InterCode code2 = translate_Stmt((Stmt->cld)[4]);
        return add_Codes(4, code1, new_lable_Code(lable1), code2, new_lable_Code(lable2));
    }
    // IF LP Exp RP Stmt ELSE Stmt
    else if (Stmt->ncld == 7 && !strcmp((Stmt->cld)[0]->name, "IF")){
        Operand lable1 = new_lable();
        Operand lable2 = new_lable();
        Operand lable3 = new_lable();
        InterCode code1 = translate_Cond((Stmt->cld)[2], lable1, lable2);
        InterCode code2 = translate_Stmt((Stmt->cld)[4]);
        InterCode code3 = translate_Stmt((Stmt->cld)[6]);
        return add_Codes(7, code1, new_lable_Code(lable1), code2, new_goto_Code(lable3), new_lable_Code(lable2), code3, new_lable_Code(lable3));
    }
    // WHILE LP Exp RP Stmt
    else if (Stmt->ncld == 5 && !strcmp((Stmt->cld)[0]->name, "WHILE")){
        Operand lable1 = new_lable();
        Operand lable2 = new_lable();
        Operand lable3 = new_lable();
        InterCode code1 = translate_Cond((Stmt->cld)[2], lable2, lable3);
        InterCode code2 = translate_Stmt((Stmt->cld)[4]);
        return add_Codes(6, new_lable_Code(lable1), code1, new_lable_Code(lable2), code2, new_goto_Code(lable1), new_lable_Code(lable3));
    }
    return new_Code();
}

/*
声明列表
DefList:
    Def DefList
	| (empty)
	;
*/
InterCode translate_DefList(tnode DefList){
    // Def DefList
    if (DefList->ncld == 2){
        InterCode code1 = translate_Def((DefList->cld)[0]);
        InterCode code2 = translate_DefList((DefList->cld)[1]);
        return add_Codes(2, code1, code2);
    }
    return new_Code();
}

/*
函数体内单条声明语句
Def:
    Specifire DecList SEMI
*/
InterCode translate_Def(tnode Def){
    // Specifire DecList SEMI
    if (Def->ncld == 3){
        return translate_DecList((Def->cld)[1]);
    }
    return new_Code();
}

/*
函数体内声明语句中除去类型，e.g. x1, x2, x2
DecList:
    Dec
	|Dec COMMA DecList 
*/
InterCode translate_DecList(tnode DecList){
    // Dec
    if (DecList->ncld == 1){
        return translate_Dec((DecList->cld)[0]);
    }
    // Dec COMMA DecList
    else if (DecList->ncld == 3){
        InterCode code1 = translate_Dec((DecList->cld)[0]);
        InterCode code2 = translate_DecList((DecList->cld)[2]);
        return add_Codes(2, code1, code2);
    }
    return new_Code();
}

/*Dec:
    VarDec 
	|VarDec ASSIGNOP Exp //允许 x1 = 10 的形式，即声明时赋值
*/
InterCode translate_Dec(tnode Dec){
    // VarDec ASSIGNOP Exp
    if (Dec->ncld == 3){
        // 没有处理数组
        // VarDec:ID
        tnode ID = ((Dec->cld)[0]->cld)[0];
        Operand variable = new_Variable(ID->content);
        Operand t1 = new_tempvar();
        InterCode code1 = translate_Exp((Dec->cld)[2], t1);
        InterCode code2 = new_assign_Code(variable, t1);
        return add_Codes(2, code1, code2);
    }
    // VarDec
    return new_Code();
}

// 当Exp的翻译模式为INT、ID、MINUS Exp时，可以获取已经声明过的操作数
Operand get_Operand(tnode Exp){
    // INT
    if (Exp->ncld == 1 && !strcmp((Exp->cld)[0]->name, "INT")){
        return find_Const((int)((Exp->cld)[0]->value));
    }
    // ID
    else if (Exp->ncld == 1 && !strcmp((Exp->cld)[0]->name, "ID")){
        Operand variable = new_Variable((Exp->cld)[0]->content);
        return variable;
    }
    // MINUS Exp  (Exp:INT)
    else if (Exp->ncld == 2 && !strcmp((Exp->cld)[0]->name, "MINUS")){
        if (!strcmp(((Exp->cld)[1]->cld)[0]->name, "INT")){
            int value = -(int)(((Exp->cld)[1]->cld)[0]->value);
            Operand result = find_Const(value);
            if (result == NULL)
                return new_Const(value);
            else
                return result;
        }
    }
    return NULL;
}

// 查看是否已经声明过同一个常数值的临时变量
Operand find_Const(int value){
    for (int i = 0; i < 100; i ++) {
        if (tempvar[i] == -1)
            break;
        if (temp_Operands[i]->kind == TEMPVAR && temp_Operands[i]->value == value)
            return temp_Operands[i];
    }
    return NULL;
}

/*
基本表达式的翻译模式

*/
InterCode translate_Exp(tnode Exp, Operand place)
{
    int isCond = 0;
    // INT
    if (Exp->ncld == 1 && !strcmp((Exp->cld)[0]->name, "INT"))
    {
        Operand value = new_Const((Exp->cld)[0]->value);
        InterCode code = new_assign_Code(place, value);
        return code;
    }
    // ID
    else if (Exp->ncld == 1 && !strcmp((Exp->cld)[0]->name, "ID"))
    {
        Operand variable = new_Variable((Exp->cld)[0]->content);
        InterCode code = new_assign_Code(place, variable);
        return code;
    }
    // Exp1 ASSIGNOP Exp2
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "ASSIGNOP"))
    {
        // Exp1 -> ID
        if ((Exp->cld)[0]->ncld == 1 && !strcmp(((Exp->cld)[0]->cld)[0]->name, "ID"))
        {
            Operand variable = new_Variable(((Exp->cld)[0]->cld)[0]->content);
            Operand existOp = get_Operand((Exp->cld)[2]);
            // 中间代码优化
            if (existOp == NULL)
            {
                Operand t1 = new_tempvar();
                InterCode code1 = translate_Exp((Exp->cld)[2], t1);
                InterCode code2 = new_assign_Code(variable, t1);
                if (place == NULL)
                    return add_Codes(2, code1, code2);
                else
                {
                    InterCode code3 = new_assign_Code(place, variable);
                    return add_Codes(3, code1, code2, code3);
                }
            }
            else
            {
                return new_assign_Code(variable, existOp);
            }
        }
    }
    // Exp PLUS Exp
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "PLUS"))
    {
        Operand op1 = get_Operand((Exp->cld)[0]);
        Operand op2 = get_Operand((Exp->cld)[2]);
        if (op1 != NULL && op2 != NULL)
        {
            InterCode code3 = new_Code();
            code3->kind = _ADD;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = op1;
            code3->operands.binop.op2 = op2;
            return code3;
        }
        else if (op1 == NULL && op2 != NULL)
        {
            Operand t1 = new_tempvar();
            InterCode code1 = translate_Exp((Exp->cld)[0], t1);
            InterCode code3 = new_Code();
            code3->kind = _ADD;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = t1;
            code3->operands.binop.op2 = op2;
            return add_Codes(2, code1, code3);
        }
        else if (op1 != NULL && op2 == NULL)
        {
            Operand t2 = new_tempvar();
            InterCode code2 = translate_Exp((Exp->cld)[2], t2);
            InterCode code3 = new_Code();
            code3->kind = _ADD;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = op1;
            code3->operands.binop.op2 = t2;
            return add_Codes(2, code2, code3);
        }
        else
        {
            Operand t1 = new_tempvar();
            Operand t2 = new_tempvar();
            InterCode code1 = translate_Exp((Exp->cld)[0], t1);
            InterCode code2 = translate_Exp((Exp->cld)[2], t2);
            InterCode code3 = new_Code();
            code3->kind = _ADD;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = t1;
            code3->operands.binop.op2 = t2;
            return add_Codes(3, code1, code2, code3);
        }
    }
    // Exp MINUS Exp
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "MINUS"))
    {
        Operand op1 = get_Operand((Exp->cld)[0]);
        Operand op2 = get_Operand((Exp->cld)[2]);
        if (op1 != NULL && op2 != NULL)
        {
            InterCode code3 = new_Code();
            code3->kind = _SUB;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = op1;
            code3->operands.binop.op2 = op2;
            return code3;
        }
        else if (op1 == NULL && op2 != NULL)
        {
            Operand t1 = new_tempvar();
            InterCode code1 = translate_Exp((Exp->cld)[0], t1);
            InterCode code3 = new_Code();
            code3->kind = _SUB;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = t1;
            code3->operands.binop.op2 = op2;
            return add_Codes(2, code1, code3);
        }
        else if (op1 != NULL && op2 == NULL)
        {
            Operand t2 = new_tempvar();
            InterCode code2 = translate_Exp((Exp->cld)[2], t2);
            InterCode code3 = new_Code();
            code3->kind = _SUB;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = op1;
            code3->operands.binop.op2 = t2;
            return add_Codes(2, code2, code3);
        }
        else
        {
            Operand t1 = new_tempvar();
            Operand t2 = new_tempvar();
            InterCode code1 = translate_Exp((Exp->cld)[0], t1);
            InterCode code2 = translate_Exp((Exp->cld)[2], t2);
            InterCode code3 = new_Code();
            code3->kind = _SUB;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = t1;
            code3->operands.binop.op2 = t2;
            return add_Codes(3, code1, code2, code3);
        }
    }
    // Exp STAR Exp
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "STAR"))
    {
        Operand op1 = get_Operand((Exp->cld)[0]);
        Operand op2 = get_Operand((Exp->cld)[2]);
        if (op1 != NULL && op2 != NULL)
        {
            InterCode code3 = new_Code();
            code3->kind = _MUL;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = op1;
            code3->operands.binop.op2 = op2;
            return code3;
        }
        else if (op1 == NULL && op2 != NULL)
        {
            Operand t1 = new_tempvar();
            InterCode code1 = translate_Exp((Exp->cld)[0], t1);
            InterCode code3 = new_Code();
            code3->kind = _MUL;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = t1;
            code3->operands.binop.op2 = op2;
            return add_Codes(2, code1, code3);
        }
        else if (op1 != NULL && op2 == NULL)
        {
            Operand t2 = new_tempvar();
            InterCode code2 = translate_Exp((Exp->cld)[2], t2);
            InterCode code3 = new_Code();
            code3->kind = _MUL;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = op1;
            code3->operands.binop.op2 = t2;
            return add_Codes(2, code2, code3);
        }
        else
        {
            Operand t1 = new_tempvar();
            Operand t2 = new_tempvar();
            InterCode code1 = translate_Exp((Exp->cld)[0], t1);
            InterCode code2 = translate_Exp((Exp->cld)[2], t2);
            InterCode code3 = new_Code();
            code3->kind = _MUL;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = t1;
            code3->operands.binop.op2 = t2;
            return add_Codes(3, code1, code2, code3);
        }
    }
    // Exp DIV Exp
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "DIV"))
    {
        Operand op1 = get_Operand((Exp->cld)[0]);
        Operand op2 = get_Operand((Exp->cld)[2]);
        if (op1 != NULL && op2 != NULL)
        {
            InterCode code3 = new_Code();
            code3->kind = _DIV;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = op1;
            code3->operands.binop.op2 = op2;
            return code3;
        }
        else if (op1 == NULL && op2 != NULL)
        {
            Operand t1 = new_tempvar();
            InterCode code1 = translate_Exp((Exp->cld)[0], t1);
            InterCode code3 = new_Code();
            code3->kind = _DIV;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = t1;
            code3->operands.binop.op2 = op2;
            return add_Codes(2, code1, code3);
        }
        else if (op1 != NULL && op2 == NULL)
        {
            Operand t2 = new_tempvar();
            InterCode code2 = translate_Exp((Exp->cld)[2], t2);
            InterCode code3 = new_Code();
            code3->kind = _DIV;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = op1;
            code3->operands.binop.op2 = t2;
            return add_Codes(2, code2, code3);
        }
        else
        {
            Operand t1 = new_tempvar();
            Operand t2 = new_tempvar();
            InterCode code1 = translate_Exp((Exp->cld)[0], t1);
            InterCode code2 = translate_Exp((Exp->cld)[2], t2);
            InterCode code3 = new_Code();
            code3->kind = _DIV;
            code3->operands.binop.result = place;
            code3->operands.binop.op1 = t1;
            code3->operands.binop.op2 = t2;
            return add_Codes(3, code1, code2, code3);
        }
    }
    // MINUS Exp
    else if (Exp->ncld == 2 && !strcmp((Exp->cld)[0]->name, "MINUS"))
    {
        Operand t1 = new_tempvar();
        InterCode code1 = translate_Exp((Exp->cld)[1], t1);
        InterCode code2 = new_Code();
        code2->kind = _SUB;
        code2->operands.binop.result = place;
        code2->operands.binop.op1 = new_Const(0);
        code2->operands.binop.op2 = t1;
        return add_Codes(2, code1, code2);
    }
    // Exp RELOP Exp
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "RELOP"))
    {
        isCond = 1;
    }
    // NOT Exp
    else if (Exp->ncld == 2 && !strcmp((Exp->cld)[0]->name, "NOT"))
    {
        isCond = 1;
    }
    // Exp AND Exp
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "AND"))
    {
        isCond = 1;
    }
    // Exp OR Exp
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "OR"))
    {
        isCond = 1;
    }
    // ID LP RP
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "LP"))
    {
        Operand function = new_Operand();
        function->kind = FUNC;
        function->operand.name = (Exp->cld)[0]->content;
        if (!strcmp(function->operand.name, "read"))
        {
            // READ函数处理
            InterCode code = new_Code();
            code->kind = _READ;
            code->operands.var = place;
            return code;
        }
        else
        {
            // 其他函数处理
            InterCode code = new_Code();
            code->kind = _CALL;
            code->operands.assign.left = place;
            code->operands.assign.right = function;
            return code;
        }
    }
    // ID LP Args RP
    else if (Exp->ncld == 4 && !strcmp((Exp->cld)[2]->name, "Args"))
    {
        int i;
        Operand function = new_Operand();
        function->kind = FUNC;
        function->operand.name = (Exp->cld)[0]->content;
        ArgList arg_list = (ArgList)malloc(sizeof(ArgListStru));
        arg_list->num = 0;
        InterCode code1 = translate_Args((Exp->cld)[2], arg_list);
        InterCode code2, code3;
        if (!strcmp(function->operand.name, "write"))
        {
            code2 = new_Code();
            code2->kind = _WRITE;
            code2->operands.var = (arg_list->list)[0];
            return add_Codes(2, code1, code2);
        }
        else
        {
            for (i = 0; i < arg_list->num; i++)
            {
                code2 = new_Code();
                code2->kind = _ARG;
                code2->operands.var = (arg_list->list)[i];
                code1 = add_Codes(2, code1, code2);
            }
            code3 = new_Code();
            code3->kind = _CALL;
            code3->operands.assign.left = place;
            code3->operands.assign.right = function;
            return add_Codes(2, code1, code3);
        }
    }
    else
    {
        printf("不能处理该类型的语句\n");
    }
    if (isCond)
    {
        Operand lable1 = new_lable();
        Operand lable2 = new_lable();
        InterCode code0 = new_assign_Code(place, new_Const(0));
        InterCode code1 = translate_Cond(Exp, lable1, lable2);
        InterCode code2 = add_Codes(2, new_lable_Code(lable1), new_assign_Code(place, new_Const(1)));
        return add_Codes(4, code0, code1, code2, new_lable_Code(lable2));
    }
    return new_Code();
}
// 条件表达式的翻译模式
InterCode translate_Cond(tnode Exp, Operand lable_true, Operand lable_false)
{
    // Exp RELOP Exp
    if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "RELOP"))
    {
        Operand op1 = get_Operand((Exp->cld)[0]);
        Operand op2 = get_Operand((Exp->cld)[2]);
        InterCode code3 = new_Code();
        code3->kind = _IFGOTO;
        code3->operands.jump.lable = lable_true;
        code3->operands.jump.relop = (Exp->cld)[1]->content;
        // 中间代码优化
        if (op1 == NULL && op2 == NULL)
        {
            Operand t1 = new_tempvar();
            Operand t2 = new_tempvar();
            InterCode code1 = translate_Exp((Exp->cld)[0], t1);
            InterCode code2 = translate_Exp((Exp->cld)[2], t2);
            code3->operands.jump.op1 = t1;
            code3->operands.jump.op2 = t2;
            return add_Codes(4, code1, code2, code3, new_goto_Code(lable_false));
        }
        else if (op1 == NULL && op2 != NULL)
        {
            Operand t1 = new_tempvar();
            InterCode code1 = translate_Exp((Exp->cld)[0], t1);
            code3->operands.jump.op1 = t1;
            code3->operands.jump.op2 = op2;
            return add_Codes(3, code1, code3, new_goto_Code(lable_false));
        }
        else if (op1 != NULL && op2 == NULL)
        {
            Operand t2 = new_tempvar();
            InterCode code2 = translate_Exp((Exp->cld)[2], t2);
            code3->operands.jump.op1 = op1;
            code3->operands.jump.op2 = t2;
            return add_Codes(3, code2, code3, new_goto_Code(lable_false));
        }
        else if (op1 != NULL && op2 != NULL)
        {
            code3->operands.jump.op1 = op1;
            code3->operands.jump.op2 = op2;
            return add_Codes(2, code3, new_goto_Code(lable_false));
        }
    }
    // NOT Exp
    else if (Exp->ncld == 2 && !strcmp((Exp->cld)[0]->name, "NOT"))
    {
        return translate_Cond((Exp->cld)[1], lable_false, lable_true);
    }
    // Exp AND Exp
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "AND"))
    {
        Operand lable1 = new_lable();
        InterCode code1 = translate_Cond((Exp->cld)[0], lable1, lable_false);
        InterCode code2 = translate_Cond((Exp->cld)[2], lable_true, lable_false);
        return add_Codes(3, code1, new_lable_Code(lable1), code2);
    }
    // Exp OR Exp
    else if (Exp->ncld == 3 && !strcmp((Exp->cld)[1]->name, "OR"))
    {
        Operand lable1 = new_lable();
        InterCode code1 = translate_Cond((Exp->cld)[0], lable_true, lable1);
        InterCode code2 = translate_Cond((Exp->cld)[2], lable_true, lable_false);
        return add_Codes(3, code1, new_lable_Code(lable1), code2);
    }
    // orther cases
    else
    {
        Operand t1 = new_tempvar();
        InterCode code1 = translate_Exp(Exp, t1);
        InterCode code2 = new_Code();
        char *relop = "!=";
        code2->kind = _IFGOTO;
        code2->operands.jump.lable = lable_true;
        code2->operands.jump.relop = relop;
        code2->operands.jump.op1 = t1;
        code2->operands.jump.op2 = new_Const(0);
        return add_Codes(3, code1, code2, new_goto_Code(lable_false));
    }
}
// 函数参数的翻译模式
InterCode translate_Args(tnode Args, ArgList arg_list)
{
    // Exp
    if (Args->ncld == 1)
    {
        Operand existOp = get_Operand((Args->cld)[0]);
        if (existOp != NULL)
        {
            if (existOp->kind == CONSTANT)
            {
                Operand t1 = new_tempvar();
                InterCode code1 = new_assign_Code(t1, existOp);
                arg_list->list[arg_list->num] = t1;
                arg_list->num++;
                return code1;
            }
            arg_list->list[arg_list->num] = existOp;
            arg_list->num++;
            return new_Code();
        }
        Operand t1 = new_tempvar();
        InterCode code1 = translate_Exp((Args->cld)[0], t1);
        arg_list->list[arg_list->num] = t1;
        arg_list->num++;
        return code1;
    }
    // Exp COMMA Args
    else if (Args->ncld == 3)
    {
        Operand t1 = new_tempvar();
        InterCode code1 = translate_Exp((Args->cld)[0], t1);
        arg_list->list[arg_list->num] = t1;
        arg_list->num++;
        InterCode code2 = translate_Args((Args->cld)[2], arg_list);
        return add_Codes(2, code1, code2);
    }
    return new_Code();
}
