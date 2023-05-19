#include "syntax_tree.h"

// name: 当前创建结点名称
// num: > 0 表示该节点不是终结符，存在num个子节点；= 0 为终结符结点
Ast newAst(char *name, int num, ...){
    // 生成父节点
    tnode father = (tnode)malloc(sizeof(struct treeNode));
    // 添加子节点
    tnode temp = (tnode)malloc(sizeof(struct treeNode));
    if (!father){
        yyerror("create treenode error");
        exit(0);
    }
    father->name = name;

    // 参数列表，详见 stdarg.h 用法
    va_list list;
    // 初始化参数列表
    va_start(list, num);

    if (num > 0) { // 表示当前节点不是终结符号，还有子节点
        father->ncld = num;
        // 第一个孩子节点
        temp = va_arg(list, tnode);
        father->cld[0] = temp;
        setChildTag(temp);
        // 父节点行号为第一个孩子节点的行号
        father->line = temp->line;

        if (num == 1) {
            //父节点的语义值等于左孩子的语义值
            father->content = temp->content;
            father->tag = temp->tag;
        } else {
            for (int i = 1; i < num; i++) {
                temp = va_arg(list, tnode);
                (father->cld)[i] = temp;
                setChildTag(temp);
            }
        }
    } else { //表示当前节点是终结符（叶节点）或者空的语法单元
        father->ncld = 0;
        father->line = va_arg(list, int);

        if (!strcmp(name, "INT")){ // name == "INT"
            father->type = "int";
            father->value = atoi(yytext);
        } else {  // 存储词法单元语义值 
            char *str;
            str = (char *)malloc(sizeof(char) * 40);
            strcpy(str, yytext);
            father->content = str;
        }
    }
    nodeList[nodeNum ++] = father;

    return father;
}

// 前序遍历
void preOrderAst(Ast ast, int level){
    if (ast == NULL) return ;
    else {
        for (int i = 0; i < level; i ++)
            printf("  "); // 层级缩进
        
        if (ast -> line != -1) {
            printf("%s", ast -> name); // token类型

            if ((!strcmp(ast->name, "ID")) || (!strcmp(ast->name, "TYPE")))
                printf(": %s", ast->content);
            else if (!strcmp(ast->name, "INT"))
                printf(": %d", (int)ast->value);
            else // 非叶节点打印行号
                printf("(%d)", ast->line);
            
            printf("\n");
            for (int i = 0; i < ast -> ncld; i ++)
                preOrderAst((ast->cld)[i], level + 1);
        }
    }
}

void yyerror(char *msg) {
    hasFault = 1;
    fprintf(stderr, "Error at Line %d: %s before %s\n", yylineno, msg, yytext);
}

// 标记结点为子节点
void setChildTag(tnode node) {
    for (int i = 0; i < nodeNum; i++){
        if (nodeList[i] == node)
            nodeIsChild[i] = 1;
    }
}

// 建立变量符号 e.g. newVar(2,$1,$2)
void newVar(int num, ...) {
    va_list valist;
    va_start(valist, num);

    var *res = (var *)malloc(sizeof(var));
    tnode temp = (tnode)malloc(sizeof(tnode));

    if (inStruc && LCnum){ // 是结构体域中
        res->inStruc = 1; // 标记当前是结构体域
        res->strucNum = strucNum; //标记是第几个结构体
    } else {
        res->inStruc = 0; // 不是结构体域
        res->strucNum = 0;
    }

    // 变量声明 e.g. int i
    temp = va_arg(valist, tnode);
    res-> type = temp->content;
    temp = va_arg(valist, tnode);
    res-> name = temp->content;

    vartail->next = res;
    vartail = res;
}

// 查找变量, 检查是否存在变量重复定义
int findVar(tnode val) {
    var *temp = (var *)malloc(sizeof(var *));
    temp = varhead->next;
    while (temp != NULL){
        if (!strcmp(temp->name, val->content)){ //找到相等的
            if (inStruc && LCnum){ // 当前变量是结构体域
                if (temp -> inStruc && temp -> strucNum == strucNum) {
                    return 1; //在同一个结构体域中重名
                }
            } else { // 当前结构体外的变量
                if (!temp -> inStruc) {
                    return 1; //结构体外的变量重定义
                }
            }
        }
        temp = temp->next;
    }
    return 0;
}

// 得到变量类型
char* typevar(tnode val){
    var *temp = (var *)malloc(sizeof(var *));
    temp = varhead->next;
    while (temp != NULL){
        if (!strcmp(temp->name, val->content))
            return temp->type; //返回变量类型

        temp = temp->next;
    }
    return NULL;
}

// 赋值号左边只能出现标识符、数组、结构体的变量, 1正0误
// <ID>、<Exp LB Exp RB>、<Exp DOT ID>
int checkLeft(tnode val){
    if (val->ncld == 1 && !strcmp((val->cld)[0]->name, "ID"))
        return 1;
    else if (val->ncld == 4 && !strcmp((val->cld)[0]->name, "Exp") && !strcmp((val->cld)[1]->name, "LB") && !strcmp((val->cld)[2]->name, "Exp") && !strcmp((val->cld)[3]->name, "RB"))
        return 1;
    else if (val->ncld == 3 && !strcmp((val->cld)[0]->name, "Exp") && !strcmp((val->cld)[1]->name, "DOT") && !strcmp((val->cld)[2]->name, "ID"))
        return 1;
    else
        return 0;
}

// 创建函数符号
void newFunc(int num, ...) {
    va_list valist;
    va_start(valist, num);

    tnode temp = (tnode)malloc(sizeof(tnode));

    switch (num){
    case 1: // 函数返回值
        if (inStruc && LCnum){ // 是结构体的域
            functail->inStruc = 1;
            functail->strucNum = strucNum;
        } else {
            functail->inStruc = 0;
            functail->strucNum = 0;
        }
        temp = va_arg(valist, tnode);
        functail-> rtype = temp->content;
        functail-> type = temp->type;
        for (int i = 0; i < rnum; i++){
            if (rtype[i] == NULL || strcmp(rtype[i], functail -> rtype))
                printf("Error at Line %d:Func return type error.\n", yylineno);
        }
        functail ->tag = 1; //标志为已定义
        func *new = (func *)malloc(sizeof(func));
        functail->next = new; //尾指针指向下一个空结点
        functail = new;
        break;
    case 2: // 函数名和声明时的参数
        //记录函数名
        temp = va_arg(valist, tnode);
        functail -> name = temp -> content;
        //设置函数声明时的参数
        temp = va_arg(valist, tnode);
        functail->va_num = 0;
        getdetype(temp);
        break;
    default:
        break;
    }
}

//定义的参数
void getdetype(tnode val) {
    if (val != NULL){
        if (!strcmp(val->name, "ParamDec")) {
            functail->va_type[functail->va_num] = val->cld[0]->content;
            functail->va_num++;
            return;
        }
        for (int i = 0; i < val->ncld; ++i)
        {
            getdetype((val->cld)[i]);
        }
    }
    else
        return;
}

//实际的参数
void getretype(tnode val)
{
    int i;
    if (val != NULL)
    {
        if (!strcmp(val->name, "Exp"))
        {
            va_type[va_num] = val->type;
            va_num++;
            return;
        }
        for (i = 0; i < val->ncld; ++i)
        {
            getretype((val->cld)[i]);
        }
    }
    else
        return;
}

//获取函数实际返回值类型
void getRType(tnode val){
    rtype[rnum ++] = val->type;
}

//检查形参与实参是否一致,没有错误返回0
int checkrtype(tnode ID, tnode Args)
{
    int i;
    va_num = 0;
    getretype(Args);
    func *temp = (func *)malloc(sizeof(func *));
    temp = funchead->next;
    while (temp != NULL && temp->name != NULL && temp->tag == 1)
    {
        if (!strcmp(temp->name, ID->content))
            break;
        temp = temp->next;
    }
    if (va_num != temp->va_num)
        return 1;
    for (i = 0; i < temp->va_num; i++)
    {
        if (temp->va_type[i] == NULL || va_type[i] == NULL || strcmp(temp->va_type[i], va_type[i]) != 0)
            return 1;
    }
    return 0;
}

// 函数是否已经定义
int findFunc(tnode val){
    func *temp = (func *)malloc(sizeof(func *));
    temp = funchead->next;
    while (temp != NULL && temp->name != NULL && temp->tag == 1){
        if (!strcmp(temp->name, val->content)){
            if (inStruc && LCnum){ // 当前变量是结构体域
                if (temp -> inStruc && temp -> strucNum == strucNum){
                    return 1; //结构体中函数明重定义
                }
            } else { // 当前变量在结构体外
                if (!temp -> inStruc) {
                    return 1; //结构体外函数重定义
                }
            }
        }
        temp = temp->next;
    }
    return 0;
}

// 函数类型
char* getFuncType(tnode val){
    func *temp = (func *)malloc(sizeof(func *));
    temp = funchead->next;
    while (temp != NULL){
        if (!strcmp(temp->name, val->content))
            return temp->type; //返回函数类型
        temp = temp->next;
    }
    return NULL;
}

// 返回形参个数
int getParameterNum(tnode val) {
    func *temp = (func *)malloc(sizeof(func*));
    temp = funchead->next;
    while (temp != NULL) {
        if (!strcmp(temp->name, val->content))
            return temp->va_num; //返回形参个数
        temp = temp->next;
    }
}

// 创建数组符号表
void newArray(int num, ...){
    va_list valist;
    va_start(valist, num);

    array *res = (array *)malloc(sizeof(array));
    tnode temp = (tnode)malloc(sizeof(struct treeNode));

    if (inStruc && LCnum){
        // 是结构体的域
        res->inStruc = 1;
        res->strucNum = strucNum;
    } else {
        res->inStruc = 0;
        res->strucNum = 0;
    }
    // int a[10]
    temp = va_arg(valist, tnode);
    res->type = temp->content;
    temp = va_arg(valist, tnode);
    res->name = temp->content;
    arraytail->next = res;
    arraytail = res;
}

// 数组是否已经定义
int findArray(tnode val){
    array *temp = (array *)malloc(sizeof(array *));
    temp = arrayhead->next;
    while (temp != NULL){
        if (!strcmp(temp->name, val->content))
            return 1;
        temp = temp->next;
    }
    return 0;
}

// 数组类型
char *typearray(tnode val)
{
    array *temp = (array *)malloc(sizeof(array *));
    temp = arrayhead->next;
    while (temp != NULL)
    {
        if (!strcmp(temp->name, val->content))
            return temp->type; //返回数组类型
        temp = temp->next;
    }
    return NULL;
}

// 创建结构体符号表
void newstruc(int num, ...) {
    va_list valist;
    va_start(valist, num);

    struc *res = (struc *)malloc(sizeof(struc));
    tnode temp = (tnode)malloc(sizeof(struct treeNode));

    // struct name{}
    temp = va_arg(valist, tnode);
    res->name = temp->content;
    structail->next = res;
    structail = res;
}

// 结构体是否和结构体或变量的名字重复
int findstruc(tnode val)
{
    struc *temp = (struc *)malloc(sizeof(struc *));
    temp = struchead->next;
    while (temp != NULL)
    {
        if (!strcmp(temp->name, val->content))
            return 1;
        temp = temp->next;
    }
    if (findVar(val) == 1)
        return 1;
    return 0;
}

// 主函数 扫描文件并且分析
// bison会自己调用yylex()，所以在main函数中不需要再调用它了
// bison使用yyparse()进行语法分析，所以需要调用yyparse()和yyrestart()
int main(int argc, char **argv) {
    if (argc < 2)
        return 1;
    
    for (int i = 1; i < argc; i++){
        // 初始化，用于记录结构体
        inStruc = 0;
        LCnum = 0;
        strucNum = 0;

        // 初始化符号表
        varhead = (var *)malloc(sizeof(var));
        vartail = varhead;
        funchead = (func *)malloc(sizeof(func));
        functail = (func *)malloc(sizeof(func));
        funchead->next = functail;
        functail->va_num = 0;
        arrayhead = (array *)malloc(sizeof(array));
        arraytail = arrayhead;
        struchead = (struc *)malloc(sizeof(struc));
        structail = struchead;
        rnum = 0;

        // 初始化节点记录列表
        nodeNum = 0;
        memset(nodeList, 0, sizeof(tnode) * 500);
        memset(nodeIsChild, 0, sizeof(int) * 500);
        hasFault = 0;

        FILE *f = fopen(argv[i], "r");
        if (!f){
            perror(argv[i]);
            return 1;
        }

        yyrestart(f);
        yyparse();
        fclose(f);

        //存在语法错误，下一个文件
        if (hasFault)
            continue;
  
        for (int j = 0; j < nodeNum; j++){
            if (nodeIsChild[j] != 1) { // 遍历所有非子节点的节点
                preOrderAst(nodeList[j], 0);
            }
        }
    }
    return 0;
}
