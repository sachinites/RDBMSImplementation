/* This file implements the SQL Parser which accespts all SQL queries satisfying the below Grammar 

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
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include "common.h"
#include "Ast.h"

%}

%%

"select" {
    return SQL_SELECT_Q;
}
"create table" {
    return SQL_CREATE_Q;
}
"insert into" {
    return SQL_INSERT_Q;
}
"varchar" {
    return SQL_STRING;
}
"int" {
    return SQL_INT;
}
"float" {
    return SQL_FLOAT;
}
"ipv4" {
    return SQL_IPV4_ADDR;
}
"primary key" {
    return SQL_PRIMARY_KEY;
}
"not null" {
    return SQL_NOT_NULL;
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

"'" {
    return QUOTATION_MARK;
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

"\\dt\n" {
    return SHOW_DB_TABLES;
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

"\\q" {
    return QUIT;
}

(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9]) {

    return SQL_IPV4_ADDR_VALUE;
}

[1-9][0-9]* {
    return INTEGER;
}

^[a-zA-Z]+(?:[\s.]+[a-zA-Z]+)*$ {
    return SQL_STRING_VALUE;
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
parse_first_aggregated_column (int token_code) {

    //printf ("Aggregated Colmn name is : %s", yytext);
    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, BRACK_START);
    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    //printf ("(%s", yytext);
    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, BRACK_END);
    //printf (")\n");
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
               // printf ("Aggregated Colmn name is : %s", yytext);
                token_code = yylex();
                PARSER_ERROR_EXIT(token_code, BRACK_START);
                token_code = yylex();
                PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
               // printf ("(%s", yytext);
                token_code = yylex();
                PARSER_ERROR_EXIT(token_code, BRACK_END);
               // printf (")\n");
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
 parse_first_plain_column (int token_code, ast_node_t *parent) {

    //printf ("Colmn name is : %s\n", yytext);
    ast_node_t *col_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    col_node->entity_type = SQL_IDENTIFIER;
    col_node->u.identifier.ident_type = SQL_COLUMN_NAME;
    strncpy (col_node->u.identifier.identifier.name, yytext, 
        sizeof (col_node->u.identifier.identifier.name));
    ast_add_child (parent, col_node);
 }

static int
parse_plain_coumns(ast_node_t *parent) {

    int token_code;

    while (1) {

        token_code = yylex();
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
       // printf ("Colmn name is : %s\n", yytext);
       ast_node_t *astnode = (ast_node_t *)calloc (1,sizeof (ast_node_t));
       astnode->entity_type = SQL_IDENTIFIER;
       astnode->u.identifier.ident_type = SQL_COLUMN_NAME;
        strncpy (astnode->u.identifier.identifier.name, yytext, 
        sizeof (astnode->u.identifier.identifier.name));
        ast_add_child (parent, astnode);
        token_code = yylex();
        if (token_code != COMMA) {
            break;
        }
    }
    return token_code;
}

static int
parse_columns (ast_node_t *parent) {

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
            parse_first_plain_column (token_code, parent);
            token_code = yylex();
            if (token_code == COMMA) {
                token_code = parse_plain_coumns(parent);
            }
            break;
        case WILDCARD:
           // printf ("Colmn name is : %s\n", yytext);
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

        //printf ("Condition : %s %d %d\n", col_name, op_code, value);
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
                //printf ("logical Op : %s\n", yytext);
            break;
            case EOL:
                return token_code;
        }
    }
    return token_code;
}

static int
parse_tables(ast_node_t *select_kw) {

    int token_code;

    while(1) {

        token_code = yylex();
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
        //printf ("Table Name : %s\n", yytext);
        ast_node_t *tble_name_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
        tble_name_node->entity_type = SQL_IDENTIFIER;
        tble_name_node->u.identifier.ident_type = SQL_TABLE_NAME;
        strncpy(tble_name_node->u.identifier.identifier.name, yytext, sizeof (tble_name_node->u.identifier.identifier.name));
        ast_add_child (select_kw, tble_name_node);
        token_code = yylex();
        if (token_code == COMMA) {
            continue;
        }
        return token_code;
    }
}

static void 
parse_select_query (ast_node_t *root) {

    int token_code;
    ast_node_t parent_temp;
  //token_code = yylex();
  //PARSER_ERROR_EXIT(token_code, SQL_SELECT_Q);
  
   memset (&parent_temp, 0, sizeof (parent_temp));
   token_code = parse_columns(&parent_temp);

   switch (token_code) {
    case SQL_FROM:
        token_code = parse_tables(root);
        root->child_list->child_list = parent_temp.child_list;
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

static  void  
parse_update_query ()  {}

#include "SQLCreateParser.c"
#include "SQLInsertIntoParser.c"
#include "../core/sql_intf.h"

static void 
parse_sql_query () {

    ast_node_t *astroot = NULL;

    printf ("postgres=# ");

    int token_code = yylex();

    switch (token_code) {

        case SQL_SELECT_Q:
            //yyless(yyleng);
            astroot = (ast_node_t *)calloc (1, sizeof (ast_node_t));
            astroot->entity_type = SQL_QUERY_TYPE;
            astroot->u.q_type = SQL_SELECT_Q;
            parse_select_query  (astroot);
            sql_process_select_query (NULL, astroot);
            break;
        case SQL_UPDATE_Q:
           // yyless(yyleng);
            //parse_update_query ();
            break;
        case SQL_CREATE_Q:
            astroot = (ast_node_t *)calloc (1, sizeof (ast_node_t));
            astroot->entity_type = SQL_QUERY_TYPE;
            astroot->u.q_type = SQL_CREATE_Q;
            parse_create_query(astroot);
            //ast_print (astroot, 0);
            sql_process_create_query(NULL, astroot);
            break;
        case SHOW_DB_TABLES:
            sql_show_table_catalog (NULL);
            break;
        case SQL_INSERT_Q:
             astroot = (ast_node_t *)calloc (1, sizeof (ast_node_t));
             astroot->entity_type = SQL_QUERY_TYPE;
             astroot->u.q_type = SQL_INSERT_Q;
             parse_insert_query (astroot);
             //ast_print (astroot, 0);
             sql_process_insert_query(NULL, astroot);
             break;
        case EOL:
            break;
        case QUIT:
            exit(0);
        default:
            printf ("%s : Unsupported Command\n", yytext);
            yylex(); // consume \n
            break;
    }
}

int 
main (int argc, char **argv) {

    while(1) {
        parse_sql_query();
    }
    return 0;
}