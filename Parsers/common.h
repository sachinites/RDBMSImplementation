
#ifndef __COMMON__
#define __COMMON__

#include <stdio.h>
#include <stdlib.h>

#define EOL 1000
#define BRACK_START 1001
#define BRACK_END 1002
#define COMMA 1003
#define SHOW_DB_TABLES  1004
#define QUOTATION_MARK 1005
#define  DECIMAL_NUMBER    1006
#define QUIT    1007
#define SQL_PARSE_ERROR   1008
#define SQL_PARSE_OK   1009
#define SEMI_COLON  1010
#define WHITE_SPACE 1011

#define PARSER_ERROR_EXIT(code, expected_code) \
    if (code != expected_code) {                                         \
        yyerror (code, expected_code, yytext, yylineno);     \
        return SQL_PARSE_ERROR; \
    }

static void 
yyerror (int token_code, int exp_token_code, char *str, int lineno) {

    printf ("[%d] : Parsing Failed [token obtained : %d, expected token %d token value : %s\n",    lineno, token_code, exp_token_code, str);
    //exit(0);
}

#define IS_TOKEN_OK(token_code_)    \
    if (token_code_ == SQL_PARSE_ERROR) return SQL_PARSE_ERROR

#endif
