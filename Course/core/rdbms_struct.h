#ifndef __RDBMS_STRUCT__
#define __RDBMS_STRUCT__

typedef struct sql_exptree_ sql_exptree_t;
class Dtype;

typedef struct qp_col_ {

    sql_exptree_t *sql_tree;
    Dtype *computed_value;

} qp_col_t ;


#endif 