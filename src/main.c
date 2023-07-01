#include "AST.h"
#include "semantics.h"
#include "ir.h"
#include "mips.h"

#define OUTPUT_OPTION 0

extern FILE *in;
FILE *out;
extern ASTNode *root;
extern int error_flag;

int main(int argc, char **argv) {
    if (argc > 1){
	      if (!(in = fopen(argv[1], "r"))){
	          perror(argv[1]);
	          return 1;
	      }
	      yyrestart(in);
    }
	if (!(out = fopen(argv[2], "w"))) {
		perror(argv[2]);
		return 1;
	}
    root = malloc(sizeof(ASTNode));
	yyparse();
    if (error_flag == 0){
	ir_generate(root);
	mips_generate();
	mips_to_file(out);
    }
		
	fclose(out);
	return 0;
}


