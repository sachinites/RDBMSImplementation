#ifndef __RDBMS_STRUCT__
#define __RDBMS_STRUCT__

typedef struct sql_exptree_ sql_exptree_t;
typedef struct BPluskey BPluskey_t;
class Dtype;

typedef struct qp_col_ {

    sql_exptree_t *sql_tree;
    Dtype *computed_value;

} qp_col_t ;

typedef struct joined_row_ {

    int size;  
    BPluskey_t **key_array;
    void **rec_array;
    int *table_id_array;    

} joined_row_t;

#endif 