
#include <assert.h>
#include <stddef.h>
#include "../BPlusTreeLib/BPlusTree.h"
#include "qplanner.h"
#include "Catalog.h"
#include "../Parsers/Ast.h"
#include "../gluethread/glthread.h"

#define NOT_SUPPORT_YET assert(0)

static qp_row_t qp_row = {NULL, NULL};
static qp_response_t  qp_resp = {&qp_row};

#if 0
static glthread_t
select_column_list(qp_node_t *select_node) {

    glthread_t curr;
    return curr;
}

static void 
qep_submit_request_downstream_node (qp_node_t *downstream_node, qp_request_data_t *qp_request) {

}

qp_response_t  *
qep_request_downstream_node (qp_node_t *downstream_node, qp_request_data_t *qp_request) {

    switch (downstream_node->qp_node_type) {

        case QP_NODE_SELECT:
        {
             qep_submit_request_downstream_node (downstream_node->u.select.child[0], qp_request);

             if (IS_GLTHREAD_LIST_EMPTY (&downstream_node->u.select.col_list_head)) {
                downstream_node->u.select.col_list_head = select_column_list(downstream_node);
             }
             sql_emit_select_output (qp_resp.row1->schema_table, qp_resp.row1->record, 
                                                    &downstream_node->u.select.col_list_head);
             return NULL;
        }
        break;
        case QP_NODE_JOIN:
        {
            qep_submit_request_downstream_node (downstream_node->u.join.child[0], qp_request);
            if (downstream_node->u.join.child[1])
            qp_submit_request_downstream_node (downstream_node->u.join.child[1], qp_request);
            return &qp_resp;
        }

        case QP_NODE_WHERE:
        {
            assert (downstream_node->u.where.row == NULL);
            qep_submit_request_downstream_node (downstream_node->u.where.child[0], qp_request);
            /* process qp_resp, apply where condition*/
            return &qp_resp;
        }

        case QP_NODE_SEQ_SCAN:
        {
            BPluskey_t *data_table = downstream_node->u.seq_scan.ctable_val->rdbms_table;
           qp_resp.row1->record  =  BPlusTree_get_next_record (data_table, 
                     &downstream_node->u.seq_scan.bpnode,
                     &downstream_node->u.seq_scan.index);
            qp_resp.row1->schema_table = downstream_node->u.seq_scan.ctable_val->schema_table;
            return &qp_resp;
        }
        break;

        default:
            assert(0);
    }

}
#endif 