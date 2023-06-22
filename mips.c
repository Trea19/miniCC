#include "compiler.h"

// 十进制整型转字符串
char *Int2String(int num, char *str){
    int i = 0;   //指示填充str
    if (num < 0){
        num = -num;
        str[i ++] = '-';
    }
    //转换
    do {
        str[i++] = num % 10 + 48; //取num最低位 字符0~9的ASCII码是48~57；简单来说数字0+48=48，ASCII码对应字符'0'
        num /= 10;                //去掉最低位
    } while (num);                //num不为0继续循环

    str[i] = '\0';

    //确定开始调整的位置
    int j = 0;
    if (str[0] == '-'){
        j = 1; 
        ++ i; 
    }

    //对称交换
    for (; j < i / 2; j++){
        // a=a+b;b=a-b;a=a-b;
        str[j] = str[j] + str[i - 1 - j];
        str[i - 1 - j] = str[j] - str[i - 1 - j];
        str[j] = str[j] - str[i - 1 - j];
    }

    return str; 
}

// 分配寄存器
char* allocate_reg(Operand op){
    char *regnumber = (char *)malloc(sizeof(char) * 10);
    char *regname = (char *)malloc(sizeof(char) * 10);
    strcat(regname, "$t");

    // 常数或临时变量值为0，直接返回zero
    if (op->kind == CONSTANT && op->operand.value == 0)
        return "$zero";
    else if (op->kind == TEMPVAR && op->value == 0)
        return "$zero";

    // 寻找存储该操作数的寄存器
    int find = 0;
    int i;
    for (i = 0; i < reg_num; i++){
        if (regs[i] == NULL || regs[i]->kind != op->kind)
            continue;
        if (regs[i]->kind == CONSTANT && regs[i]->operand.value == op->operand.value){
            find = 1;
            break;
        }
        else if (regs[i]->kind == TEMPVAR && regs[i]->operand.tempvar == op->operand.tempvar){
            find = 1;
            break;
        }
        else if (regs[i]->kind == VARIABLE && !strcmp(regs[i]->operand.name, op->operand.name)){
            find = 1;
            break;
        }
    }
    if (find){
        Int2String(i, regnumber);
        strcat(regname, regnumber);
        return regname;
    } else { // 分配空闲寄存器
        Int2String(reg_num, regnumber);
        strcat(regname, regnumber);
        regs[reg_num] = op;
        reg_num++;
        if (reg_num == 10) {
            reg_num = 0;
        }
        return regname;
    }
}

// 翻译单条中间代码
void generate_MIPS_Code(InterCode code){
    if (code == NULL){
        printf("Error, MIPS is NULL\n");
        return;
    }

    switch (code->kind){
    case _NULL:
        break;
    case _LABLE:
    {
        print_Operand(code->operands.var);
        printf(":\n");
        break;
    }
    case _FUNCTION:
    {
        print_Operand(code->operands.var);
        printf(":\n");
        break;
    }
    case _ASSIGN:
    {
        Operand left = code->operands.assign.left;
        Operand right = code->operands.assign.right;
        if (right->kind == CONSTANT) {
            // 如果将0赋给一个临时变量，则不需要输出该mips代码
            if (left->kind == TEMPVAR && right->value == 0)
                break;
            else
                printf("\tli %s, %d\n", allocate_reg(left), right->operand.value);
        } else {
            printf("\tmove %s, %s\n", allocate_reg(left), allocate_reg(right));
        }
        break;
    }
    case _ADD:
    {
        Operand result = code->operands.binop.result;
        Operand op1 = code->operands.binop.op1;
        Operand op2 = code->operands.binop.op2;
        if (op2->kind == CONSTANT){
            printf("\taddi %s, %s, %d\n", allocate_reg(result), allocate_reg(op1), op2->value);
        }
        else { 
            printf("\tadd %s, %s, %s\n", allocate_reg(result), allocate_reg(op1), allocate_reg(op2));
        }
        break;
    }
    case _SUB:
    {
        Operand result = code->operands.binop.result;
        Operand op1 = code->operands.binop.op1;
        Operand op2 = code->operands.binop.op2;
        if (op2->kind == CONSTANT){
            printf("\taddi %s, %s, %d\n", allocate_reg(result), allocate_reg(op1), -(op2->value));
        }
        else{
            printf("\tsub %s, %s, %s\n", allocate_reg(result), allocate_reg(op1), allocate_reg(op2));
        }
        break;
    }
    case _MUL:
    {
        Operand result = code->operands.binop.result;
        Operand op1 = code->operands.binop.op1;
        Operand op2 = code->operands.binop.op2;
        printf("\tmul %s, %s, %s\n", allocate_reg(result), allocate_reg(op1), allocate_reg(op2));
        break;
    }
    case _DIV:
    {
        Operand result = code->operands.binop.result;
        Operand op1 = code->operands.binop.op1;
        Operand op2 = code->operands.binop.op2;
        printf("\tdiv %s, %s\n", allocate_reg(op1), allocate_reg(op2));
        printf("\tmflo %s\n", allocate_reg(result));
        break;
    }
    case _GOTO:
    {
        Operand lable = code->operands.jump.lable;
        printf("\tj ");
        print_Operand(lable);
        printf("\n");
        break;
    }
    case _CALL:
    {
        break;
    }
    case _READ:
    {
        Operand op = code->operands.var;
        printf("\taddi $sp, $sp, -4\n");
        printf("\tsw $ra, 0($sp)\n");
        printf("\tjal read\n");
        printf("\tlw $ra, 0($sp)\n");
        printf("\taddi $sp, $sp, 4\n");
        printf("\tmove %s, $v0\n", allocate_reg(op));
        break;
    }
    case _WRITE:
    {
        Operand op = code->operands.var;
        printf("\tmove $a0, %s\n", allocate_reg(op));
        printf("\taddi $sp, $sp, -4\n");
        printf("\tsw $ra, 0($sp)\n");
        printf("\tjal write\n");
        printf("\tlw $ra, 0($sp)\n");
        printf("\taddi $sp, $sp, 4\n");
        break;
    }
    case _RETURN:
    {
        Operand res = code->operands.var;
        printf("\tmove $v0, %s\n", allocate_reg(res));
        printf("\tjr $ra\n");
        break;
    }
    case _IFGOTO:
    {
        char *op = code->operands.jump.relop;
        Operand lable = code->operands.jump.lable;
        Operand op1 = code->operands.jump.op1;
        Operand op2 = code->operands.jump.op2;
        if (!strcmp(op, "=="))
        {
            printf("\tbeq %s, %s, ", allocate_reg(op1), allocate_reg(op2));
            print_Operand(lable);
            printf("\n");
        }
        else if (!strcmp(op, "!="))
        {
            printf("\tbne %s, %s, ", allocate_reg(op1), allocate_reg(op2));
            print_Operand(lable);
            printf("\n");
        }
        else if (!strcmp(op, ">"))
        {
            printf("\tbgt %s, %s, ", allocate_reg(op1), allocate_reg(op2));
            print_Operand(lable);
            printf("\n");
        }
        else if (!strcmp(op, "<"))
        {
            printf("\tblt %s, %s, ", allocate_reg(op1), allocate_reg(op2));
            print_Operand(lable);
            printf("\n");
        }
        else if (!strcmp(op, ">="))
        {
            printf("\tbge %s, %s, ", allocate_reg(op1), allocate_reg(op2));
            print_Operand(lable);
            printf("\n");
        }
        else if (!strcmp(op, "<="))
        {
            printf("\tble %s, %s, ", allocate_reg(op1), allocate_reg(op2));
            print_Operand(lable);
            printf("\n");
        }
        break;
    }
    default:
        break;
    }
}

// 根据中间代码生成mips代码
void generate_MIPS_Codes(InterCode codes){
    printf("\n目标代码打印：\n");
    // 声明部分
    printf(".data\n_prompt: .asciiz \"Enter an integer:\"\n");
    printf("_ret: .asciiz \"\\n\"\n.globl main\n.text\n");
    // read函数
    printf("read:\n\tli $v0, 4\n\tla $a0, _prompt\n\tsyscall\n");
    printf("\tli $v0, 5\n\tsyscall\n\tjr $ra\n\n");
    // write函数
    printf("write:\n\tli $v0, 1\n\tsyscall\n\tli $v0, 4\n\tla $a0, _ret\n");
    printf("\tsyscall\n\tmove $v0, $0\n\tjr $ra\n\n");
    InterCode temp = new_Code();
    temp = codes;
    while (temp != NULL){
        generate_MIPS_Code(temp);
        temp = temp->next;
    }
    printf("打印完毕\n");
}
