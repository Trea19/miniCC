#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> 

/**********************语法分析**************************/

// 行数
extern int yylineno;
// 文本
extern char *yytext;
// 错误处理
void yyerror(char *msg);

// 抽象语法树
typedef struct treeNode {
    // 行号
    int line;
    // Token类型
    char* name;
    
    // 数据类型 int
    char *type;
    // 变量的值
    int value;

    // 1变量 2函数 3常数 4数组 5结构体
    int tag;
    // 语义值
    char *content;

    // 使用孩子数组表示法
    struct treeNode *cld[10];
    int ncld; //几个子节点

} * Ast, *tnode;

// 所有节点数量
int nodeNum;
// 存放所有节点
tnode nodeList[5000];
int nodeIsChild[5000];


// 构造抽象语法树(节点)
Ast newAst(char *name, int num, ...);

// 先序遍历语法树
void preOrderAst(Ast ast, int level);

// 设置某结点为子节点
void setChildTag(tnode node);

// 是否有词法语法错误
int hasFault;

/**********************语义分析**************************/

// 变量符号表的结点
typedef struct var_
{
    char *name;
    char *type;
    // 是否为结构体域
    int inStruc;
    // 所属的结构体编号
    int strucNum;
    struct var_ *next;
} var;

var  *varhead, *vartail;

// 建立变量符号
void newVar(int num,...);

// 变量是否已经定义
int  findVar(tnode val);

// 变量类型
char* typevar(tnode val);

// 赋值号左边仅能出现ID、Exp LB Exp RB 以及 Exp DOT ID
int checkLeft(tnode val);

// 函数符号表的结点
typedef struct func_ {
    int tag; // 0表示未定义，1表示定义
    char *name;
    char *type;
    int inStruc; // 是否为结构体域
    int strucNum; // 所属的结构体编号
    char *rtype; //声明返回值类型
    int va_num;  //记录函数形参个数
    char *va_type[10];
    struct func_ *next;
} func;

func *funchead,*functail;

// 记录函数实参
int va_num;
char* va_type[10];
void getdetype(tnode val);//定义的参数
void getretype(tnode val);//实际的参数
void getargs(tnode Args);//获取实参
int checkrtype(tnode ID,tnode Args);//检查形参与实参是否一致
// 建立函数符号
void newFunc(int num, ...);
// 函数是否已经定义
int findFunc(tnode val);
// 函数类型
char *getFuncType(tnode val);
// 函数的形参个数
int getParameterNum(tnode val);
// 函数实际返回值类型
char *rtype[10];
int rnum;
void getRType(tnode val);

// 数组符号表的结点
typedef struct array_ {
    char *name;
    char *type;
    // 是否为结构体域
    int inStruc;
    // 所属的结构体编号
    int strucNum;
    struct array_ *next;
} array;
array *arrayhead,*arraytail;
// 建立数组符号
void newArray(int num, ...);
// 查找数组是否已经定义
int findArray(tnode val);
// 数组类型
char *typearray(tnode val);

// 结构体符号表的结点
typedef struct struc_
{
    char *name;
    char *type;
    // 是否为结构体域
    int inStruc;
    // 所属的结构体编号
    int strucNum;
    struct struc_ *next;
} struc;

struc *struchead, *structail;
// 建立结构体符号
void newstruc(int num, ...);
// 查找结构体是否已经定义
int findstruc(tnode val);
// 当前是结构体域
int inStruc;
// 判断结构体域，{ 和 }是否抵消
int LCnum;
// 当前是第几个结构体
int strucNum;


