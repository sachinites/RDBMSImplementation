#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include "sql_utils.h"
#include "sql_where.h"
#include "qplanner.h"
#include "Catalog.h"
#include "sql_groupby.h"
#include "../stack/stack.h"
#include "../Parsers/common.h"

void *
sql_compute_group_by_clause_keys (qep_struct_t *qep_struct,  joined_row_t  *joined_row) {

    int i;
    qp_col_t *qp_col;
    int key_size = 0;

    for (i = 0; i < qep_struct->groupby.n; i++) {

        qp_col = qep_struct->groupby.col_list[i];
        key_size += qp_col->schema_rec->dtype_size;    
    }

    unsigned char *key_out = (unsigned char *)calloc (key_size, 1);
    int offset = 0;
    void *key;

    for (i = 0; i < qep_struct->groupby.n; i++) {

        qp_col = qep_struct->groupby.col_list[i];
        key = sql_get_column_value_from_joined_row (joined_row, qp_col);
        assert(key);
        memcpy (key_out + offset, key, qp_col->schema_rec->dtype_size);
        offset += qp_col->schema_rec->dtype_size;
    }

    return (void *)key_out;
}

#if 0
static int 
group_by_key_comp_fn (const avltree_node_t *_c1_new, const avltree_node_t *_c2_existing) {

    int i;
    int rc;
    void *key_ptr1;
    void *key_ptr2;
    qp_col_t *qp_col;
    qep_struct_t *qep;

    group_by_avl_node_t *new_node = avltree_container_of(_c1_new, group_by_avl_node_t, avl_node);
    group_by_avl_node_t *ex_node = avltree_container_of(_c2_existing, group_by_avl_node_t, avl_node);

    qep = new_node->qep;

    for (i = 0 ; i < qep->groupby.n; i++) {

        qp_col = qep->groupby.col_list[i];
        key_ptr1 = sql_get_column_value_from_joined_row (&new_node->joined_row, qp_col);
        key_ptr2 = sql_get_column_value_from_joined_row (&ex_node->joined_row, qp_col);
        /* For grouping of record based on keys, comparison based on memcmp is enough. No need to
            typecast keys into actual data type for this since we need to know either keys are equal or not, and not which key is less than the other based on data type*/
        rc = memcmp (key_ptr1, key_ptr2, qp_col->schema_rec->dtype_size);
        if (rc) return rc;
    }
    return 0;
}
#endif 
/* This fn requires exact comparison based on data type of the 'order by' column/field*/
static int 
order_by_key_comp_fn (const avltree_node_t *_c1_new, const avltree_node_t *_c2_existing) {

    qp_col_t *qp_col;
    qep_struct_t *qep;

    order_by_avl_node_t *new_node = avltree_container_of(_c1_new, order_by_avl_node_t, avl_node);
    order_by_avl_node_t *ex_node = avltree_container_of(_c2_existing, order_by_avl_node_t, avl_node);

    qep =  new_node->qep;

    if (!qep->orderby.col) {
        return -1; // FCFS , new record is always bigger
    }

    /* ToDo : Order by column based on ASC & DESC*/

    return 0;
}

void 
sql_group_by_init_groupby_trees (qep_struct_t *qep) {

#if 0
    avltree_init(&qep->groupby.avl_grpby_root, group_by_key_comp_fn);
#endif
    avltree_init(&qep->orderby.avl_order_by_root, order_by_key_comp_fn);
}

 void 
 sql_process_group_by (qep_struct_t *qep_struct) {

    
 }