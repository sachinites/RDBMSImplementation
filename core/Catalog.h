#ifndef __CATALOG__
#define __CATALOG__

#include <stdbool.h>
#include "sql_const.h"
#include "rdbms_struct.h"
#include "../SqlParser/SQLParserStruct.h"

typedef struct BPlusTree BPlusTree_t ;
typedef struct BPluskey BPluskey_t;
typedef struct BPlusTreeNode BPlusTreeNode ;
typedef struct sql_create_data_ sql_create_data_t ;


typedef struct schema_rec_ {

    unsigned char column_name [SQL_COLUMN_NAME_MAX_SIZE];
    sql_dtype_t dtype;
    int dtype_size;
    bool is_primary_key;
    bool is_non_null;
    int offset;

} schema_rec_t;

typedef struct catalog_table_value {

    unsigned char table_name [SQL_TABLE_NAME_MAX_SIZE];
    BPlusTree_t  *rdbms_table;
    BPlusTree_t  *schema_table;
    glthread_t col_list_head;
    
} ctable_val_t;

bool 
Catalog_insert_new_table (BPlusTree_t *catalog, sql_create_data_t *cdata) ;

int
Catalog_create_schema_table_records ( sql_create_data_t *cdata,
                                                        BPluskey_t ***bkeys,
                                                        schema_rec_t ***crecords) ;

bool
Catalog_get_column (BPlusTree_t *tcatalog, 
                                    char *table_name, 
                                    char *col_name,
                                    qp_col_t *qp_col);

bool
sql_process_select_wildcard (BPlusTree_t *tcatalog, ast_node_t *select_kw, ast_node_t *table_name_node) ;

void 
sql_show_table_catalog (BPlusTree_t *TableCatalog) ;

#endif 
