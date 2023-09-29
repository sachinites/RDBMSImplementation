#include <stdio.h>
#include <stdbool.h>
#include "ParserExport.h"
#include "SQLParserStruct.h"
#include "../core/sql_create.h"
#include "../core/qep.h"
#include "../core/sql_insert_into.h"

extern parse_rc_t select_query_parser () ;
extern parse_rc_t create_query_parser () ;
extern parse_rc_t insert_into_query_parser () ;

extern void sql_show_table_catalog (BPlusTree_t *TableCatalog);

extern sql_create_data_t cdata; 
extern qep_struct_t qep;
extern sql_insert_into_data_t idata; 



int 
main (int argc, char **argv) {

    parse_init ();

    while (true) {

        printf ("postgres=# ");

        fgets (lex_buffer, sizeof (lex_buffer), stdin);

        if (lex_buffer[0] == '\n') {
            lex_buffer[0] = 0;
            continue;
        }

        lex_set_scan_buffer (lex_buffer);

        token_code = cyylex ();

        switch (token_code) {

            case SQL_SELECT_Q:

                yyrewind(1);
                err = select_query_parser ();
                if (err == PARSE_SUCCESS) {
                    sql_process_select_query (&qep);
                }
            break;

            case SQL_CREATE_Q:

                yyrewind(1);
                err = create_query_parser ();
                if (err == PARSE_SUCCESS) {
                    sql_process_create_query (&cdata);
                }
                break;


            case SQL_INSERT_Q:

                yyrewind(1);
                err = insert_into_query_parser();
                if (err == PARSE_SUCCESS) {
                    sql_process_insert_query (&idata);
                }
                break; 


            case SHOW_DB_TABLES:
                sql_show_table_catalog (NULL);
                break;


            default:
                printf ("Error : Unrecognized Input\n");
                break;
        }

        Parser_stack_reset();
    }

    return 0;
}