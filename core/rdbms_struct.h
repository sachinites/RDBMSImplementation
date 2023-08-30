#ifndef __RDBMS_STRUCT__
#define __RDBMS_STRUCT__

#include "../Parsers/SQLParserStruct.h"
#include "../gluethread/glthread.h"
#include "sql_const.h"
#include "../BPlusTreeLib/BPlusTree.h"

typedef struct catalog_table_value ctable_val_t ;
typedef struct schema_rec_ schema_rec_t ;
typedef struct ast_node_ ast_node_t;

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

    ctable_val_t *ctable_val; 
    schema_rec_t *schema_rec;
    sql_agg_fn_t agg_fn;
    void *computed_value;

}qp_col_t;

typedef struct joined_row_ {

    BPlusTree_t **schema_table_array;
    void **rec_array;

} joined_row_t;


#endif 
