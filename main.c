#include "AST.h"
#include "semantics.h"

extern FILE* in;
extern ASTNode* root;
extern int error_flag;

int main(int argc, char **argv){
    if (argc > 1) {
        if (!(in = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return 1;
        }
        yyrestart(in);
    }

    root = malloc(sizeof(ASTNode));
    yyparse();
    if (!error_flag) { // no lexical or syntax error
        print_AST(root, 0);
        semantics_analysis(root);
        if (error_flag) { // semantic error occurs
            print_error_list();
        }
        return 0;
    }
}