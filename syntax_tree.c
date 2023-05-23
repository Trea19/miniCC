#include "compiler.h"

// 用于遍历
int i;

Ast newAst(char *name, int num, ...)
{
    // 生成父节点
    tnode father = (tnode)malloc(sizeof(struct treeNode));
    // 添加子节点
    tnode temp = (tnode)malloc(sizeof(struct treeNode));
    if (!father)
    {
        yyerror("create treenode error");
        exit(0);
    }
    father->name = name;
    father->type = NULL;

    // 参数列表，详见 stdarg.h 用法
    va_list list;
    // 初始化参数列表
    va_start(list, num);

    // 表示当前节点不是终结符号，还有子节点
    if (num > 0)
    {
        father->ncld = num;
        // 第一个孩子节点
        temp = va_arg(list, tnode);
        (father->cld)[0] = temp;
        setChildTag(temp);
        // 父节点行号为第一个孩子节点的行号
        father->line = temp->line;
        father->type = temp->type;

        if (num == 1)
        {
            //父节点的语义值等于左孩子的语义值
            father->content = temp->content;
            father->tag = temp->tag;
        }
        else
        {
            for (i = 1; i < num; i++)
            {
                temp = va_arg(list, tnode);
                (father->cld)[i] = temp;
                // 该节点为其他节点的子节点
                setChildTag(temp);
            }
        }
    }
    else //表示当前节点是终结符（叶节点）或者空的语法单元，此时num表示行号（空单元为-1）
    {
        father->ncld = 0;
        father->line = va_arg(list, int);
        // strcmp()==0 表示相同
        if (!strcmp(name, "INT"))
        {
            father->type = "int";
            father->value = atoi(yytext);
        }
        else if (!strcmp(name, "FLOAT"))
        {
            father->type = "float";
            father->value = atof(yytext);
        }
        else
        {
            father->type = curType;
            // 存储词法单元语义值
            char *str;
            str = (char *)malloc(sizeof(char) * 40);
            strcpy(str, yytext);
            father->content = str;
        }
    }
    nodeList[nodeNum] = father;
    nodeNum++;
    // Preorder(father,0);
    return father;
}

// 父节点->左子节点->右子节点....
void Preorder(Ast ast, int level)
{
    if (!strcmp(ast->name, "Program"))
        printf("语法分析树打印：\n");
    int i;
    if (ast != NULL)
    {
        // 空节点不打印
        if (ast->line != -1)
        {
            // 层级结构缩进
            for (i = 0; i < level; ++i)
            {
                printf(" ");
            }
            // 打印节点类型
            printf("%s", ast->name);
            // 根据不同类型打印节点数据
            if ((!strcmp(ast->name, "ID")) || (!strcmp(ast->name, "TYPE")))
            {
                printf(": %s", ast->content);
            }
            else if (!strcmp(ast->name, "INT"))
            {
                printf(": %d", (int)(ast->value));
            }
            else if (!strcmp(ast->name, "FLOAT"))
            {
                printf(": %f", ast->value);
            }
            else
            {
                // 非叶节点打印行号
                printf("(%d)", ast->line);
            }
            printf("\n");
        }
        for (i = 0; i < ast->ncld; i++)
        {
            Preorder((ast->cld)[i], level + 1);
        }
    }
    else
    {
        return;
    }
    if (!strcmp(ast->name, "Program"))
        printf("打印完毕\n");
}

// 错误处理
void yyerror(char *msg)
{
    hasFault = 1;
    fprintf(stderr, "Error type B at Line %d: %s before %s\n", yylineno, msg, yytext);
}

// 设置节点打印状态 该节点为子节点
void setChildTag(tnode node)
{
    int i;
    for (i = 0; i < nodeNum; i++)
    {
        if (nodeList[i] == node)
        {
            nodeIsChild[i] = 1;
        }
    }
}

