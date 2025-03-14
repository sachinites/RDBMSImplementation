#include <string.h>
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

void
sql_query_exec(char *sql_query)
{

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
            sql_execute_qep(&qep);
        }
        qep_deinit(&qep);
        break;

    case SQL_CREATE_Q:

        yyrewind(1);
        err = create_query_parser();
        if (err == PARSE_SUCCESS)
        {
            sql_process_create_query(&cdata);
        }
        sql_create_data_destroy(&cdata);
        break;

    case SQL_INSERT_Q:

        yyrewind(1);
        err = insert_into_query_parser();
        if (err == PARSE_SUCCESS)
        {
            sql_process_insert_query(&idata);
        }
        sql_insert_into_data_destroy(&idata);
        break;

    case SQL_DROP_TABLE_Q:
    {
        char *table_name;
        token_code = cyylex();
        if (strcmp(lex_curr_token, "table"))
        {
            printf("Error : Unrecognized Input\n");
            break;
        }
        token_code = cyylex();
        if (token_code != SQL_IDENTIFIER)
        {
            printf("Error : Unrecognized Input\n");
            break;
        }
        table_name = lex_curr_token;
        token_code = cyylex();
        if (token_code != PARSER_EOL)
        {
            printf("Error : Unrecognized Input\n");
            break;
        }
        sql_drop_table(table_name);
        break;
    }

    case SQL_DELETE_Q:
        yyrewind(1);
        err = delete_query_parser();
        if (err == PARSE_SUCCESS)
        {
            sql_execute_qep(&qep);
        }
        qep_deinit(&qep);
        break;

    case SQL_UPDATE_Q:
        yyrewind(1);
        err = update_query_parser();
        if (err == PARSE_SUCCESS)
        {
            sql_execute_qep(&qep);
        }
        qep_deinit(&qep);
        break;

    default:
        printf("Error : Unrecognized Input\n");
        break;

        Parser_stack_reset();
    }

}