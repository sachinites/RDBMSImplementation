#ifndef __RDBMS_STRUCT__
#define __RDBMS_STRUCT__

#include <string>

#include "../SqlParser/SqlParserStruct.h"
#include "../gluethread/glthread.h"
#include "sql_const.h"
#include "../BPlusTreeLib/BPlusTree.h"

typedef struct catalog_table_value ctable_val_t ;
typedef struct schema_rec_ schema_rec_t ;
typedef struct ast_node_ ast_node_t;
typedef struct sql_exptree_ sql_exptree_t;

class Dtype;

typedef struct  key_mdata_ {

    sql_dtype_t dtype;
    int size;

} key_mdata_t ;

typedef struct list_node_ {

    void *data;
    glthread_t glue;

} list_node_t;
GLTHREAD_TO_STRUCT (glue_to_list_node, list_node_t, glue);

typedef struct qp_col_ {

    sql_exptree_t *sql_tree;
    sql_agg_fn_t agg_fn;
    Dtype* computed_value;
    bool alias_provided_by_user;
    char alias_name[SQL_ALIAS_NAME_LEN];
    
}qp_col_t;

typedef struct joined_row_ {

    // size : could be 1 when where is enforced on a single table OR
    // table_cnt when where is enforced on Joined Row
    int size;  
    BPlusTree_t **schema_table_array;
    void **rec_array;
    int *table_id_array;    

} joined_row_t;

#endif 
