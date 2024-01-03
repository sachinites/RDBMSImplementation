#ifndef __RDBMS_STRUCT__
#define __RDBMS_STRUCT__

#include "sql_const.h"

typedef struct sql_exptree_ sql_exptree_t;
typedef struct BPluskey BPluskey_t;
typedef struct schema_rec_ schema_rec_t;

class Dtype;

typedef struct qp_col_ {

    sql_exptree_t *sql_tree;
    Dtype *computed_value;
    bool alias_provided_by_user;
    char alias_name[SQL_ALIAS_NAME_LEN];

} qp_col_t ;

typedef struct joined_row_ {

    int size;  
    BPluskey_t **key_array;
    void **rec_array;

} joined_row_t;

typedef struct exp_tree_data_src_ {

    int table_index;
    schema_rec_t *schema_rec;
    joined_row_t **joined_row;

} exp_tree_data_src_t;

#endif 