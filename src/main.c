#include "AST.h"
#include "semantics.h"
#include "ir.h"
#include "mips.h"

#define OUTPUT_OPTION 1  // 0->ir, 1->mips

extern FILE *in;
FILE *out;
extern AST_Node *root;
extern int error_flag;
int out_is_file;

int main(int argc, char **argv) {
    if (argc > 1){
        if (!(in = fopen(argv[1], "r"))){
            perror(argv[1]);
            return 1;
        }
        yyrestart(in);
    }
    if (argc > 2){
        if (!(out = fopen(argv[2], "w"))) {
            perror(argv[2]);
            return 1;
        }
        out_is_file = 1;
    } else {
        out_is_file = 0;
        out = stdout;
    }
    
    root = malloc(sizeof(AST_Node));
    yyparse();
    // if no lexical / parser error
    if (error_flag == 0){
        // print_AST(root, 0);
        semantics_analysis(root);
        // if semantics error exists
        if (error_flag){
            print_error_list();
        }
        // if no semantics error
        else {
            ir_generate(root);
            if (OUTPUT_OPTION == 0) {
                ir_to_file(out);
            }
            else {
                mips_generate();
                mips_to_file(out);
            }
        }
    }
    if (out_is_file){
        fclose(out);
    }
    
    return 0;
}
