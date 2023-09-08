#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include "../c-hashtable/hashtable.h"
#include "../c-hashtable/hashtable_itr.h"
#include "sql_utils.h"
#include "sql_io.h"
#include "sql_where.h"
#include "qplanner.h"
#include "Catalog.h"
#include "sql_groupby.h"
#include "../stack/stack.h"
#include "../Parsers/common.h"

void *
sql_compute_group_by_clause_keys (qep_struct_t *qep_struct,  joined_row_t  *joined_row) {

    int i;
    int key_size;
    qp_col_t *qp_col;

    key_size = sql_compute_group_by_key_size (qep_struct);

    unsigned char *key_out = (unsigned char *)calloc (key_size, 1);
    int offset = 0;
    void *key;

    for (i = 0; i < qep_struct->groupby.n; i++) {

        qp_col = qep_struct->groupby.col_list[i];
        key = sql_get_column_value_from_joined_row (joined_row, qp_col);
        assert(key);

        /* For string keys, remove '\0' and fill te remaining space with
            some junk letter say 'z'. This is necessary to work with hashtables.
            Though I personally dont like to process each character in a loop as
            it increases memory I/O operations ( performance hit )*/

        if (qp_col->schema_rec->dtype == SQL_STRING) {

            unsigned char c, *key2 = (unsigned char *)key;
            unsigned char *key_out2 = (unsigned char *)key_out + offset;
            int j = 0;

            while (((c = key2[j]) != '\0') && j < qp_col->schema_rec->dtype_size ) {
                key_out2[j] = c;
                j++;
            }

            for ( ; j <  qp_col->schema_rec->dtype_size; j++) {
                 key_out2[j] = 'z';
            }

            offset += qp_col->schema_rec->dtype_size;
            continue;
        }

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

static bool 
qep_enforce_having_clause_phase2 (qep_struct_t *qep_struct) {

    return true;
}

int
sql_compute_group_by_key_size (qep_struct_t *qep_struct) {

    int i;
    int size = 0;
    qp_col_t *col;

    if (qep_struct->groupby.n == 0) return 0;

    for (i = 0; i < qep_struct->groupby.n ; i++) {

        col = qep_struct->groupby.col_list[i];
        size += col->schema_rec->dtype_size;
    }
    return size;
}


 void 
 sql_process_group_by (qep_struct_t *qep_struct) {

    int i;
    void *val;
    qp_col_t *col;
    glthread_t *curr;
    list_node_t *lnode;
    int row_no = 0;
    struct hashtable_itr *itr;
    list_node_t *lnode_head;
    joined_row_t *joined_row;
    int row_no_per_group = 0;

    if (hashtable_count(qep_struct->groupby.ht) == 0) {
        return;
    }

     qep_struct->having.having_phase = 2;

    itr = hashtable_iterator(qep_struct->groupby.ht);

    do {

        void *ht_key =  hashtable_iterator_key(itr);
        lnode_head = (list_node_t *) hashtable_iterator_value (itr);
        row_no ++;
        row_no_per_group = 0;

        /* Fill non-aggregated columns in select list first. This includes
            the columns mentioned in group-by clause itself*/
        for (i = 0; i < qep_struct->select.n; i++) {

            col = qep_struct->select.sel_colmns[i];
            if (col->agg_fn != SQL_AGG_FN_NONE) {
                continue;
            }
            if (!col->computed_value) {

                col->computed_value = calloc(col->schema_rec->dtype_size, 1);
            }
            curr = glthread_get_next(&lnode_head->glue);
            lnode = glue_to_list_node(curr);
            joined_row = (joined_row_t *) lnode->data;
            val = sql_get_column_value_from_joined_row(joined_row, col);
            assert(val);
            memcpy(col->computed_value, val, col->schema_rec->dtype_size);
        }

        /* Now fill the aggregated columns in select list*/
        ITERATE_GLTHREAD_BEGIN (&lnode_head->glue, curr) {

            lnode = glue_to_list_node (curr);
            joined_row = lnode->data;
            row_no_per_group++; 

            for (i = 0; i < qep_struct->select.n; i++) {

                col = qep_struct->select.sel_colmns[i];
                
                if (col->agg_fn == SQL_AGG_FN_NONE) continue;

                if (!col->computed_value) {
                    col->computed_value = calloc(col->schema_rec->dtype_size, 1);
                }

                if (row_no_per_group == 1)
                {
                    if (col->agg_fn == SQL_COUNT)
                    {
                        *(int *)(col->computed_value) = 1;
                    }
                    else
                    {
                        val = sql_get_column_value_from_joined_row(joined_row, col);
                        memset (col->computed_value, 0 , col->schema_rec->dtype_size);
                        memcpy(col->computed_value, val, col->schema_rec->dtype_size);
                    }
                }
                else
                {
                    val = sql_get_column_value_from_joined_row(joined_row, col);
                    sql_compute_aggregate(col->agg_fn, val,
                                          col->computed_value,
                                          col->schema_rec->dtype,
                                          col->schema_rec->dtype_size,
                                          row_no_per_group);
                }
            }

            remove_glthread(curr);
            free (joined_row->rec_array);
            /* Do not free below arrays, as they are shared with
                joined_row_tmplate (only one copy)*/
            //free (joined_row->schema_table_array);
            //free(joined_row->table_id_array);
            free(joined_row);
            free (lnode);

        } ITERATE_GLTHREAD_END (&lnode_head->glue, curr);

        if (!qep_enforce_having_clause (qep_struct, NULL)) {
            if (hashtable_iterator_advance(itr)) continue;
            break;
        }


        if (row_no == 1) {
            
            sql_print_hdr  (qep_struct->select.sel_colmns, qep_struct->select.n);
        }

        sql_emit_select_output (qep_struct->select.n, qep_struct->select.sel_colmns);
        if (qep_struct->limit == row_no) {
            break;
        }

    } while (hashtable_iterator_advance(itr));

    free(itr);
 }

bool 
qep_enforce_having_clause  (qep_struct_t *qep_struct, joined_row_t *joined_row) {

    if (!qep_struct->having.expt_root) return true;
    return sql_evaluate_where_expression_tree (qep_struct, qep_struct->having.expt_root, joined_row);
}

/* To be used in Group by Phase 2 filteration */
 bool 
 sql_having_compute (qep_struct_t *qep_struct, where_cond_t *wc, joined_row_t *joined_row) {

    void *lval;
    void *rval;
    int l_value_size;
    int r_value_size;
    qp_col_t *lcol;
    qp_col_t *rcol;

    /* Compute lval*/
    lcol = &wc->col;
    l_value_size = lcol->schema_rec->dtype_size;

    if (qep_struct->having.having_phase == 1) {

        if (lcol->agg_fn == SQL_AGG_FN_NONE) {

            lval = (void *) sql_get_column_value_from_joined_row (joined_row, lcol);
            if (!lval) return false;
        }
        else {
            return true;
        }
    }
    else {

        lval = (void *)lcol->grpby_col_to_select_col_linkage->computed_value;
        if (!lval) return false;
    }

    /* Compute rval*/
    switch (wc->right_op.w_opd) {

        case WH_COL:
            rcol = &wc->right_op.u.col;
            r_value_size = rcol->schema_rec->dtype_size;

            if (qep_struct->having.having_phase == 1) {

                if (rcol->agg_fn == SQL_AGG_FN_NONE) {

                    rval = (void *) sql_get_column_value_from_joined_row (joined_row, rcol);
                    if (!rval) return false;
                }
                else {
                    return true;
                }
            }
            else {

                rval = (void *)rcol->grpby_col_to_select_col_linkage->computed_value;
                if (!rval) return false;
            }

            assert (lcol->schema_rec->dtype == rcol->schema_rec->dtype);
            return sql_where_compare (lval, l_value_size, rval, r_value_size, lcol->schema_rec->dtype, wc->op);

        case WH_VALUE:
            rval = wc->right_op.u.value.val;
            r_value_size = wc->right_op.u.value.size;
            assert (lcol->schema_rec->dtype == wc->right_op.u.value.dtype);
            return sql_where_compare (lval, l_value_size, rval, r_value_size, lcol->schema_rec->dtype, wc->op);

        default:
            assert(0);
    }
    
    assert(0);
    return false;
 }