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
parse_rc_t
VALUE () {

    parse_init();

    token_code = cyylex();

    switch (token_code ) {

        case SQL_INTEGER_VALUE:
            idata.sql_values[idata.n].dtype = SQL_INT;
            idata.sql_values[idata.n].u.int_val = atoi (lex_curr_token);
            break;
        case SQL_DOUBLE_VALUE:
            idata.sql_values[idata.n].dtype = SQL_DOUBLE;
            idata.sql_values[idata.n].u.d_val = (double)atof (lex_curr_token);
            break;
        case SQL_STRING_VALUE:
            idata.sql_values[idata.n].dtype = SQL_STRING;
            if ( (sizeof (idata.sql_values[idata.n].u.str_val) <= lex_curr_token_len -2)) {
                printf ("Error : %s(%d) Buffer overflow\n", __FUNCTION__, __LINE__);
                RETURN_PARSE_ERROR;
            }
            /* Make sure you dont copy double/single quotes which sandwich the string.
            For ex :  lex_curr_token = "Abhishek" (including double quotes), you should 
            copy only Abhishek  (without double/single quotes)*/
            strncpy (idata.sql_values[idata.n].u.str_val, lex_curr_token + 1,  // skip ' or "
                        lex_curr_token_len - 2); 
            break;
        default:
            RETURN_PARSE_ERROR;
    }
    idata.n++;
    RETURN_PARSE_SUCCESS;
}


// VALUES -> VALUE , VALUES
// VALUES -> VALUE 

static parse_rc_t
VALUES() {

    parse_init();

    int initial_chkp;
    CHECKPOINT(initial_chkp);

    // VALUES -> VALUE , VALUES
    do {

        err = VALUE();

        if (err == PARSE_ERR) break;

        token_code = cyylex();

        if (token_code != SQL_COMMA) {
            idata.n--;
            break;
        }

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

    strncpy (idata.table_name, lex_curr_token, sizeof (idata.table_name));

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