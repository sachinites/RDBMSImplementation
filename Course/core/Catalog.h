#ifndef __CATALOG__
#define __CATALOG__

#include <stdbool.h>
#include "sql_const.h"
#include "../SqlParser/SqlEnums.h"

typedef struct BPlusTree BPlusTree_t ;
typedef struct BPluskey BPluskey_t;
typedef struct BPlusTreeNode BPlusTreeNode ;
typedef struct sql_create_data_ sql_create_data_t ;

typedef struct schema_rec_ {

    char column_name [SQL_COLUMN_NAME_MAX_SIZE];
    sql_dtype_t dtype;
    int dtype_size;
    bool is_primary_key;
    bool is_non_null;
    int offset;

} schema_rec_t;

typedef struct catalog_table_value {

    char table_name [SQL_TABLE_NAME_MAX_SIZE];
    BPlusTree_t  *record_table;
    BPlusTree_t  *schema_table;
    /* List of column names in the same order as they appear in create table query*/
    char column_lst[SQL_MAX_COLUMNS_SUPPORTED_PER_TABLE][SQL_COLUMN_NAME_MAX_SIZE];
    
} ctable_val_t;

bool 
Catalog_insert_new_table (BPlusTree_t *catalog, sql_create_data_t *cdata) ;

void 
Catalog_table_print (BPlusTree_t *catalog);

#endif 
