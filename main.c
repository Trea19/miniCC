#include "compiler.h"

// 主函数 扫描文件并且分析
// 为bison会自己调用yylex()，所以在main函数中不需要再调用它了
// bison使用yyparse()进行语法分析，所以需要我们在main函数中调用yyparse()和yyrestart()
int main(int argc, char **argv)
{
    int j, tem;
    if (argc < 2)
    {
        return 1;
    }
    for (int i = 1; i < argc; i++)
    {
        // 初始化，用于记录结构体域
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
        memset(nodeList, 0, sizeof(tnode) * 5000);
        memset(nodeIsChild, 0, sizeof(int) * 5000);
        hasFault = 0;

        // 初始化临时变量、标签数组
        init_tempvar_lable();

        // 添加read、write函数到符号表
        char *read = "read";
        char *write = "write";
        char *typeInt = "int";
        // read函数
        functail->tag = 1;
        functail->name = read;
        functail->type = typeInt;
        functail->rtype = typeInt;
        functail->va_num = 0;
        functail->inStruc = 0;
        functail->strucNum = 0;
        // 新建节点
        func *new = (func *)malloc(sizeof(func));
        functail->next = new; //尾指针指向下一个空结点
        functail = new;
        // write函数
        functail->tag = 1;
        functail->name = write;
        functail->va_num = 1;
        (functail->va_type)[0] = typeInt;
        functail->inStruc = 0;
        functail->strucNum = 0;
        // 新建节点
        new = (func *)malloc(sizeof(func));
        functail->next = new; //尾指针指向下一个空结点
        functail = new;

        // 设置已经分配的寄存器数量
        reg_num = 0;

        FILE *f = fopen(argv[i], "r");
        if (!f)
        {
            perror(argv[i]);
            return 1;
        }
        yyrestart(f);
        yyparse();
        fclose(f);

        // 遍历所有非子节点的节点
        if (hasFault)
            continue;
        for (j = 0; j < nodeNum; j++)
        {
            if (nodeIsChild[j] != 1)
            {
                Preorder(nodeList[j], 0);
                InterCode codes = translate_Program(nodeList[j]);
                print_Codes(codes);
                //generate_MIPS_Codes(codes);
            }
        }
    }
}
