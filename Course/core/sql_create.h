#ifndef __SQL_CREATE__
#define __SQL_CREATE__

#include <stdbool.h>
#include "sql_const.h"
#include "../SqlParser/SqlEnums.h"

typedef struct  key_mdata_ key_mdata_t;

typedef struct sql_create_data_ {

    char table_name [SQL_TABLE_NAME_MAX_SIZE];

    int n_cols;

    struct {

        char col_name [SQL_COLUMN_NAME_MAX_SIZE];
        sql_dtype_t dtype;
        int dtype_len;
        bool is_primary_key;

    }  column_data [SQL_MAX_COLUMNS_SUPPORTED_PER_TABLE];

}  sql_create_data_t; 

 void 
 sql_process_create_query (sql_create_data_t *cdata) ;

 void 
sql_create_data_destroy (sql_create_data_t *cdata) ;

key_mdata_t *
sql_construct_table_key_mdata (sql_create_data_t *cdata, int *key_mdata_size);

#endif 