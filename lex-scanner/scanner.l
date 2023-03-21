%{
#include "token.h"
int cur_line_num = 1;
int cur_col_num = 0;
void init_scanner();
void lex_error(char* msg, int line);
%}


INTEGER             ([0-9]+)
UNTERM_STRING       (\042[^\042\n]*)
STRING              (\042[^\042\n]*\042)
IDENTIFIER          ([_a-zA-Z][_a-zA-Z0-9]*)
OPERATOR            ([+*-/%=,;!<>(){}])
SINGLE_COMMENT1     ("//"[^\n]*)
SINGLE_COMMENT2     ("#"[^\n]*)

%%

[\n]                { cur_line_num++; cur_col_num = 0;      }
[ \t\r\a]+          { /* ignore all spaces */               }
{SINGLE_COMMENT1}   { /* skip for single line comment */    }
{SINGLE_COMMENT2}   { /* skip for single line commnet */    }

{OPERATOR}          { return yytext[0];         }   

"<="                { return T_LessEqual;       }
">="                { return T_GreaterEqual;    }
"=="                { return T_Equal;           }
"!="                { return T_NotEqual;        }
"&&"                { return T_And;             }
"||"                { return T_Or;              }
"void"              { return T_Void;            }
"int"               { return T_Int;             }
"while"             { return T_While;           }
"if"                { return T_If;              }
"else"              { return T_Else;            }
"return"            { return T_Return;          }
"break"             { return T_Break;           }
"continue"          { return T_Continue;        }
"print"             { return T_Print;           }
"readint"           { return T_ReadInt;         }

{INTEGER}           { return T_IntConstant;     }
{STRING}            { return T_StringConstant;  }
{IDENTIFIER}        { return T_Identifier;      }

<<EOF>>             { return 0; }

{UNTERM_STRING}     { lex_error("Unterminated string constant", cur_line_num);  }
.                   { lex_error("Unrecognized character", cur_line_num);        }

%%

int main(int argc, char* argv[]) {
    int token;
    init_scanner();
    int token_id = 0;
    while (token = yylex()) {
        //print_token(token);
        //puts(yytext);
        //example: @16,47:46='<EOF>',<EOF>,2:0
        printf("[@%d, %d:%d = '%s', <", token_id, cur_col_num, cur_col_num + yyleng, yytext);
        print_token(token);
        printf(">, %d:%d]\n", cur_line_num, cur_col_num);
        
        token_id ++;
        cur_col_num += yyleng;
    }
    return 0;
}

void init_scanner() {
    //printf("%-20s%s\n", "TOKEN-TYPE", "TOKEN-VALUE");
    printf("Token Streams");
    printf("-------------------------------------------------\n");
}

void lex_error(char* msg, int line) {
    printf("\nError at line %-3d: %s\n\n", line, msg);
}

int yywrap(void) {
    return 1;
}
