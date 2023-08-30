/* This file implements the SQL Parser which accespts all SQL queries satisfying the below Grammar 

    <sql_query>         ::= <select_query> | <update_query> | <delete_query>

    <select_query>     ::=  SELECT <columns> FROM <tables> WHERE <conditions>

    <columns>            ::= <plain_columns> | <aggregate_colmns> | *

    <plain_columns>  ::= <column> | <column> , <plain_columns>

    <aggregate_columns>     ::= <agg_column> | <agg_column> , <aggregate_columns>

    <agg_column> ::= <function>(<column>)

    <tables> ::= <table> | <table> , <tables>

    <conditions> ::= <condition> | <condition> <lop> <conditions>

    <condition>  ::= <column> <mop> <digits> |  <column> <mop> <column> | <column> <mop> 'identifer'

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
#include <time.h>
#include <assert.h>
#include <arpa/inet.h>
#include "common.h"
#include "Ast.h"
#include "../core/sql_const.h"
#include "../core/sql_where.h"
#include "../core/Catalog.h"
#include "../core/sql_utils.h"

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
"delete" {
    return SQL_DELETE_Q;
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

"\\\n" {

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

%{
/*
[0-9]*.[0-9]+ {
    return DECIMAL_NUMBER;
}
*/
%}

[a-zA-Z0-9]+ {
    return SQL_IDENTIFIER;
}

[a-zA-Z0-9]+\.[a-zA-Z0-9]+ {

    return SQL_TABLE_COLMN_NAME;
}

^[a-zA-Z]+(?:[\s.]+[a-zA-Z]+)*$ {
    return SQL_STRING_VALUE;
}

"*" {
    return WILDCARD;
}

. {
    printf ("Non parsable character : %c\n", *yytext);
    exit(0);
}

%%

static int paranthesis_cnt = 0;

static int
parse_first_aggregated_column (int token_code, ast_node_t *parent) {

    //printf ("Aggregated Colmn name is : %s", yytext);
    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, BRACK_START);
    token_code = yylex();
    if (token_code != SQL_IDENTIFIER &&
         token_code != SQL_TABLE_COLMN_NAME) {
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    }
    //printf ("(%s", yytext);
    ast_node_t *column_name_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    column_name_node->entity_type =  SQL_IDENTIFIER;
    column_name_node->u.identifier.ident_type = SQL_COLUMN_NAME;
    strcpy (column_name_node->u.identifier.identifier.name, yytext);
    ast_add_child (parent, column_name_node);
    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, BRACK_END);
    //printf (")\n");
    return SQL_PARSE_OK;
}

static int
parse_aggregated_columns(ast_node_t *parent) {

    int token_code;
    ast_node_t *agg_fn_node ;
    ast_node_t *column_name_node;

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
                agg_fn_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                agg_fn_node->entity_type =  SQL_AGG_FN;
                agg_fn_node->u.agg_fn = token_code;
                token_code = yylex();
                PARSER_ERROR_EXIT(token_code, BRACK_START);
                token_code = yylex();
                 if (token_code != SQL_IDENTIFIER &&
                     token_code != SQL_TABLE_COLMN_NAME) {
                    PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
                }
               // printf ("(%s", yytext);
               column_name_node = (ast_node_t *)calloc (1,sizeof (ast_node_t));
               column_name_node->entity_type = SQL_IDENTIFIER;
               column_name_node->u.identifier.ident_type = SQL_COLUMN_NAME;
               strncpy (column_name_node->u.identifier.identifier.name, yytext, 
                    sizeof column_name_node->u.identifier.identifier.name);
                ast_add_child (column_name_node , agg_fn_node);
                ast_add_child (parent, column_name_node);
                token_code = yylex();
                PARSER_ERROR_EXIT(token_code, BRACK_END);
               // printf (")\n");
                token_code = yylex();
                if (token_code != COMMA) {
                    return token_code;
                }
            break;
            default:
                break;
        }
    }
    return token_code;
}

static int
 parse_first_plain_column (int token_code, ast_node_t *parent) {

    //printf ("Colmn name is : %s\n", yytext);
    ast_node_t *col_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    col_node->entity_type = SQL_IDENTIFIER;
    col_node->u.identifier.ident_type = SQL_COLUMN_NAME;
    strncpy (col_node->u.identifier.identifier.name, yytext, 
        sizeof (col_node->u.identifier.identifier.name));
    ast_add_child (parent, col_node);
    return SQL_PARSE_OK;
 }

static int
parse_plain_coumns(ast_node_t *parent) {

    int token_code;

    while (1) {

        token_code = yylex();

        if (token_code != SQL_IDENTIFIER &&
            token_code != SQL_TABLE_COLMN_NAME) {
        
            PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
        }

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
            parse_first_aggregated_column (token_code, parent);
            ast_node_t *column_name_node = parent->child_list;
            ast_node_t *agg_fn_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
            agg_fn_node->entity_type =  SQL_AGG_FN;
            agg_fn_node->u.agg_fn = token_code;
            ast_add_child (column_name_node, agg_fn_node);
            token_code = yylex();
            if (token_code == COMMA) {
                token_code = parse_aggregated_columns(parent);
            }
            break;
        case SQL_IDENTIFIER:
        case SQL_TABLE_COLMN_NAME:
            parse_first_plain_column (token_code, parent);
            token_code = yylex();
            if (token_code == COMMA) {
                token_code = parse_plain_coumns(parent);
            }
            break;
        case WILDCARD:
        {
            ast_node_t *astnode = (ast_node_t *)calloc (1,sizeof (ast_node_t));
            astnode->entity_type = SQL_IDENTIFIER;
            astnode->u.identifier.ident_type = SQL_COLUMN_NAME;
            strncpy (astnode->u.identifier.identifier.name, yytext, 
            sizeof (astnode->u.identifier.identifier.name));
            ast_add_child (parent, astnode);
           // printf ("Colmn name is : %s\n", yytext);
            token_code = yylex();
            break;
        }
        default: ;
    }
    return token_code;
}


static int 
parse_where_condition(ast_node_t *where_kw, where_literal_t **wlit) {

    int value;
    int op_code;
    where_cond_t *wc;
    ast_node_t ast_tmplate;

    ast_node_t *root = where_kw->parent;

    char in_col_name [SQL_COMPOSITE_COLUMN_NAME_SIZE] = {0};
    char operand_col_name [SQL_COMPOSITE_COLUMN_NAME_SIZE] = {0};
    char operand_identifer [SQL_OPERAND_MAX_VALUE] = {0};

    int token_code = yylex();

    while (token_code == BRACK_START) {
        paranthesis_cnt++;
        (*wlit)->where_token_type = WHERE_LITERAL_OPERATOR_TOKEN_CODE;
        (*wlit)->u.token_id = BRACK_START;
        (*wlit)++;
        token_code = yylex();
    }

    (*wlit)->where_token_type = WHERE_LITERAL_WHERE_COND;
    wc = &(*wlit)->u.wc;

    if (token_code != SQL_IDENTIFIER &&
        token_code != SQL_TABLE_COLMN_NAME) {
        
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    }

    strcpy (in_col_name, yytext);

    char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE] = {0};
    char table_name [SQL_TABLE_NAME_MAX_SIZE] = {0};
    char *table_name_ptr = table_name;

    parser_split_table_column_name (in_col_name, table_name, lone_col_name);

    if (table_name[0] == '\0') {
        /* Pick the first table from the FROM list */
        ast_tmplate.entity_type = SQL_KEYWORD;
        ast_tmplate.u.kw = SQL_FROM;
        ast_node_t *from_kw = ast_find (root, &ast_tmplate);
        assert(from_kw);
        table_name_ptr = from_kw->child_list->u.identifier.identifier.name;
    }

    if (!Catalog_get_column (NULL, table_name_ptr, lone_col_name, &wc->col) ) {
        printf ("Error : Unrecognized Column <%s::%s>\n", 
            table_name_ptr, in_col_name);
        return SQL_PARSE_ERROR;
    }

    token_code = yylex();

    /* supported operators */
    op_code = token_code;

    switch(token_code) {

        case SQL_LESS_THAN:
        case SQL_GREATER_THAN:
        case SQL_EQ:
        case SQL_NOT_EQ:
        wc->op = token_code;
        break;
        default:
            printf ("%d operator not supported\n", token_code);
            return SQL_PARSE_ERROR;
    }

    //printf ("%s op(%d) ", col_name, token_code);

    token_code = yylex();

    switch (token_code) {

        case INTEGER:
            value = atoi (yytext);
            wc->right_op.w_opd = WH_VALUE;
            wc->right_op.u.value.dtype = SQL_INT;
            wc->right_op.u.value.size = sql_dtype_size (wc->right_op.u.value.dtype);
            wc->right_op.u.value.val = calloc (wc->right_op.u.value.size, 1);
            *(int *)(wc->right_op.u.value.val) = value;
            break;
        case SQL_IDENTIFIER:
        case SQL_STRING_VALUE:
        case SQL_TABLE_COLMN_NAME:
            strcpy (operand_col_name, yytext);
            memset ( table_name, 0, sizeof (table_name));
            table_name_ptr = table_name;
            memset (lone_col_name, 0, sizeof (lone_col_name));
            parser_split_table_column_name (operand_col_name, table_name, lone_col_name);
            if (table_name[0] == '\0') {
                /* Pick the first table from the FROM list */
                ast_tmplate.entity_type = SQL_KEYWORD;
                ast_tmplate.u.kw = SQL_FROM;
                ast_node_t *from_kw = ast_find (root, &ast_tmplate);
                assert(from_kw);
                table_name_ptr = from_kw->child_list->u.identifier.identifier.name;
            }
            wc->right_op.w_opd = WH_COL;
            if (!Catalog_get_column (NULL, 
                                table_name_ptr, 
                                lone_col_name, &wc->right_op.u.col)) {
                printf ("Error : Unrecognized Column <%s::%s>\n", 
                    table_name_ptr, lone_col_name);
               return SQL_PARSE_ERROR;
            }
            break;
        case QUOTATION_MARK:
            token_code = yylex();
            //PARSER_ERROR_EXIT(token_code, SQL_STRING_VALUE);
            PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
            strcpy (operand_identifer, yytext);
            //printf ("\'%s\'\n", operand_identifer);
            wc->right_op.w_opd = WH_VALUE;
            wc->right_op.u.value.dtype = SQL_STRING;
            wc->right_op.u.value.size = strlen (yytext);
            wc->right_op.u.value.val = calloc (wc->right_op.u.value.size, 1);
            strcpy (wc->right_op.u.value.val, yytext);
            token_code = yylex();
            PARSER_ERROR_EXIT(token_code, QUOTATION_MARK);
            break;
        case SQL_IPV4_ADDR_VALUE:
            wc->right_op.w_opd = WH_VALUE;
            wc->right_op.u.value.dtype = SQL_IPV4_ADDR;
            wc->right_op.u.value.size = sql_dtype_size (wc->right_op.u.value.dtype);
            //wc->right_op.u.value.size = 16;
            wc->right_op.u.value.val = calloc (wc->right_op.u.value.size, 1);
            inet_pton (AF_INET, (const char *)yytext, wc->right_op.u.value.val);
            //sql_debug_print_where_cond (wc);
            //strcpy (wc->right_op.u.value.val , yytext);
            break;
        default:
            PARSER_ERROR_EXIT(token_code, 0);
    }

    /* What follows after condition : Logical Operators */
    token_code =  yylex();
    
    while (token_code == BRACK_END) {
        (*wlit)++;
        (*wlit)->where_token_type = WHERE_LITERAL_OPERATOR_TOKEN_CODE;
        (*wlit)->u.token_id = BRACK_END;
        paranthesis_cnt--;
        token_code = yylex();
    }

    return token_code;
}

static int
parse_where_conditions(ast_node_t *where_kw) {

    int token_code;
    where_literal_t *wlit;
    
    where_literal_t *where_literals_arr = 
        (where_literal_t *) calloc ( SQL_MAX_WHERE_LITERAL_ARRAY_SIZE, sizeof (where_literal_t));
    
    ast_node_t *where_literals_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    where_literals_node->entity_type =  SQL_IDENTIFIER;
    where_literals_node->u.identifier.ident_type = SQL_PTR;
    memcpy ((void *)where_literals_node->u.identifier.identifier.name, (void *)&where_literals_arr, sizeof (void *));
    ast_add_child (where_kw, where_literals_node);

    wlit = &where_literals_arr[0];

    while(1) {

        token_code = parse_where_condition(where_kw, &wlit);

        switch (token_code) {

            case SQL_AND:
            case SQL_OR:
            case SQL_NOT:
               /// printf ("logical Op : %s\n", yytext);
                wlit++;
                wlit->where_token_type = WHERE_LITERAL_OPERATOR_TOKEN_CODE;
                wlit->u.token_id = token_code;
            break;
            case EOL:
                wlit++;
                wlit->where_token_type = WHERE_LITERAL_OPERATOR_TOKEN_CODE;
                wlit->u.token_id = token_code;
                //sql_debug_print_where_literals (where_literals_arr);
                return token_code;
        }
        wlit++;
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

static int
parse_select_query (ast_node_t *root) {

    int token_code;
    ast_node_t *curr, *select_kw;
    ast_node_t *from_kw, *where_kw;

    select_kw = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    select_kw->entity_type = SQL_KEYWORD;
    select_kw->u.kw = SQL_SELECT;
    ast_add_child (root, select_kw);
   
   token_code = parse_columns(select_kw /* selet keyword */);

   switch (token_code) {

    case SQL_FROM:

        from_kw = (ast_node_t *)calloc (1, sizeof (ast_node_t));
        from_kw->entity_type = SQL_KEYWORD;
        from_kw->u.kw = SQL_FROM;
        ast_add_child (root, from_kw);

        token_code = parse_tables(from_kw /* FROM keyword*/);

        if (select_kw->child_list->next == NULL &&
                strcmp (select_kw->child_list->u.identifier.identifier.name, "*") == 0) {
            /* If user typed select * .... */
            ast_node_t *star_node = select_kw->child_list;
            select_kw->child_list = NULL;
            star_node->parent = NULL;
            ast_destroy_tree (star_node);
            for (curr = from_kw->child_list; curr; curr = curr->next) {
                if (!sql_process_select_wildcard  (NULL, select_kw, curr)) {
                    return SQL_PARSE_ERROR;
                }
            }
        }

        switch(token_code) {

            case SQL_WHERE:

                where_kw = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                where_kw->entity_type = SQL_KEYWORD;
                where_kw->u.kw = SQL_WHERE;
                ast_add_child (root, where_kw);

                token_code = parse_where_conditions(where_kw);
                 switch(token_code) {
                    case EOL:
                        return SQL_PARSE_OK;
                 }
                break;
            case EOL:
                return SQL_PARSE_OK;
        }
        break;
    case EOL:
        return SQL_PARSE_OK;
    break;
        default:
        PARSER_ERROR_EXIT(token_code, 0);
        break;
   }
   return SQL_PARSE_OK;
}

static  void  
parse_update_query ()  {}

#include "SQLCreateParser.c"
#include "SQLInsertIntoParser.c"
#include "SQLDeleteParser.c"
#include "../core/sql_intf.h"

static void 
parse_sql_query () {

    bool rc;
    int err_code;
    ast_node_t *astroot = NULL;
    paranthesis_cnt = 0;

    printf ("postgres=# ");
    clock_t t = clock();

    int token_code = yylex();

    switch (token_code) {

        case SQL_SELECT_Q:
            //yyless(yyleng);
            astroot = (ast_node_t *)calloc (1, sizeof (ast_node_t));
            astroot->entity_type = SQL_QUERY_TYPE;
            astroot->u.q_type = SQL_SELECT_Q;
            err_code = parse_select_query  (astroot);
            //ast_print (astroot, 0);
            if (paranthesis_cnt) {
                printf ("Error : Unbalanced Paranthesis\n");
                 ast_destroy_tree_from_root (astroot);
                return;
            }
            if (err_code == SQL_PARSE_ERROR) {
                ast_destroy_tree_from_root (astroot);
                return;
            }
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
        case SQL_DELETE_Q:
            astroot = (ast_node_t *)calloc (1, sizeof (ast_node_t));
             astroot->entity_type = SQL_QUERY_TYPE;
             astroot->u.q_type = SQL_DELETE_Q;
             if (parse_delete_query (astroot) == SQL_PARSE_OK) {
                sql_process_delete_query(NULL, astroot);
             }
             break;
        case SQL_PARSE_OK:
        case EOL:
            ast_destroy_tree_from_root (astroot);
            return;
        case QUIT:
            exit(0);
        default:
            printf ("%s : Unsupported Command\n", yytext);
            yylex(); // consume \n
            break;
    }

    t = clock() - t;
    double time_taken = ((double)t * 1000)/CLOCKS_PER_SEC; 
    printf("%f msec\n", time_taken);
    ast_destroy_tree_from_root (astroot);
}

int 
main (int argc, char **argv) {

    while(1) {
        parse_sql_query();
    }
    return 0;
}