#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include "ParserExport.h"
#include "SqlEnums.h"

/* CFG
    create_query_parser -> create table <identifier> ( COLSLIST )

    COLSLIST -> COL | COL , COLSLIST          // list generation

    COL -> <identifier> DTYPE | <identifier> DTYPE primary key 

    DTYPE -> varchar (<number>) | int | double
*/

//DTYPE -> varchar (<number>) | int | double
parse_rc_t
DTYPE() {

    parse_init();

    token_code = cyylex();

    switch (token_code) {

        case SQL_INT:
        case SQL_DOUBLE:
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

    // COL -> <identifier> DTYPE primary key 

    do {

        token_code = cyylex();

        if (token_code != SQL_IDENTIFIER) break;

        err = DTYPE();

        if (err == PARSE_ERR) break;

        token_code = cyylex();

        if (token_code != SQL_PRIMARY_KEY) break;

        RETURN_PARSE_SUCCESS;

    } while (0);

    RESTORE_CHKP(initial_chkp);

      // COL -> <identifier> DTYPE

      do {

        token_code = cyylex();

        if (token_code != SQL_IDENTIFIER) {
            PARSER_LOG_ERR (token_code, SQL_IDENTIFIER);
            RETURN_PARSE_ERROR;
        }

        err = DTYPE();

        if (err == PARSE_ERR) RETURN_PARSE_ERROR;

        RETURN_PARSE_SUCCESS;

      } while (0);

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

        if (token_code != SQL_COMMA) break;

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