#ifndef __QPLANNER__
#define __QPLANNER__

#include "../Tree/libtree.h"
#include "../Parsers/SQLParserStruct.h"
#include "sql_const.h"
#include "sql_where.h"

typedef struct catalog_table_value ctable_val_t ;
typedef struct schema_rec_ schema_rec_t ;
typedef struct ast_node_ ast_node_t;
typedef struct BPlusTree BPlusTree_t;
typedef  struct hashtable hashtable_t;
typedef struct stack Stack_t;

typedef enum qp_stage_id {

    QP_NODE_SEQ_SCAN,
    QP_NODE_WHERE,
    QP_NODE_JOIN,
    QP_NODE_JOIN_PREDICATE,
    QP_NODE_GROUP_BY,
    QP_NODE_HAVING,
    QP_NODE_SELECT,
    QP_NODE_DISTINCT,
    QP_NODE_ORDER_BY,
    QP_NODE_LIMIT,
    QP_NODE_OUTPUT,
    QP_NODE_MAX

} qp_stage_id_t;

typedef struct table_iter_data_ {

    BPlusTreeNode *bpnode;
    int index;
    ctable_val_t *ctable_val;
} table_iter_data_t;

typedef struct table_iterators_ {

    int table_cnt;
    table_iter_data_t table_iter_data[0];

} table_iterators_t;


typedef struct having_ {

    qp_col_t col;
    sql_op_t op;
    where_operand_t right_op;

} having_t;

typedef struct join_predicate_ {

    qp_col_t col1; /* some column from table1*/
    sql_op_t op;    /* Join Operator*/
    qp_col_t col2; /* some column from table2*/

} join_predicate_t;

typedef struct qep_struct_ {

    /* We cannot use more than three tables in our SQL query*/
    ctable_val_t **ctable_val;
    expt_node_t *expt_root;
   
    struct {

        int table_cnt;
    } join;

    struct {

        int n;
        qp_col_t **col_list;
        expt_node_t *expt_root;
        /* Collection of records based on group by fields*/
        hashtable_t *ht;
    } groupby;

    struct {

        int n;
        qp_col_t **sel_colmns;

    } select;

    struct {

        bool distinct;
        qp_col_t col;

    } distinct;

    struct {

        bool orderby;
        qp_col_t *col;
        /* AVL tree to store records ordered by 'order by'* field. Default is FCFS*/
        avltree_t avl_order_by_root;
    } orderby;

    uint32_t limit;  /* 0 means no limit*/

    /* other variables*/
    bool is_join_started;
    bool is_join_finished;
    table_iterators_t *titer;

} qep_struct_t;

void
qep_execute_select (qep_struct_t *qep_struct) ;

void
qep_execute_delete (qep_struct_t *qep_struct) ;

bool
qep_struct_init (qep_struct_t *qep_struct, BPlusTree_t *tcatalog, ast_node_t *root);

void 
qep_deinit (qep_struct_t *qep_struct);

#endif 