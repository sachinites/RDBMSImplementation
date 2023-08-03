#ifndef __CATALOG__
#define __CATALOG__

#include <stdbool.h>
#include "sql_const.h"

typedef struct BPlusTree BPlusTree_t ;
typedef struct ast_node_ ast_node_t;
typedef struct BPlusTreeNode BPlusTreeNode ;

typedef struct schema_rec_ {

    sql_dype_t dtype;
    int dtype_size;
    bool is_primary_key;
    bool is_non_null;

} schema_rec_t;

typedef struct catalog_table_value {

    BPlusTree_t  *rdbms_table;
    BPlusTree_t  *schema_table;

} ctable_val_t;

bool 
Catalog_insert_new_table (BPlusTree_t *catalog, ast_node_t *root) ;

int
Catalog_create_schema_table_records (ast_node_t *root,  
                                                        BPluskey_t ***bkeys,
                                                        schema_rec_t ***crecords) ;

#endif 
