#include <unistd.h>
#include "sql_api.h"

int 
main (int argc, char **argv) {

    char err_msg[128];
    sql_query_exec (0, "create table test (a int primary key, b int, c int)\n", err_msg);
    sql_query_exec (0, "insert into test values (1, 2, 3)\n", err_msg);
    sql_query_exec (0, "insert into test values (14 , 2, 3)\n", err_msg);
    sql_query_exec (0, "insert into test values (2, 2, 3)\n", err_msg);
    sql_query_exec (0, "select * from test\n", err_msg);
    sql_query_exec (0, "drop table test\n", err_msg);

    return 0;
}