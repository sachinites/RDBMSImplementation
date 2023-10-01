#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "ParserExport.h"
#include "SqlParserStruct.h"
#include "../../MathExpressionParser/ParserMexpr.h"
#include "../core/sql_mexpr_intf.h"
#include "../core/qep.h"

/* CFG 

select_query_parser -> select COLLIST  from TABS WHERE GRPBY ORDERBY LMT

COLLIST -> COL | COL , COLLIST

COL -> AGG_FN(MEXPR) | AGG_FN(MEXPR) as L

L -> $ | as <identifer>

TABS -> <ident> L | <ident> L , TABS

WHERE -> $ | where LEXPR

GRPBY ->  $  |  group by INDTF_LST HAVING

HAVING  ->  $  |  having LEXPR

INDTF_LST -> IDENT | IDENT , INDTF_LST

IDENT -> identifer | identider.identifer  [identifer can be col name or name of colmn alias] 
                                                                [identider.identifer is table_name.colmn name only ]

ORDERBY  -> $  |  order by IDENT C

C ->  asc |  dsc

LMT  -> $  |  limit <integer>

*/


qep_struct_t qep;
unsigned char *L_alias_name = NULL;

static parse_rc_t WHERE() ;
static parse_rc_t TABS() ;
static parse_rc_t L() ;
static parse_rc_t COL();
static parse_rc_t COLLIST();


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
        printf ("Error : Coulld not build Where Logical Expression Tree\n");
        RETURN_PARSE_ERROR;
    }

    RETURN_PARSE_SUCCESS;
}


/* TABS -> <ident> L | <ident> L , TABS */
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
    assert (err == PARSE_SUCCESS);

    token_code = cyylex();

    if (token_code != SQL_COMMA) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;
    }

    err = TABS();

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS;
}

/* L -> $ | as <identifer> */
parse_rc_t
L() {

    parse_init();

    token_code = cyylex();

    if (token_code != SQL_AS) {
        yyrewind(1);
        L_alias_name[0] = '\0';
       RETURN_PARSE_SUCCESS;
    }

    token_code = cyylex ();

    if (token_code != SQL_IDENTIFIER) {
        yyrewind(2);
        L_alias_name[0] = '\0';
        RETURN_PARSE_SUCCESS;
    }

    strncpy (L_alias_name,  lex_curr_token, lex_curr_token_len);
    RETURN_PARSE_SUCCESS;
}

/* COL -> MEXPR | MEXPR as L | AGG_FN(MEXPR) | AGG_FN(MEXPR) as L */
parse_rc_t
COL() {

    parse_init();

    qp_col_t *qp_col = (qp_col_t *)calloc (1, sizeof (qp_col_t));
    qp_col->agg_fn = SQL_AGG_FN_NONE;
    qp_col->alias_provided_by_user = false;

    do {

        /* COL -> MEXPR */
        qp_col->sql_tree = sql_create_exp_tree_compute ();

        if (!qp_col->sql_tree) break;
        
        /* COL ->MEXPR as L */
        L_alias_name = qp_col->alias_name;
        err = L();
        assert (err == PARSE_SUCCESS);
        
        if (qp_col->alias_name[0] != '\0') {
            qp_col->alias_provided_by_user = true;
        }

        qep.select.sel_colmns[qep.select.n++] = qp_col;
        RETURN_PARSE_SUCCESS;

    } while (0);


    do {

        /* COL -> AGG_FN(MEXPR) */
        token_code = cyylex ();
        if (token_code != SQL_SUM &&
            token_code != SQL_MIN &&
            token_code != SQL_MAX &&
            token_code != SQL_COUNT &&
            token_code != SQL_AVG) {

            PARSER_LOG_ERR(token_code, SQL_SUM);
            PARSER_LOG_ERR(token_code, SQL_MIN);
            PARSER_LOG_ERR(token_code, SQL_MAX);
            PARSER_LOG_ERR(token_code, SQL_COUNT);
            PARSER_LOG_ERR(token_code, SQL_AVG);
            RETURN_PARSE_ERROR;
        }

        qp_col->agg_fn = token_code;

        token_code = cyylex();

        if (token_code != SQL_BRACKET_START) RETURN_PARSE_ERROR;

        qp_col->sql_tree = sql_create_exp_tree_compute ();

        if (!qp_col->sql_tree) RETURN_PARSE_ERROR;

        token_code = cyylex();

         if (token_code != SQL_BRACKET_END) RETURN_PARSE_ERROR;

         /* COL -> AGG_FN(MEXPR) as L */
         L_alias_name = qp_col->alias_name;
         err = L();
         assert (err == PARSE_SUCCESS);      

    }   while (0); 

    RETURN_PARSE_SUCCESS;
}


/* COLLIST -> * | COL | COL , COLLIST */
parse_rc_t
COLLIST() {

    parse_init ();

    token_code = cyylex();

    if (token_code == SQL_MATH_MUL) {
        qep.select.n = 0;
        RETURN_PARSE_SUCCESS;
    }

    yyrewind(1);

    err = COL();

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    token_code = cyylex();

    if (token_code != SQL_COMMA) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;
    }

    err = COLLIST();

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;
    RETURN_PARSE_SUCCESS;
}

/* select_query_parser -> select COLS from TABS WHERE GRPBY ORDERBY LMT */
parse_rc_t
select_query_parser () {

    parse_init();

    memset (&qep, 0, sizeof (qep));

    /* consume 'select' keyword */
    token_code = cyylex ();
    assert (token_code == SQL_SELECT_Q);

    /* Now parse list of Columns*/
    err = COLLIST();

    if (err == PARSE_ERR) {

        printf ("Failed\n");
        RETURN_PARSE_ERROR;
    }

    token_code = cyylex();

    if (token_code != SQL_FROM) {

        PARSER_LOG_ERR(token_code, SQL_FROM);
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
        printf ("Failed\n");
        RETURN_PARSE_ERROR;
    }

    printf ("Success\n");
    RETURN_PARSE_SUCCESS;
}
