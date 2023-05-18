#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

extern int yylinenum;
extern char* yytext;
void yyeror(char* msg);

typedef struct treeNode{
	int line; // line number
	char *name; // type of token
	struct treeNode *fchild, *next; //first child and brother

	union {
		char* id_type;
		int intval;
		float fltval;
	};
} *Ast, *tnode;

Ast newAst(char *name, int num, ...);

void preOrderAst(Ast ast, int leval);

int nodeNumCount;

tnode nodeList[5000];
int nodeIsChild[5000];

void setChildTag(tnode node);

int hasFault;
