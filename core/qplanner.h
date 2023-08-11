#ifndef __QPLANNER__
#define __QPLANNER__

#include "../Parsers/SQLParserStruct.h"
#include "sql_const.h"
#include "rdbms_struct.h"

typedef struct catalog_table_value ctable_val_t ;
typedef struct schema_rec_ schema_rec_t ;
typedef struct ast_node_ ast_node_t;
typedef struct BPlusTree BPlusTree_t;

typedef enum qp_node_type_ {

    QP_NODE_SEQ_SCAN,
    QP_NODE_WHERE,
    QP_NODE_JOIN,
    QP_NODE_AGGREGATOR,
    QP_NODE_GROUP_BY,
    QP_NODE_HAVING,
    QP_NODE_SELECT,
    QP_NODE_UNDEFINED

} qp_node_type_t;

typedef struct qp_node_ {
  
    struct qp_node_ *parent;

    qp_node_type_t qp_node_type;

    union {

        /* QP_NODE_LIMIT */

        /* QP_NODE_ORDER_BY*/

        /* QP_NODE_DISTINCT*/


        /* QP_NODE_SELECT */
        struct {
            int n;
            bool is_aggrgated;
            sql_agg_fn_t *agg_fn;
            qp_col_t *col_names;
            struct qp_node_ *child[1];
            glthread_t col_list_head;
        } select;


        /* QP_NODE_HAVING */
        struct {
            
            /* sum (salary) < 10000 */
            sql_agg_fn_t agg_fn;
            qp_col_t *col_name;
            sql_op_t op;
            operand_val_t op_val;
            struct qp_node_ *child[1];
        }having;


        /*QP_NODE_GROUP_BY*/
        struct {

            /* group by salary, dept */
            qp_col_t *col_name[SQL_MAX_GROUP_BY_N_SUPPORTED];
             struct qp_node_ *child[1];
        } groupby;



        /* QP_NODE_JOIN */
        struct
        {
            ctable_val_t *ctable_val1; /* Table on which scan need to be performed */
            ctable_val_t *ctable_val2; /* Table on which scan need to be performed */
            struct
            {
                /* t1.customer_id = t2.customer_id*/
                qp_col_t *col_name1;
                sql_op_t op;
                qp_col_t *col_name2;
                
            } join_predicate_t;
             struct qp_node_ *child[2];
        } join;


        /*QP_NODE_WHERE*/
        struct
        {
            /* sales < 10000 */
            qp_row_t *row; /* Row it has obtained from downstream node*/
            qp_col_t *column;
            sql_op_t op;
            operand_val_t op_val;
             struct qp_node_ *child[1];
        } where;

         /* QP_NODE_SEQ_SCAN */
        struct
        {
            ctable_val_t *ctable_val;      /* Table on which scan need to be performed */
            unsigned char *index_col_scan; /* Colmn name on which indexed scan need to be performed if possible*/
            /* Caching to fetch next record */
            BPlusTreeNode *bpnode;
            int index;
        } seq_scan;

    } u;


}qp_node_t;


typedef struct qp_response_ {

    qp_row_t *row1;
    qp_row_t *row2;
} qp_response_t;

typedef enum {

    QP_REQ_ID_INDEX_COLUMN_NAME,
    QP_REQ_ID_FETCH_MORE_REC,
    QP_REQ_ID_FETCH_REC_RESET

} qp_request_data_id_t;

typedef struct qp_request_data_ {

    qp_request_data_id_t req_data_id;
    union {
        int i;
         qp_col_t  qp_col_name[0];
    } u;
}qp_request_data_t;

#define QEP_F_GROUPBY   1

typedef struct qep_ {

    uint32_t flags;
    qp_node_t *root;

} qep_t;


qp_response_t  *
qep_request_downstream_node (qp_node_t *downstream_node, qp_request_data_t *qp_request);

void
qep_respond_upstream_node (qp_response_t *qp_resp);

qep_t*
select_qep_prepare_execution_plan (BPlusTree_t *tcatalog, ast_node_t *root);

void 
qep_destory (qep_t *qep);

void 
qep_execute (qep_t *qep);

#endif 