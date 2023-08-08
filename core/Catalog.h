#ifndef __CATALOG__
#define __CATALOG__

#include <stdbool.h>
#include "sql_const.h"
#include "rdbms_struct.h"
#include "../Parsers/SQLParserStruct.h"

typedef struct BPlusTree BPlusTree_t ;
typedef struct BPluskey BPluskey_t;
typedef struct ast_node_ ast_node_t;
typedef struct BPlusTreeNode BPlusTreeNode ;

typedef struct schema_rec_ {

    unsigned char column_name [SQL_COLUMN_NAME_MAX_SIZE];
    sql_dype_t dtype;
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
Catalog_insert_new_table (BPlusTree_t *catalog, ast_node_t *root) ;

int
Catalog_create_schema_table_records (ast_node_t *root,  
                                                        BPluskey_t ***bkeys,
                                                        schema_rec_t ***crecords) ;

#endif 
