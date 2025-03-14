#include <unistd.h>
#include "sql_api.h"

int 
main (int argc, char **argv) {

    sql_query_exec ("create table test (a int primary key, b int, c int)\n");
    sql_query_exec ("insert into test values (1, 2, 3)\n");
    sql_query_exec ("insert into test values (14 , 2, 3)\n");
    sql_query_exec ("insert into test values (2, 2, 3)\n");
    sql_query_exec ("select * from test\n");
    sql_query_exec ("drop table test\n");

    return 0;
}