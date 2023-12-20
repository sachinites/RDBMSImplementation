#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include "ParserExport.h"
#include "SqlEnums.h"
#include "../core/sql_create.h"


/* CFG
    create_query_parser -> create table <identifier> ( COLSLIST )

    COLSLIST -> COL | COL , COLSLIST          // list generation

    COL -> <identifier> DTYPE | <identifier> DTYPE primary key 

    DTYPE -> varchar (<number>) | int | double
*/

sql_create_data_t cdata;

//DTYPE -> varchar (<number>) | int | double
parse_rc_t
DTYPE() {

    parse_init();

    token_code = cyylex();

    switch (token_code) {

        case SQL_INT:
        case SQL_DOUBLE:
            cdata.column_data[cdata.n_cols].dtype = (sql_dtype_t )token_code;
            cdata.column_data[cdata.n_cols].dtype_len = sql_dtype_size ((sql_dtype_t )token_code);
            RETURN_PARSE_SUCCESS;
        case SQL_STRING:
            token_code = cyylex();
            if (token_code != SQL_BRACKET_START) {
                PARSER_LOG_ERR(token_code, SQL_BRACKET_START);
                RETURN_PARSE_ERROR;
            }
            token_code = cyylex();
            if (token_code != SQL_INTEGER_VALUE) {
                PARSER_LOG_ERR(token_code, SQL_INTEGER_VALUE);
                RETURN_PARSE_ERROR;
            }
            cdata.column_data[cdata.n_cols].dtype = (sql_dtype_t )SQL_STRING;
            cdata.column_data[cdata.n_cols].dtype_len = atoi(lex_curr_token);
            token_code = cyylex();
            if (token_code != SQL_BRACKET_END) {
                PARSER_LOG_ERR(token_code, SQL_BRACKET_END);
                RETURN_PARSE_ERROR;
            }
            RETURN_PARSE_SUCCESS;
            break;
        default:
            RETURN_PARSE_ERROR;
    }
}



// COL -> <identifier> DTYPE | <identifier> DTYPE primary key 
parse_rc_t
COL() {

    parse_init();

    int initial_chkp;
    CHECKPOINT (initial_chkp);

    memset(&cdata.column_data[cdata.n_cols], 0, 
        sizeof (cdata.column_data[0]));

    // COL -> <identifier> DTYPE primary key 

    do {

        token_code = cyylex();

        if (token_code != SQL_IDENTIFIER) break;

        strncpy (cdata.column_data[cdata.n_cols].col_name, lex_curr_token, 
            SQL_COLUMN_NAME_MAX_SIZE);

        err = DTYPE();

        if (err == PARSE_ERR) break;

        token_code = cyylex();

        if (token_code != SQL_PRIMARY_KEY) break;

        cdata.column_data[cdata.n_cols].is_primary_key = true;
        cdata.n_cols++;

        RETURN_PARSE_SUCCESS;

    } while (0);

    RESTORE_CHKP(initial_chkp);

    memset(&cdata.column_data[cdata.n_cols], 0, 
        sizeof (cdata.column_data[0]));


      // COL -> <identifier> DTYPE

      do {

        token_code = cyylex();

        if (token_code != SQL_IDENTIFIER) {
            PARSER_LOG_ERR (token_code, SQL_IDENTIFIER);
            RETURN_PARSE_ERROR;
        }

        strncpy (cdata.column_data[cdata.n_cols].col_name, lex_curr_token, 
            SQL_COLUMN_NAME_MAX_SIZE);

        err = DTYPE();

        if (err == PARSE_ERR) RETURN_PARSE_ERROR;

        cdata.n_cols++;
        RETURN_PARSE_SUCCESS;

      } while (0);

    cdata.n_cols++;
    RETURN_PARSE_SUCCESS;
}



//COLSLIST -> COL | COL , COLSLIST          // list generation
parse_rc_t
COLSLIST() {

    parse_init();

    int initiak_chkp;
    CHECKPOINT(initiak_chkp);

    // COLSLIST -> COL , COLSLIST 

    do {

        err = COL();

        if (err == PARSE_ERR) break;

        token_code = cyylex();

        if (token_code != SQL_COMMA) {

            cdata.n_cols--;
            break;
        }

        err = COLSLIST();

        if (err == PARSE_ERR) break;

        RETURN_PARSE_SUCCESS;
    } while (0);

    RESTORE_CHKP(initiak_chkp);

     // COLSLIST -> COL

     do {

        err = COL();

        if (err == PARSE_ERR) RETURN_PARSE_ERROR;
        RETURN_PARSE_SUCCESS;
     } while (0);

     RETURN_PARSE_SUCCESS;
}



//  create_query_parser -> create table <identifier> ( COLSLIST ) 
parse_rc_t
create_query_parser () {

    parse_init();

    token_code = cyylex ();
    assert (token_code == SQL_CREATE_Q);

    memset (&cdata, 0, sizeof (cdata));
    
    token_code = cyylex();

    if (strcmp (lex_curr_token, "table")) {
        printf ("Error : Expected keyword \'table\' is missing, typo error  = %s\n", lex_curr_token);
        RETURN_PARSE_ERROR;
    }

    token_code = cyylex();

    if (token_code != SQL_IDENTIFIER) {

        PARSER_LOG_ERR(token_code, SQL_IDENTIFIER);
        RETURN_PARSE_ERROR;        
    }

    strncpy(cdata.table_name, lex_curr_token, SQL_TABLE_NAME_MAX_SIZE);

    token_code = cyylex();

    if (token_code != SQL_BRACKET_START) {

        PARSER_LOG_ERR (token_code, SQL_BRACKET_START);
        RETURN_PARSE_ERROR;
    }

    err = COLSLIST();

    if (err == PARSE_ERR) {
        RETURN_PARSE_ERROR;
    }

    token_code = cyylex();

    if (token_code != SQL_BRACKET_END) {

        PARSER_LOG_ERR (token_code, SQL_BRACKET_END);
        RETURN_PARSE_ERROR;
    }    

    token_code = cyylex();

    if (token_code != PARSER_EOL) {

         PARSER_LOG_ERR (token_code, PARSER_EOL);
         RETURN_PARSE_ERROR;
    }

    RETURN_PARSE_SUCCESS;
}