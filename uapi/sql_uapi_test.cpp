#include <unistd.h>
#include "sql_api.h"

int 
main (int argc, char **argv) {

    char err_msg[128];
    rdbms_t *rdbms = rdbms_create ();
    if (!rdbms) return 1;

    sql_query_exec (rdbms, (char *)"create table test (a int primary key, b int, c int)\n", err_msg);
    sql_query_exec (rdbms, (char *)"insert into test values (1, 2, 3)\n", err_msg);
    sql_query_exec (rdbms, (char *)"insert into test values (14 , 2, 3)\n", err_msg);
    sql_query_exec (rdbms, (char *)"insert into test values (2, 2, 3)\n", err_msg);
    sql_query_exec (rdbms, (char *)"select * from test\n", err_msg);
    sql_query_exec (rdbms, (char *)"drop table test\n", err_msg);

    rdbms_destroy (rdbms);
    return 0;
}
