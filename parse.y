%{
#include<unistd.h>
#include<stdio.h>   
#include "syntax_tree.h"

int inWhile = 0; /* 如果 inWhile > 0 则在循环体中*/
%}

%union{
    tnode type_tnode;
	double d; // 这里声明double是为了防止出现指针错误-segmentation fault
}

/*声明记号*/
%token <type_tnode> INT STRING TYPE STRUCT RETURN PRINT IF ELSE WHILE BREAK ID COMMENT SPACE SEMI COMMA ASSIGNOP 
%token <type_tnode> PLUS MINUS MULTI DIV AND OR DOT NOT LP RP LB RB LC RC AERROR RELOP EOL GETINT

%type  <type_tnode> Program ExtDefList ExtDef ExtDecList Specifire StructSpecifire PrintStmt ReadStmt PActuals
%type  <type_tnode> OptTag Tag VarDec FunDec VarList ParamDec Compst StmtList Stmt DefList Def DecList Dec Exp Args _while

/*优先级*/
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left MULTI DIV
%right NOT 
%left LP RP LB RB DOT

%nonassoc LOWER_THAN_ELSE 
%nonassoc ELSE

/*产生式*/
/*$$表示左表达式 ${num}表示右边的第几个表达式*/
%%

Program:
ExtDefList {$$=newAst("Program",1,$1); }
;

ExtDefList:
ExtDef ExtDefList {$$=newAst("ExtDefList",2,$1,$2); }
| {$$=newAst("ExtDefList",0,-1); }
;

ExtDef:
Specifire ExtDecList SEMI {
	$$=newAst("ExtDef",3,$1,$2,$3);
	if(findVar($2)) // 变量重定义
		printf("Error TYPE-1 at Line %d: Redefined Variable: '%s'\n",yylineno,$2->content);
    else newVar(2,$1,$2); //新增变量表符号 
    }    
| Specifire SEMI	{$$=newAst("ExtDef",2,$1,$2); }
| Specifire FunDec Compst {
		$$=newAst("ExtDef",3,$1,$2,$3); 
		// 设置函数声明的返回值类型并检查返回类型错误
		newFunc(1,$1);
}
;

ExtDecList:
VarDec {$$=newAst("ExtDecList",1,$1); }
|VarDec COMMA ExtDecList {$$=newAst("ExtDecList",3,$1,$2,$3); }
;

Specifire:
TYPE {$$=newAst("Specifire",1,$1);}
|StructSpecifire {$$=newAst("Specifire",1,$1); }
;

StructSpecifire:
STRUCT OptTag LC DefList RC {
		// 结构体定义完成，当前在结构体定义外部
		inStruc = 0;
		$$=newAst("StructSpecifire",5,$1,$2,$3,$4,$5); 
		if(findstruc($2))	// 结构体的名字与前面定义过的结构体或变量的名字重复
			printf("Error TYPE-8 at Line %d: Redefined Struct Name: '%s'\n",yylineno,$2->content);
        else newstruc(1,$2); }
|STRUCT Tag {$$=newAst("StructSpecifire",2,$1,$2); }
;

OptTag:
ID {$$=newAst("OptTag",1,$1); }
|{$$=newAst("OptTag",0,-1); }
;

Tag:
ID {$$=newAst("Tag",1,$1); }
;

VarDec:
ID {$$=newAst("VarDec",1,$1); $$->tag=1; //变量
	}
|VarDec LB INT RB { //数组
	$$=newAst("VarDec",4,$1,$2,$3,$4); 
	$$->content=$1->content;$$->tag=4;}
;

FunDec:
ID LP VarList RP {
		$$=newAst("FunDec",4,$1,$2,$3,$4); $$->content = $1->content;
		if(findFunc($1)) //函数重定义
			printf("Error TYPE-6 at Line %d: Redefined Function: '%s'\n",yylineno,$1->content);
        else newFunc(2,$1,$3); // 设置函数名称以及参数列表 
        }
|ID LP RP {
		$$=newAst("FunDec",3,$1,$2,$3); $$->content=$1->content;
		if(findFunc($1)) //函数重定义
			printf("Error TYPE-6 at Line %d: Redefined Function: '%s'\n",yylineno,$1->content);
        else newFunc(2,$1,$3); 	// 设置函数名称以及参数列表 
        }
;

VarList:
ParamDec COMMA VarList {$$=newAst("VarList",3,$1,$2,$3); }
|ParamDec {$$=newAst("VarList",1,$1); }
;

ParamDec:
Specifire VarDec {
		$$=newAst("ParamDec",2,$1,$2); 
		if(findVar($2)||findArray($2))  // 变量重定义
			printf("Error TYPE-1 at Line %d: Redefined Variable: '%s'\n",yylineno,$2->content);
        else if($2->tag==4) //创建数组符号表
			newArray(2,$1,$2);
        else // 创建变量符号表
			newVar(2,$1,$2); }
;

Compst:
LC DefList StmtList RC {$$=newAst("Compst",4,$1,$2,$3,$4); }
;

StmtList:
Stmt StmtList{$$=newAst("StmtList",2,$1,$2); }
| {$$=newAst("StmtList",0,-1); }
;

Stmt:
Exp SEMI {$$=newAst("Stmt",2,$1,$2); }
|Compst {$$=newAst("Stmt",1,$1); }
|RETURN Exp SEMI {
		$$=newAst("Stmt",3,$1,$2,$3); 
		getRType($2);}
|IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {$$=newAst("Stmt",5,$1,$2,$3,$4,$5); }
|IF LP Exp RP Stmt ELSE Stmt {$$=newAst("Stmt",7,$1,$2,$3,$4,$5,$6,$7); }
|_while LP Exp RP Stmt { inWhile = inWhile - 1;
						$$=newAst("Stmt",5,$1,$2,$3,$4,$5); }
|PrintStmt {$$=newAst("Stmt",1,$1); }
;

PrintStmt:
PRINT LP STRING PActuals RP SEMI {$$=newAst("PrintStmt",6,$1,$2,$3,$4,$5,$6); }
;

PActuals:
PActuals COMMA Exp {$$=newAst("PActuals",3,$1,$2,$3);}
| {$$=newAst("PActuals",0,-1); }
;

_while:
WHILE {inWhile = inWhile + 1; 
	$$=newAst("_while",1,$1);}
;

/*Local Definitions*/
DefList:
Def DefList{$$=newAst("DefList",2,$1,$2); }
| {$$=newAst("DefList",0,-1); }
;

Def:
Specifire DecList SEMI {
		$$=newAst("Def",3,$1,$2,$3); 
		// 变量出现重复定义
		if(findVar($2)||findArray($2))  
			printf("Error TYPE-1 at Line %d: Redefined Variable: '%s'\n",yylineno,$2->content);
        else if($2->tag==4) 
			newArray(2,$1,$2);
        else 
			newVar(2,$1,$2);}
;

DecList:
Dec {$$=newAst("DecList",1,$1); }
|Dec COMMA DecList {$$=newAst("DecList",3,$1,$2,$3); $$->tag=$3->tag;}
;

Dec:
VarDec {$$=newAst("Dec",1,$1); }
|VarDec ASSIGNOP Exp {$$=newAst("Dec",3,$1,$2,$3); $$->content=$1->content;}
;

/*Expressions*/
Exp:
Exp ASSIGNOP Exp {
		$$=newAst("Exp",3,$1,$2,$3); 
		if($1->type==NULL || $3->type==NULL){ // 当有一边变量是未定义时，不进行处理
			return;
		}
		if(strcmp($1->type,$3->type)) // 赋值号两边的表达式类型不匹配
			printf("Error TYPE-4 at Line %d: Type mismatched for assignment.\n ",yylineno);
		if(!checkLeft($1)) // 赋值号左边出现一个只有右值的表达式
			printf("Error TYPE-3 at Line %d: Left-end of the assignment must be a variable.\n ",yylineno); }
|Exp AND Exp {$$=newAst("Exp",3,$1,$2,$3); }
|Exp OR Exp {$$=newAst("Exp",3,$1,$2,$3); }
|Exp RELOP Exp {$$=newAst("Exp",3,$1,$2,$3); }
|Exp PLUS Exp {
		$$=newAst("Exp",3,$1,$2,$3); 
		// 操作数类型不匹配
		if(strcmp($1->type,$3->type))
			printf("Error TYPE-5 at Line %d: Type mismatched for operands.\n ",yylineno);}
|Exp MINUS Exp {
		$$=newAst("Exp",3,$1,$2,$3); 
		// 操作数类型不匹配
		if(strcmp($1->type,$3->type))
			printf("Error TYPE-5 at Line %d: Type mismatched for operands.\n ",yylineno);}
|Exp MULTI Exp {
		$$=newAst("Exp",3,$1,$2,$3); 
		// 操作数类型不匹配
		if(strcmp($1->type,$3->type))
			printf("Error TYPE-5 at Line %d: Type mismatched for operands.\n ",yylineno);}
|Exp DIV Exp {
		$$=newAst("Exp",3,$1,$2,$3); 
		// 操作数类型不匹配
		if(strcmp($1->type,$3->type))
			printf("Error TYPE-5 at Line %d: Type mismatched for operands.\n ",yylineno);}
|LP Exp RP {$$=newAst("Exp",3,$1,$2,$3); }
|MINUS Exp {$$=newAst("Exp",2,$1,$2); }
|NOT Exp {$$=newAst("Exp",2,$1,$2); }
|ID LP Args RP {
		$$=newAst("Exp",4,$1,$2,$3,$4); 
		if(!findFunc($1) && (findVar($1)||findArray($1))) // 对普通变量使用“(...)”或“()”（函数调用）操作符
			printf("Error TYPE-9 at Line %d: '%s' is not a function.\n ",yylineno,$1->content);
		else if(!findFunc($1)) // 函数在调用时未经定义
			printf("Error TYPE-7 at Line %d: Undefined Function: %s\n ",yylineno,$1->content);
		else if(checkrtype($1,$3))// 函数实参和形参类型不一致
			printf("Error TYPE-10 at Line %d: Function parameter type error.\n ",yylineno);
		else{ /*empty*/ }}
|ID LP RP {
		$$=newAst("Exp",3,$1,$2,$3); 
		if(!findFunc($1) && (findVar($1)||findArray($1))) // 对普通变量使用“(...)”或“()”（函数调用）操作符
			printf("Error TYPE-9 at Line %d: '%s' is not a function.\n ",yylineno,$1->content);
		else if(!findFunc($1)) 	// 函数在调用时未经定义
			printf("Error TYPE-7 at Line %d: Undefined Function: %s\n ",yylineno,$1->content);
		else {}
	}
|Exp LB Exp RB {$$=newAst("Exp",4,$1,$2,$3,$4); }
|Exp DOT ID {$$=newAst("Exp",3,$1,$2,$3); }
|ID {
		$$=newAst("Exp",1,$1); 
		if(!findVar($1)&&!findArray($1)) // 变量在使用时未经定义
			printf("Error TYPE-2 at Line %d: Undefined Variable or Array: %s\n",yylineno,$1->content);
		else 
			$$->type=typevar($1);}
|INT {$$=newAst("Exp",1,$1); $$->tag=3;$$->type="int";}
|BREAK {
	$$=newAst("Exp",1,$1);
	if (inWhile <= 0)
		printf("Error TYPE-11 at Line %d: Break Not In Loop.\n",yylineno);}
|ReadStmt {$$=newAst("Stmt",1,$1); }
;

ReadStmt:
GETINT LP STRING RP SEMI {$$=newAst("ReadStmt",5,$1,$2,$3,$4,$5);}
;

Args:
Exp COMMA Args {$$=newAst("Args",3,$1,$2,$3);}
|Exp {$$=newAst("Args",1,$1);}
;

%%


