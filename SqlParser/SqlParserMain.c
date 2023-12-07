#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "ParserExport.h"
#include "SqlEnums.h"
#include "../core/sql_const.h"
#include "../core/sql_create.h"
#include "../core/sql_delete.h"
#include "../core/qep.h"
#include "../core/sql_insert_into.h"

extern parse_rc_t select_query_parser () ;
extern parse_rc_t create_query_parser () ;
extern parse_rc_t insert_into_query_parser () ;
extern parse_rc_t delete_query_parser () ;
extern parse_rc_t update_query_parser () ;

extern void sql_show_table_catalog (BPlusTree_t *TableCatalog);

extern sql_create_data_t cdata; 
extern qep_struct_t qep;
extern sql_insert_into_data_t idata; 

int 
main (int argc, char **argv) {

    parse_init ();

    clock_t tclk ;
    double time_taken ;

    while (true) {

        printf ("postgres=# ");

        fgets ( (char *)lex_buffer, sizeof (lex_buffer), stdin);

        if (lex_buffer[0] == '\n') {
            lex_buffer[0] = 0;
            continue;
        }

        lex_set_scan_buffer (lex_buffer);

        tclk  = clock();

        token_code = cyylex ();

        switch (token_code) {

            case SQL_SELECT_Q:

                yyrewind(1);
                err = select_query_parser ();
                if (err == PARSE_SUCCESS) {
                    sql_execute_qep (&qep);
                }
                qep_deinit (&qep);
                break;

            case SQL_CREATE_Q:

                yyrewind(1);
                err = create_query_parser ();
                if (err == PARSE_SUCCESS) {
                    sql_process_create_query (&cdata);
                }
                sql_create_data_destroy(&cdata);
                break;

            case SQL_INSERT_Q:

                yyrewind(1);
                err = insert_into_query_parser();
                if (err == PARSE_SUCCESS) {
                    sql_process_insert_query (&idata);
                }
                sql_insert_into_data_destroy(&idata);
                break; 

            case SQL_DROP_TABLE_Q:

                {
                    char *table_name;
                    token_code = cyylex();
                    if (strcmp (lex_curr_token, "table")) {
                        printf ("Error : Unrecognized Input\n");
                        break;
                    }
                    token_code = cyylex();
                    if (token_code != SQL_IDENTIFIER) {
                        printf ("Error : Unrecognized Input\n");
                        break;
                    }
                    table_name = lex_curr_token;
                    token_code = cyylex();
                    if (token_code != PARSER_EOL) {
                        printf ("Error : Unrecognized Input\n");
                        break;
                    }
                    sql_drop_table (table_name);
                    break;
                }

            case SQL_DELETE_Q:
                yyrewind(1);
                err = delete_query_parser();
                if (err == PARSE_SUCCESS) {
                    sql_execute_qep (&qep);
                }
                qep_deinit (&qep);
            break;

            case SQL_UPDATE_Q:
                yyrewind(1);
                err = update_query_parser();
                if (err == PARSE_SUCCESS) {
                    sql_execute_qep (&qep);
                }
                qep_deinit (&qep);
                break;

            case SQL_SHOW_DB_TABLES:
                sql_show_table_catalog (NULL);
                break;

            default:
                printf ("Error : Unrecognized Input\n");
                break;
        }

        Parser_stack_reset();
        tclk= clock() - tclk;
        time_taken = ((double)tclk * 1000)/CLOCKS_PER_SEC; 
        printf("%f msec\n", time_taken);
    }

    return 0;
}
