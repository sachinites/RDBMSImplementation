#include <string.h>
#include "sql_api.h"
#include "../core/qep.h"
#include "../../MathExpressionParser/Dtype.h"
#include "../SqlParser/ParserExport.h"

extern qep_struct_t qep;
extern parse_rc_t select_query_parser () ;

void
sql_select_exec (std::string sql_select_query,
                            sql_record_reader_fn_ptr sql_record_reader, 
                            void *app_data) {

    memset (&qep, 0, sizeof (qep));
    strncpy (lex_buffer, sql_select_query.c_str(), sql_select_query.size());
    lex_set_scan_buffer (lex_buffer);
    Parser_stack_reset();

    parse_rc_t rc = select_query_parser ();

    if (rc == PARSE_ERR) {
        qep_deinit (&qep);
        Parser_stack_reset();
        return ;
    }

    Parser_stack_reset();
    qep.select.app_data = app_data;
    qep.select.sql_record_reader = sql_record_reader;
    sql_execute_qep (&qep);
    qep_deinit(&qep);
}
