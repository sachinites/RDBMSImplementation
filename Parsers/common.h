
#ifndef __COMMON__
#define __COMMON__

#include <stdio.h>
#include <stdlib.h>

#define EOL 1000
#define BRACK_START 1001
#define BRACK_END 1002
#define COMMA 1003
#define INTEGER 1004
#define WILDCARD 1005

#define PARSER_ERROR_EXIT(code, expected_code) \
    if (code != expected_code) yyerror (code, expected_code, yytext, yylineno)

static void 
yyerror (int token_code, int exp_token_code, char *str, int lineno) {

    printf ("[%d] : Parsing Failed [token obtained : %d, expected token %d token value : %s\n",    lineno, token_code, exp_token_code, str);
    exit(0);
}

#endif
