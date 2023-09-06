#ifndef __SQL_GROUPBY__
#define __SQL_GROUPBY__

#include "../Tree/libtree.h"

typedef struct  joined_row_  joined_row_t;
typedef struct qep_struct_ qep_struct_t ;

typedef struct order_by_avl_node {

    qep_struct_t *qep;
    avltree_node_t avl_node;

} order_by_avl_node_t;

void *
sql_compute_group_by_clause_keys (qep_struct_t *qep_struct,  joined_row_t  *joined_row) ;

 void 
 sql_process_group_by (qep_struct_t *qep_struct) ;
 
 void 
sql_group_by_init_groupby_trees (qep_struct_t *qep) ;

#endif 