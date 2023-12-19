#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#include "ParserExport.h"
#include "SqlEnums.h"
#include "../core/sql_insert_into.h"

/* CFG 

INSERT INTO table_name
VALUES (value1, value2, value3, ...);

insert_into_query_parser ()  -> insert into IDENTFIER values (VALUES)
IDENTFIER -> <string>
VALUES -> VALUE | VALUE , VALUES
VALUE ->  <integer> | <double> | '<string>' | "<string>"

*/

sql_insert_into_data_t idata; 

// VALUE ->  <integer> | <double> | '<string>' | "<string>"
static parse_rc_t
VALUE() {

    parse_init();

    token_code = cyylex();

    switch (token_code) {

        case SQL_INTEGER_VALUE:
        case SQL_DOUBLE_VALUE:
        case SQL_STRING_VALUE:
            RETURN_PARSE_SUCCESS;
        default:
            RETURN_PARSE_ERROR;
    }
    RETURN_PARSE_ERROR;
}


// VALUES -> VALUE , VALUES
// VALUES -> VALUE 

static parse_rc_t
VALUES() {

    parse_init();

    int initial_chkp;

    // VALUES -> VALUE , VALUES
    do {

        err = VALUE();

        if (err == PARSE_ERR) break;

        token_code = cyylex();

        if (token_code != SQL_COMMA) break;

        err = VALUES ();

        if (err == PARSE_ERR) break;

        RETURN_PARSE_SUCCESS;

    } while (0);

    RESTORE_CHKP(initial_chkp);

    // VALUES -> VALUE 
    do {

        err = VALUE();

        if (err == PARSE_ERR) RETURN_PARSE_ERROR;

        RETURN_PARSE_SUCCESS;

    } while (0);

    RETURN_PARSE_SUCCESS;

}

// insert_into_query_parser ()  -> insert into IDENTFIER values (VALUES)
parse_rc_t
insert_into_query_parser () {

    parse_init();

    memset (&idata, 0, sizeof (idata));
    
    token_code = cyylex();

    assert (token_code == SQL_INSERT_Q);

    token_code = cyylex();

    if (token_code != SQL_IDENTIFIER) {
        PARSER_LOG_ERR (token_code, SQL_IDENTIFIER);
        RETURN_PARSE_ERROR;
    }

    token_code = cyylex();

    if (strcmp (lex_curr_token, "values")) {
        printf ("Error : Expected keyword \'values\' is missing, typo error  = %s\n", lex_curr_token);
        RETURN_PARSE_ERROR;
    }

    token_code = cyylex();

    if (token_code != SQL_BRACKET_START) {
        PARSER_LOG_ERR (token_code, SQL_BRACKET_START);
        RETURN_PARSE_ERROR;
    }

    err = VALUES();

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

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