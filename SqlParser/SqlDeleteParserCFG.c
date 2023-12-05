#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "ParserExport.h"
#include "SqlEnums.h"
#include "../core/qep.h"
#include "../core/SqlMexprIntf.h"
#include "../core/sql_delete.h"
#include "../core/sql_const.h"

 /* CFG : 
    delete_query_parser -> delete from TABS WHERE
    TABS -> <ident> L
    L -> $ | as <identifier>
    WHERE -> $ | where LEXPR 
 */

extern qep_struct_t qep;
static char *L_alias_name = NULL;

static parse_rc_t WHERE() ;
static parse_rc_t L() ;
static parse_rc_t TABS() ;

/* WHERE -> $ | where LEXPR */
parse_rc_t
WHERE() {

    parse_init ();

    token_code = cyylex();

    if (token_code != SQL_WHERE) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;
    }

    qep.where.gexptree = sql_create_exp_tree_conditional();

    if (!qep.where.gexptree) {
        printf ("Error : Could not build Where Logical Expression Tree\n");
        RETURN_PARSE_ERROR;
    }

    RETURN_PARSE_SUCCESS;
}

// L -> $ | as <identifier>
parse_rc_t
L() {

    parse_init();

     L_alias_name[0] = '\0';

    token_code = cyylex();

    if (token_code != SQL_AS) {
        yyrewind(1);
       RETURN_PARSE_SUCCESS;
    }

    token_code = cyylex ();

    if (token_code != SQL_IDENTIFIER) {
        yyrewind(2);
        RETURN_PARSE_SUCCESS;
    }

    strncpy (L_alias_name,  lex_curr_token, lex_curr_token_len);
    RETURN_PARSE_SUCCESS;
}

// TABS -> <ident> L
parse_rc_t
TABS() {

    parse_init();

    token_code = cyylex();

    if (token_code != SQL_IDENTIFIER) {
        PARSER_LOG_ERR (token_code, SQL_IDENTIFIER); 
        RETURN_PARSE_ERROR;
    }

    /* Store a Table name */
    if (!qep_struct_record_table (&qep, lex_curr_token)) {

        printf ("Error : Table %s Do not Exist\n", lex_curr_token);
        RETURN_PARSE_ERROR;
    }

    L_alias_name = qep.join.tables[qep.join.table_cnt].alias_name;

    err = L();

    if (err == PARSE_ERR ) {

        printf ("Error : Could not parse Alias name\n");
        RETURN_PARSE_ERROR;
    }

    qep.join.table_cnt++;

    RETURN_PARSE_SUCCESS;
}


parse_rc_t 
delete_query_parser () {

    parse_init();

    memset (&qep, 0, sizeof (qep));
    qep.query_type = SQL_DELETE_Q;

    token_code = cyylex();

    assert (token_code == SQL_DELETE_Q);

    token_code = cyylex();

    if (token_code != SQL_FROM) {
        PARSER_LOG_ERR (token_code, SQL_FROM);
        RETURN_PARSE_ERROR;
    }

    err = TABS();

    if (err == PARSE_ERR) {

        printf ("Error : Parsing Error on Tables\n");
        RETURN_PARSE_ERROR;        
    }

    err = WHERE();

    if (err == PARSE_ERR) {

        printf ("Error : Parsing Error on Where Clause\n");
        RETURN_PARSE_ERROR;        
    }

    token_code = cyylex ();

    if (token_code !=  PARSER_EOL) {
        PARSER_LOG_ERR (token_code, PARSER_EOL);
        RETURN_PARSE_ERROR;
    }

    RETURN_PARSE_SUCCESS;
}