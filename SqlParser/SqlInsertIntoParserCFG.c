#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "ParserExport.h"
#include "SQLParserStruct.h"
#include "../../MathExpressionParser/ParserMexpr.h"
#include "../core/sql_mexpr_intf.h"
#include "../core/sql_insert_into.h"

/* CFG 

insert_query_parser -> Q1 | Q2 

INSERT INTO table_name
VALUES (value1, value2, value3, ...);

Q1 -> insert into IDENTFIER values (VALUES)
IDENTFIER -> <string>
VALUES -> VALUE | VALUE , VALUES
VALUE ->  <integer> | <double> | '<string>' | ipv4-addr

INSERT INTO table_name (column1, column2, column3, ...)
VALUES (value1, value2, value3, ...);

Q2 -> insert into IDENTFIER (COLS) values (VALUES)
VALUES -> Same as above
COLS -> COL | COL, COLS
COL -> IDENTFIER
IDENTFIER -> <string>

We are supporitng only Q1 method of specifying values

*/

sql_insert_into_data_t idata; 

/* VALUE ->  <integer> | <double> | '<string>' | ipv4-addr */
parse_rc_t
VALUE () {

    parse_init();

    token_code = cyylex();

    switch (token_code ) {

        case SQL_INTEGER_VALUE:
            idata.sql_values[idata.i].dtype = SQL_INT;
            idata.sql_values[idata.i].u.int_val = atoi (lex_curr_token);
            break;
        case SQL_DOUBLE_VALUE:
            idata.sql_values[idata.i].dtype = SQL_DOUBLE;
            idata.sql_values[idata.i].u.d_val = (double)atof (lex_curr_token);
            break;
        case SQL_IPV4_ADDR_VALUE:
            idata.sql_values[idata.i].dtype = SQL_IPV4_ADDR;
            strncpy (idata.sql_values[idata.i].u.ipv4_addr_str, lex_curr_token, 16);
            break;
        case SQL_QUOTATION_MARK:
            token_code = cyylex();
            if (token_code != SQL_IDENTIFIER) {
                PARSER_LOG_ERR(token_code, SQL_IDENTIFIER);
                RETURN_PARSE_ERROR;
            }

            idata.sql_values[idata.i].dtype = SQL_STRING;
            strncpy (idata.sql_values[idata.i].u.str_val, lex_curr_token, 
                        sizeof (idata.sql_values[idata.i].u.str_val));

             token_code = cyylex();
            if (token_code != SQL_QUOTATION_MARK) {
                PARSER_LOG_ERR(token_code, SQL_QUOTATION_MARK);
                RETURN_PARSE_ERROR;
            }

            RETURN_PARSE_SUCCESS;
        default:
            RETURN_PARSE_ERROR;
    }

    RETURN_PARSE_SUCCESS;
}


/* VALUES -> VALUE | VALUE , VALUES */
parse_rc_t
VALUES () {

    parse_init();

    err = VALUE();

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    idata.i++;

    token_code = cyylex();

    if (token_code != SQL_COMMA) {

        yyrewind(1);
        RETURN_PARSE_SUCCESS;
    }

    err = VALUES();

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS;
}


parse_rc_t
insert_into_query_parser () {

    parse_init();

    memset (&idata, 0, sizeof (idata));

    token_code = cyylex ();

    assert (token_code == SQL_INSERT_Q);

    token_code = cyylex();

    if (token_code != SQL_IDENTIFIER) {
        PARSER_LOG_ERR (token_code, SQL_IDENTIFIER);
        RETURN_PARSE_ERROR;
    }

    strncpy (idata.table_name, lex_curr_token, sizeof (idata.table_name));

    token_code = cyylex();

    if (token_code != SQL_IDENTIFIER) {
        PARSER_LOG_ERR (token_code, SQL_IDENTIFIER);
        RETURN_PARSE_ERROR;
    }

    if (strcmp (lex_curr_token, "values") || 
            lex_curr_token_len != 6) {

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

    token_code = cyylex ();

    if (token_code !=  PARSER_EOL) {
        PARSER_LOG_ERR (token_code, PARSER_EOL);
        sql_insert_into_data_destroy(&idata);
        printf ("Failed\n");
        RETURN_PARSE_ERROR;
    }

    RETURN_PARSE_SUCCESS;    
}