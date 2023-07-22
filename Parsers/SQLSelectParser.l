/* This file implements the SQL Parser which accespts all SQL queries 
    satisfying the below Grammar :
    
    <sql_query>         ::= <select_query> | <update_query> | <delete_query>

    <select_query>     ::=  SELECT <columns> FROM <tables> WHERE <conditions>

    <columns>            ::= <plain_columns> | <aggregate_colmns> | *

    <plain_columns>  ::= <column> | <column> , <plain_columns>

    <aggregate_columns>     ::= <agg_column> | <agg_column> , <aggregate_columns>

    <agg_column> ::= <function>(<column>)

    <tables> ::= <table> | <table> , <tables>

    <conditions> ::= <condition> | <condition> <lop> <conditions>

    <condition>  ::= <column> <mop> <digits> 

    <function>     ::= COUNT | SUM | AVG | MIN | MAX

     <column>      ::= identifier

     <table>          ::= identifier

     <lop>  ::= AND | OR | NOT | IN | BETWEEN | LIKE | EXISTS

     <digits> := <digit> | <digit><digits>

     <digit> ::= 0|1|2|3|4|5|6|7|8|9

     <mop>  ::= <   |   >   |   =   |   != 
*/

%{

#include <stdio.h>
#include "SQLParserStruct.h"
#define EOL 1000
#define BRACK_START 1001
#define BRACK_END 1002
#define COMMA 1003
#define INTEGER 1004
#define WILDCARD 1005

#define PARSER_ERROR_EXIT(code, expected_code) \
    if (code != expected_code) yyerror (code, expected_code, yytext, yylineno)

%}

%%

"select" {
    return SQL_SELECT_Q;
}

"max" {
    return SQL_MAX;
}
"sum" {
    return SQL_SUM;
}
"min" {
    return SQL_MIN;
}
"count" {
    return SQL_COUNT;
}
"avg" {
    return SQL_AVG;
}

"from" {
    return SQL_FROM;
}

"where" {
    return SQL_WHERE;
}

"(" {
    return BRACK_START;
}

")" {
    return BRACK_END;
}

"<" {
    return SQL_LESS_THAN;
}

">" {
    return SQL_GREATER_THAN;
}

"=" {
    return SQL_EQ;
}

"!=" {
    return SQL_NOT_EQ;
}

"and" {
    return SQL_AND;
}

"or" {
    return SQL_OR;
}

\n {
    return EOL;
}

"," {

    return COMMA;
}

[ ] {
    /* Ignore */
}

[\t] {
    /*ignore*/
}

[1-9][0-9]* {
    return INTEGER;
}

[a-zA-Z0-9]+ {
    return SQL_IDENTIFIER;
}

"*" {
    return WILDCARD;
}

. {
    printf ("Non parsable character : %c\n", *yytext);
    exit(0);
}

%%

static void 
yyerror (int token_code, int exp_token_code, char *str, int lineno) {

    printf ("[%d] : Parsing Failed [token obtained : %d, expected token %d token value : %s\n",    lineno, token_code, exp_token_code, str);
    exit(0);
}

static void 
parse_first_aggregated_column (int token_code) {

    printf ("Aggregated Colmn name is : %s", yytext);
    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, BRACK_START);
    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    printf ("(%s", yytext);
    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, BRACK_END);
    printf (")\n");
}

static int
parse_aggregated_columns() {

    int token_code;
    /* Parse list of AGG columns 
    <function_name>(colmn_name) , . . . 
    */
    while(1) {
        token_code = yylex();

        switch (token_code){
            case SQL_SUM:
            case SQL_MIN:
            case SQL_MAX:
            case SQL_COUNT:
            case SQL_AVG:
                printf ("Aggregated Colmn name is : %s", yytext);
                token_code = yylex();
                PARSER_ERROR_EXIT(token_code, BRACK_START);
                token_code = yylex();
                PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
                printf ("(%s", yytext);
                token_code = yylex();
                PARSER_ERROR_EXIT(token_code, BRACK_END);
                printf (")\n");
                token_code = yylex();
                if (token_code != COMMA) {
                    return token_code;
                }
            break;
            default:
                PARSER_ERROR_EXIT(token_code, 0);
        }
    }
    return token_code;
}

static void 
 parse_first_plain_column (int token_code) {

    printf ("Colmn name is : %s\n", yytext);
 }

static int
parse_plain_coumns() {

    int token_code;

    while (1) {

        token_code = yylex();
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
        printf ("Colmn name is : %s\n", yytext);
        token_code = yylex();
        if (token_code != COMMA) {
            break;
        }
    }
    return token_code;
}

static int
parse_columns () {

 int token_code = yylex();

    switch (token_code) {

        case SQL_SUM:
        case SQL_MIN:
        case SQL_MAX:
        case SQL_COUNT:
        case SQL_AVG:
            parse_first_aggregated_column (token_code);
            token_code = yylex();
            if (token_code == COMMA) {
                token_code = parse_aggregated_columns();
            }
            break;
        case SQL_IDENTIFIER:
            parse_first_plain_column (token_code);
            token_code = yylex();
            if (token_code == COMMA) {
                token_code = parse_plain_coumns();
            }
            break;
        case WILDCARD:
            printf ("Colmn name is : %s\n", yytext);
            token_code = yylex();
            break;
        default: ;
    }
    return token_code;
}


static int 
parse_where_condition() {

    char col_name [64];
    int op_code;
    int value;

    int token_code = yylex();
    PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    strcpy (col_name, yytext);

    token_code = yylex();

    if (token_code == SQL_LESS_THAN ||
         token_code == SQL_GREATER_THAN ||
         token_code == SQL_EQ ||
         token_code == SQL_NOT_EQ) {

        op_code = token_code;

        token_code = yylex();
        PARSER_ERROR_EXIT(token_code, INTEGER);
        value = atoi (yytext);

        printf ("Condition : %s %d %d\n", col_name, op_code, value);
    }

    else if (token_code == SQL_IN) {

        exit(0);
    }

    /* What follows after condition : Logical Operators */
    return yylex();
}

static int
parse_where_conditions() {

    int token_code;
    
    while(1){

        token_code = parse_where_condition();
        
        switch (token_code) {

            case SQL_AND:
            case SQL_OR:
                printf ("logical Op : %s\n", yytext);
            break;
            case EOL:
                return token_code;
        }
    }
    return token_code;
}

static int
parse_tables() {

    int token_code;

    while(1) {
        token_code = yylex();
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
        printf ("Table Name : %s\n", yytext);
        token_code = yylex();
        if (token_code == COMMA) {
            continue;
        }
        return token_code;
    }
}

static void 
parse_select_query () {

    int token_code;

  //token_code = yylex();
  //PARSER_ERROR_EXIT(token_code, SQL_SELECT_Q);
  
   token_code = parse_columns();

   switch (token_code) {
    case SQL_FROM:
        token_code = parse_tables();
        switch(token_code) {
            case SQL_WHERE:
                token_code = parse_where_conditions();
                 switch(token_code) {
                    case EOL:
                        return;
                 }
                break;
            case EOL:
                return;
        }
        break;
        default:
        PARSER_ERROR_EXIT(token_code, 0);
        break;
   }
}

static void 
parse_update_query () {


}

static void 
parse_sql_query () {

    int token_code = yylex();

    switch (token_code) {

        case SQL_SELECT_Q:
            //yyless(yyleng);
            parse_select_query  ();
            break;
        case SQL_UPDATE_Q:
           // yyless(yyleng);
            parse_update_query ();
            break;
        default:
            printf ("%s Query not supported\n", yytext);
            exit(0);
    }
}

int 
main (int argc, char **argv) {

    parse_sql_query();
    printf ("Parser finished successfully\n");
    return 0;
}