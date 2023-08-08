#ifndef __RDBMS_STRUCT__
#define __RDBMS_STRUCT__

#include "../Parsers/SQLParserStruct.h"
#include "../gluethread/glthread.h"
typedef struct catalog_table_value ctable_val_t ;
typedef struct schema_rec_ schema_rec_t ;
typedef struct ast_node_ ast_node_t;

typedef struct  key_mdata_ {

    sql_dype_t dtype;
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

} qp_col_t;

typedef struct qp_row_ {

     ctable_val_t *schema_table;
     void *record;

} qp_row_t;

typedef struct operand_val_ {

    sql_dype_t dtype;
    unsigned char operand_val[256];

} operand_val_t;

#endif 
