#ifndef __QPLANNER__
#define __QPLANNER__

#include "../Parsers/SQLParserStruct.h"
#include "sql_const.h"
#include "rdbms_struct.h"


typedef struct catalog_table_value ctable_val_t ;
typedef struct schema_rec_ schema_rec_t ;
typedef struct ast_node_ ast_node_t;
typedef struct BPlusTree BPlusTree_t;
typedef  struct hashtable hashtable_t;

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

typedef struct table_iterators_ {

    BPlusTreeNode *bpnode[3];
    int index[3];
    ctable_val_t *ctable_val[3];
} table_iterators_t;

typedef struct joined_row_ {

    BPlusTree_t *schema_table[3];
    void *rec[3];

} joined_row_t;


/* Data structure to store where conditions on a single table.
    Current let us support only 1 conditon */
typedef struct where_ {

    qp_col_t col;
    sql_op_t op;
    operand_val_t op_val;

} where_t;

typedef struct having_ {

    qp_col_t col;
    sql_op_t op;
    operand_val_t op_val;

} having_t;

typedef struct join_predicate_ {

    qp_col_t col1; /* some column from table1*/
    sql_op_t op;    /* Join Operator*/
    qp_col_t col2; /* some column from table2*/

} join_predicate_t;



typedef struct qep_struct_ {

    /* We cannot use more than three tables in our SQL query*/
    ctable_val_t *ctable_val1;
    where_t *ctable_val_cond1;
    ctable_val_t *ctable_val2;
    where_t *ctable_val_cond2;
    ctable_val_t *ctable_val3;
    where_t *ctable_val_cond3;
   
    struct {

        join_predicate_t *join_pred[2];
         bool join_pred_or;  /* join predicates is joined by OR is true, else AND*/
    } join;

    struct {

        int n;
        qp_col_t **col_list;

    } groupby;

    having_t *having[2];
    bool having_pred_or; 

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
        qp_col_t col;
    } orderby;

    uint32_t limit;  /* 0 means no limit*/

    /* other variables*/
    table_iterators_t titer;
    bool is_join_started;
    hashtable_t *ht;

} qep_struct_t;

void
qep_execute (qep_struct_t *qep_struct) ;

void 
qep_struct_init (qep_struct_t *qep_struct, BPlusTree_t *tcatalog, ast_node_t *root);

void 
qep_deinit (qep_struct_t *qep_struct);

#endif 