#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include "ParserExport.h"
#include "SqlEnums.h"
#include "../core/sql_create.h"

extern parse_rc_t create_query_parser () ;
extern sql_create_data_t cdata;

int
main(int argc, char **argv) {

    parse_init();

    while (true) {

        printf ("postgres=# ");

        fgets ((char *)lex_buffer, sizeof(lex_buffer), stdin);

        if (lex_buffer[0] == '\n') {
            lex_buffer[0] = 0;
            continue;
        }

        lex_set_scan_buffer (lex_buffer);

        token_code = cyylex();

        switch (token_code) {

            case SQL_SELECT_Q:
            break;

            case SQL_INSERT_Q:
            break;

            case SQL_CREATE_Q:
                yyrewind(1);
                err = create_query_parser();
                if (err == PARSE_SUCCESS) {
                    sql_process_create_query (&cdata);
                }
                 sql_create_data_destroy(&cdata);
            break;

            default:
                printf ("Error : Unrecognized Input\n");
                break;
        }

        Parser_stack_reset();
    }

    return 0;
}