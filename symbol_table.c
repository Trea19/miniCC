#include "compiler.h"

/******************语义分析符号表********************/

// 先序遍历分析
void analysis(Ast ast)
{
    int i;
    if (ast != NULL)
    {
        for (i = 0; i < ast->ncld; ++i)
        {
            analysis((ast->cld)[i]);
        }
    }
    else
        return;
}

// 建立变量符号
void newvar(int num, ...)
{
    va_list valist;
    va_start(valist, num);

    var *res = (var *)malloc(sizeof(var));
    tnode temp = (tnode)malloc(sizeof(tnode));

    if (inStruc && LCnum)
    {
        // 是结构体的域
        res->inStruc = 1;
        res->strucNum = strucNum;
    }
    else
    {
        res->inStruc = 0;
        res->strucNum = 0;
    }

    // 变量声明 int i,j
    res->type = curType;
    temp = va_arg(valist, tnode);
    res->name = temp->content;

    vartail->next = res;
    vartail = res;
}
// 查找变量
int findvar(tnode val)
{
    var *temp = (var *)malloc(sizeof(var *));
    temp = varhead->next;
    while (temp != NULL)
    {
        if (!strcmp(temp->name, val->content))
        {
            if (inStruc && LCnum) // 当前变量是结构体域
            {
                if (!temp->inStruc)
                {
                    // 结构体域与变量重名
                    printf("Error type 9 at Line %d:Struct Field and Variable use the same name.\n", yylineno);
                }
                else if (temp->inStruc && temp->strucNum != strucNum)
                {
                    // 不同结构体中的域重名
                    printf("Error type 10 at Line %d:Struct Fields use the same name.\n", yylineno);
                }
                else
                {
                    // 同一结构体中域名重复
                    return 1;
                }
            }
            else // 当前变量是全局变量
            {
                if (temp->inStruc)
                {
                    // 变量与结构体域重名
                    printf("Error type 9 at Line %d:Struct Field and Variable use the same name.\n", yylineno);
                }
                else
                {
                    // 变量与变量重名，即重复定义
                    return 1;
                }
            }
        }
        temp = temp->next;
    }
    return 0;
}
// 变量类型
char *typevar(tnode val)
{
    var *temp = (var *)malloc(sizeof(var *));
    temp = varhead->next;
    while (temp != NULL)
    {
        if (!strcmp(temp->name, val->content))
            return temp->type; //返回变量类型
        temp = temp->next;
    }
    return NULL;
}
// 赋值号左边只能出现ID、Exp LB Exp RB 以及 Exp DOT ID
int checkleft(tnode val)
{
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
void newfunc(int num, ...)
{
    int i;
    va_list valist;
    va_start(valist, num);

    tnode temp = (tnode)malloc(sizeof(struct treeNode));

    switch (num)
    {
    case 1:
        if (inStruc && LCnum)
        {
            // 是结构体的域
            functail->inStruc = 1;
            functail->strucNum = strucNum;
        }
        else
        {
            functail->inStruc = 0;
            functail->strucNum = 0;
        }
        //设置函数返回值类型
        temp = va_arg(valist, tnode);
        functail->rtype = temp->content;
        functail->type = temp->type;
        for (i = 0; i < rnum; i++)
        {
            if (rtype[i] == NULL || strcmp(rtype[i], functail->rtype))
                printf("Error type 12 at Line %d:Func return type error.\n", yylineno);
        }
        functail->tag = 1; //标志为已定义
        func *new = (func *)malloc(sizeof(func));
        functail->next = new; //尾指针指向下一个空结点
        functail = new;
        break;
    case 2:
        //记录函数名
        temp = va_arg(valist, tnode);
        functail->name = temp->content;
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
void getdetype(tnode val)
{
    int i;
    if (val != NULL)
    {
        if (!strcmp(val->name, "ParamDec"))
        {
            functail->va_type[functail->va_num] = (val->cld)[0]->content;
            functail->va_num++;
            return;
        }
        for (i = 0; i < val->ncld; ++i)
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
//函数实际返回值类型
void getrtype(tnode val)
{
    rtype[rnum] = val->type;
    rnum++;
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
int findfunc(tnode val)
{
    func *temp = (func *)malloc(sizeof(func *));
    temp = funchead->next;
    while (temp != NULL && temp->name != NULL && temp->tag == 1)
    {
        if (!strcmp(temp->name, val->content))
        {
            if (inStruc && LCnum) // 当前变量是结构体域
            {
                if (!temp->inStruc)
                {
                    // 结构体域与变量重名
                    printf("Error type 9 at Line %d:Struct Field and Variable use the same name.\n", yylineno);
                }
                else if (temp->inStruc && temp->strucNum != strucNum)
                {
                    // 不同结构体中的域重名
                    printf("Error type 10 at Line %d:Struct Fields use the same name.\n", yylineno);
                }
                else
                {
                    // 同一结构体中域名重复
                    return 1;
                }
            }
            else // 当前变量是全局变量
            {
                if (temp->inStruc)
                {
                    // 变量与结构体域重名
                    printf("Error type 9 at Line %d:Struct Field and Variable use the same name.\n", yylineno);
                }
                else
                {
                    // 变量与变量重名，即重复定义
                    return 1;
                }
            }
        }
        temp = temp->next;
    }
    return 0;
}
// 函数类型
char *typefunc(tnode val)
{
    func *temp = (func *)malloc(sizeof(func *));
    temp = funchead->next;
    while (temp != NULL)
    {
        if (!strcmp(temp->name, val->content))
            return temp->rtype; //返回函数类型
        temp = temp->next;
    }
    return NULL;
}
// 形参个数
int numfunc(tnode val)
{
    func *temp = (func *)malloc(sizeof(func *));
    temp = funchead->next;
    while (temp != NULL)
    {
        if (!strcmp(temp->name, val->content))
            return temp->va_num; //返回形参个数
        temp = temp->next;
    }
}
// 创建数组符号表
void newarray(int num, ...)
{
    va_list valist;
    va_start(valist, num);

    array *res = (array *)malloc(sizeof(array));
    tnode temp = (tnode)malloc(sizeof(struct treeNode));

    if (inStruc && LCnum)
    {
        // 是结构体的域
        res->inStruc = 1;
        res->strucNum = strucNum;
    }
    else
    {
        res->inStruc = 0;
        res->strucNum = 0;
    }
    // int a[10]
    res->type = curType;
    temp = va_arg(valist, tnode);
    res->name = temp->content;
    arraytail->next = res;
    arraytail = res;
}
// 数组是否已经定义
int findarray(tnode val)
{
    array *temp = (array *)malloc(sizeof(array *));
    temp = arrayhead->next;
    while (temp != NULL)
    {
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
void newstruc(int num, ...)
{
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
    if (findvar(val) == 1)
        return 1;
    return 0;
}