#include <string.h>
#include <assert.h>
#include "sql_api.h"
#include "../core/qep.h"
#include "../../MathExpressionParser/Dtype.h"
#include "../SqlParser/ParserExport.h"
#include "../SqlParser/ParserExport.h"
#include "../core/sql_create.h"
#include "../core/sql_insert_into.h"
#include "../core/sql_delete.h"


extern qep_struct_t qep;
extern parse_rc_t select_query_parser () ;
extern parse_rc_t create_query_parser () ;
extern parse_rc_t insert_into_query_parser () ;
extern parse_rc_t delete_query_parser () ;
extern parse_rc_t update_query_parser () ;

extern sql_create_data_t cdata; 
extern qep_struct_t qep;
extern sql_insert_into_data_t idata;

int
sql_query_exec (BPlusTree_t *sql_db, char *sql_query, char *err_msg)
{
    uint8_t rc = 0;
    
    parse_init();
    memset(&qep, 0, sizeof(qep));
    strncpy(lex_buffer, sql_query, strlen(sql_query));
    lex_set_scan_buffer(lex_buffer);
    Parser_stack_reset();

    token_code = cyylex();

    switch (token_code)
    {

    case SQL_SELECT_Q:

        yyrewind(1);
        err = select_query_parser();
        if (err == PARSE_SUCCESS)
        {
            sql_execute_qep(sql_db, &qep);
        }
        qep_deinit(&qep);
        break;

    case SQL_CREATE_Q:

        yyrewind(1);
        err = create_query_parser();
        if (err == PARSE_SUCCESS)
        {
            sql_process_create_query(sql_db, &cdata);
        }
        sql_create_data_destroy(&cdata);
        break;

    case SQL_INSERT_Q:

        yyrewind(1);
        err = insert_into_query_parser();
        if (err == PARSE_SUCCESS)
        {
            sql_process_insert_query(sql_db, &idata);
        }
        sql_insert_into_data_destroy(&idata);
        break;

    case SQL_DROP_TABLE_Q:
    {
        char *table_name;
        token_code = cyylex();
        if (strcmp(lex_curr_token, "table"))
        {
            sprintf (err_msg, "Error : Unrecognized Input\n");
            rc = -1;
            break;
        }
        token_code = cyylex();
        if (token_code != SQL_IDENTIFIER)
        {
            sprintf (err_msg, "Error : Unrecognized Input\n");
            rc = -1;
            break;
        }
        table_name = lex_curr_token;
        token_code = cyylex();
        if (token_code != PARSER_EOL)
        {
            sprintf (err_msg, "Error : Unrecognized Input\n");
            rc = -1;
            break;
        }
        sql_drop_table(sql_db, table_name);
        break;
    }

    case SQL_DELETE_Q:
        yyrewind(1);
        err = delete_query_parser();
        if (err == PARSE_SUCCESS)
        {
            sql_execute_qep(sql_db, &qep);
        }
        qep_deinit(&qep);
        break;

    case SQL_UPDATE_Q:
        yyrewind(1);
        err = update_query_parser();
        if (err == PARSE_SUCCESS)
        {
            sql_execute_qep(sql_db, &qep);
        }
        qep_deinit(&qep);
        break;

    default:
        sprintf (err_msg, "Error : Unrecognized Input\n");
        rc = -1;
        break;

        Parser_stack_reset();
        return rc;
    }

    return rc;
}

extern  int 
rdbms_key_comp_fn (BPluskey_t *key_1, BPluskey_t *key_2, key_mdata_t *key_mdata, int size);

void 
sql_init_db (BPlusTree_t **db) {

    assert (*db == NULL);
    *db = (BPlusTree_t *)calloc(1, sizeof(BPlusTree_t));
}