#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#include "ParserExport.h"
#include "SqlEnums.h"
#include "../core/qep.h"
#include "../core/SqlMexprIntf.h"
#include "../core/rdbms_struct.h"

qep_struct_t qep;

/*
select_query_parser -> select COLLIST from TABS

COLLIST -> COL | COL , COLLIST

COL -> < sql_create_exp_tree_compute () ALIAS >  

TABS -> <ident> ALIAS | <ident> ALIAS , TABS 

ALIAS -> as <identifier> | $ 
*/

static char *alias_name = NULL;


//ALIAS -> as <identifier> | $ 
static parse_rc_t
ALIAS () {

    parse_init();

    alias_name[0] = '\0';
    
    token_code = cyylex();

    if (token_code != SQL_AS) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;
    }

    token_code = cyylex();

    if (token_code != SQL_IDENTIFIER) {
        yyrewind(2);
        RETURN_PARSE_SUCCESS;
    }

    strncpy (alias_name, lex_curr_token, SQL_ALIAS_NAME_LEN);

    RETURN_PARSE_SUCCESS;
}


//TABS -> <ident> ALIAS | <ident> ALIAS , TABS 
static parse_rc_t
TABS() {

    parse_init();
    int initial_chkp;

    CHECKPOINT(initial_chkp);

    //TABS ->  <ident> ALIAS , TABS 
    do {

        token_code = cyylex();

        if (token_code != SQL_IDENTIFIER) break;

        strncpy (qep.join.tables[qep.join.table_cnt].table_name, 
            lex_curr_token, SQL_TABLE_NAME_MAX_SIZE);

        alias_name = qep.join.tables[qep.join.table_cnt].alias_name;
        err = ALIAS();

        token_code = cyylex();

        if (token_code != SQL_COMMA) {
            alias_name[0] = '\0';
            break;
        }

        qep.join.table_cnt++;

        err = TABS();

        if (err == PARSE_ERR) break;

        RETURN_PARSE_SUCCESS;
        
    } while (0);

    RESTORE_CHKP(initial_chkp);

   //TABS -> <ident> ALIAS
    token_code = cyylex();

    if (token_code != SQL_IDENTIFIER) {
        PARSER_LOG_ERR(token_code, SQL_IDENTIFIER);
        RETURN_PARSE_ERROR;
    }
    
    strncpy (qep.join.tables[qep.join.table_cnt].table_name, 
            lex_curr_token, SQL_TABLE_NAME_MAX_SIZE);

    alias_name = qep.join.tables[qep.join.table_cnt].alias_name;
    err = ALIAS();
    qep.join.table_cnt++;

    RETURN_PARSE_SUCCESS;
}


// COL -> < sql_create_exp_tree_compute () ALIAS >  
static parse_rc_t 
COL() {

    parse_init();

    qp_col_t *qp_col = (qp_col_t *)calloc (1, sizeof (qp_col_t));
    qep.select.sel_colmns[qep.select.n] = qp_col;
    qep.select.n++;

    qp_col->sql_tree = sql_create_exp_tree_compute ();

    if (!qp_col->sql_tree) {

        free(qp_col);
        qep.select.n--;
        qep.select.sel_colmns[qep.select.n] = NULL;
        RETURN_PARSE_ERROR;   
    }

    alias_name = qp_col->alias_name;
    err = ALIAS();
    if (qp_col->alias_name[0] != '\0') {
        qp_col->alias_provided_by_user = true;
    }
    
    RETURN_PARSE_SUCCESS;
}


// COLLIST -> COL | COL , COLLIST
static  parse_rc_t 
 COLLIST() {

    parse_init();
    int initial_chkp;

    CHECKPOINT(initial_chkp);

    // COLLIST -> COL , COLLIST
    do {

        err = COL();

        if (err == PARSE_ERR) break;

        token_code = cyylex();

        if (token_code != SQL_COMMA) {
            
            qep.select.n--;
            sql_destroy_exp_tree(qep.select.sel_colmns[qep.select.n]->sql_tree);
            free(qep.select.sel_colmns[qep.select.n]);
            qep.select.sel_colmns[qep.select.n] = NULL;
            break;
        }

        err = COLLIST();

        if (err == PARSE_ERR) break;

        RETURN_PARSE_SUCCESS;

    } while (0);

    RESTORE_CHKP(initial_chkp);

    // COLLIST -> COL 

    err = COL();

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS;
 }


// select_query_parser -> select COLLIST from TABS
parse_rc_t 
select_query_parser() {

    parse_init();

    memset (&qep, 0 , sizeof (qep));

    token_code = cyylex();

    assert (token_code == SQL_SELECT_Q);

    err = COLLIST();

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    token_code = cyylex();

    if (token_code != SQL_FROM) RETURN_PARSE_ERROR;

    err = TABS();

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    token_code = cyylex();

    if (token_code != PARSER_EOL) {
        PARSER_LOG_ERR (token_code, PARSER_EOL);
        RETURN_PARSE_ERROR;
    }

    RETURN_PARSE_SUCCESS;
}