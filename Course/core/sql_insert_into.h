#ifndef __SQL_INSERT_INTO__
#define __SQL_INSERT_INTO__

#include <stdbool.h>
#include "sql_const.h"
#include "../SqlParser/SqlEnums.h"

typedef struct sql_value_ {

    sql_dtype_t dtype;

    union {

        char str_val[SQL_STRING_MAX_VALUE_LEN];
        int int_val;
        double d_val;
    } u;
} sql_value_t;


typedef struct sql_insert_into_data_ {

    char table_name[SQL_TABLE_NAME_MAX_SIZE];
    int n;
    sql_value_t  sql_values [SQL_MAX_COLUMNS_SUPPORTED_PER_TABLE];

} sql_insert_into_data_t;


void
sql_process_insert_query (sql_insert_into_data_t *idata);

#endif 
