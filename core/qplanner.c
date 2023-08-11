#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <memory.h>
#include "../BPlusTreeLib/BPlusTree.h"
#include "qplanner.h"
#include "Catalog.h"
#include "../Parsers/Ast.h"
#include "../gluethread/glthread.h"
#include "sql_io.h"

#define NOT_SUPPORT_YET assert(0)

qep_t*
select_qep_prepare_execution_plan (BPlusTree_t *tcatalog, ast_node_t *root) {

    BPluskey_t bpkey;
    ast_node_t *curr, *curr2;
    ast_node_t tmplate_node;
    ast_node_t *agg_fn_node;
    ast_node_t *column_name_node;
    qp_node_t *qp_node_seq_scan = (qp_node_t *)calloc (1, sizeof (qp_node_t));

     /* QP_NODE_SEQ_SCAN */
     qp_node_seq_scan->qp_node_type = QP_NODE_SEQ_SCAN;
     tmplate_node.entity_type = SQL_IDENTIFIER;
     tmplate_node.u.identifier.ident_type = SQL_TABLE_NAME;

     ast_node_t *table_name_node = ast_find (root, &tmplate_node);
     assert (table_name_node);

    bpkey.key =  table_name_node->u.identifier.identifier.name;
    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;
    ctable_val_t *ctable_val = (ctable_val_t *) BPlusTree_Query_Key (tcatalog, &bpkey);
    qp_node_seq_scan->u.seq_scan.ctable_val = ctable_val;
    qp_node_seq_scan->u.seq_scan.index_col_scan = NULL;
    qp_node_seq_scan->u.seq_scan.bpnode = NULL;
    qp_node_seq_scan->u.seq_scan.index = 0;


    /* QP_NODE_SELECT */
    qp_node_t *qp_node_select = (qp_node_t *)calloc (1, sizeof (qp_node_t));
    qp_node_select->qp_node_type = QP_NODE_SELECT;

    int n_col = 0;

    FOR_ALL_AST_CHILD (table_name_node, curr) {
        n_col++; 
    } FOR_ALL_AST_CHILD_END ;

    assert (n_col);

    qp_node_select->u.select.n = n_col;
    qp_node_select->u.select.agg_fn = (sql_agg_fn_t *)calloc (n_col, sizeof (sql_agg_fn_t ));
    qp_node_select->u.select.col_names = (qp_col_t *)calloc (n_col, sizeof (qp_col_t));

    n_col = 0;
    FOR_ALL_AST_CHILD (table_name_node, curr) {
        
        column_name_node = curr;

        FOR_ALL_AST_CHILD (column_name_node, curr2) {
            
            switch (curr2->entity_type) {

                case SQL_AGG_FN:
                    qp_node_select->u.select.agg_fn[n_col] = curr2->u.agg_fn;
                    qp_node_select->u.select.is_aggrgated = true;
                break;
                default:
                break;
            }

        } FOR_ALL_AST_CHILD_END ;

        qp_node_select->u.select.col_names[n_col].ctable_val = ctable_val;
        bpkey.key = column_name_node->u.identifier.identifier.name;
        bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
        qp_node_select->u.select.col_names[n_col].schema_rec = (schema_rec_t *) BPlusTree_Query_Key (ctable_val->schema_table, &bpkey);
        assert (qp_node_select->u.select.col_names[n_col].schema_rec);
        n_col++;

    } FOR_ALL_AST_CHILD_END ;

    qp_node_select->u.select.child[0] = qp_node_seq_scan;

    qep_t *qep = (qep_t *)calloc (1, sizeof (qep_t));
    qep->root = qp_node_select;

    /* Once QEP is ready, we dont need AST any more */
    ast_destroy_tree_from_root (root);

    return qep;
}

static void *
qep_execute_seq_scan (qep_t *qep, qp_node_t *qp_node_seq_scan) {

    return (void *) BPlusTree_get_next_record (
                                qp_node_seq_scan->u.seq_scan.ctable_val->rdbms_table,
                                &qp_node_seq_scan->u.seq_scan.bpnode, 
                                &qp_node_seq_scan->u.seq_scan.index);
}

static void 
qep_execute_internal (qep_t *qep, qp_node_t *qp_node_root) {

     int rows_count = 0;

    switch (qp_node_root->qp_node_type) {

        case QP_NODE_SELECT:

            switch (qp_node_root->u.select.child[0]->qp_node_type) {

                case QP_NODE_SEQ_SCAN:
                {
                    sql_print_hdr(qp_node_root->u.select.col_names, qp_node_root->u.select.n);
                    qp_node_t *qp_node_seq_scan = qp_node_root->u.select.child[0];
                    void *record = NULL;
                    while ((record = qep_execute_seq_scan(qep, qp_node_seq_scan)))
                    {
                        if (!qp_node_root->parent)
                        {
                            sql_emit_select_output(
                            qp_node_seq_scan->u.seq_scan.ctable_val->schema_table,
                            qp_node_root->u.select.n,
                            qp_node_root->u.select.col_names,
                            record);
                            rows_count++;
                        }
                    }
                    printf("(%d rows)\n", rows_count);
                }
                break;


                case QP_NODE_AGGREGATOR:
                {
                    qp_node_t *qp_node_agg = qp_node_root->u.select.child[0];
                    /* initialize the Aggregate fields*/
                    int i;
                    qp_col_t *qp_col;
                    for (i = 0; i < qp_node_root->u.select.n; i++) {
                        qp_col = &qp_node_root->u.select.col_names[i];
                        memset (qp_col->computed_value, 0, sizeof (qp_col->computed_value));
                    }
                }
                break;
                default:
                    break;
            }

        default:
            break;
    }
}

void 
qep_execute (qep_t *qep) {

    qep_execute_internal (qep, qep->root);
    qep_destory (qep);
}

void 
qep_destory (qep_t *qep) {


}