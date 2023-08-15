#ifndef __WHERE__
#define __WHERE__

#include <stdbool.h>
#include "rdbms_struct.h"

typedef struct  joined_row_  joined_row_t;

typedef enum {

    WH_COL,
    WH_VALUE

} where_opd_type_t;

typedef struct where_operand_ {

    where_opd_type_t w_opd;

    union {

        qp_col_t col;

        struct {
            sql_dtype_t dtype;
            int size;           // size in bytes
            void *val;
        } value;

    } u;

} where_operand_t;

typedef struct where_cond_ {

    qp_col_t col;   // left operand
    sql_op_t op;   // mathematical operator < | > | = | !=
    where_operand_t right_op;

} where_cond_t;

bool 
sql_where_compute (where_cond_t *wc, joined_row_t *joined_row);

/* Expression Tree*/
typedef enum {

    LOG_OP,
    WHERE_COND

} expt_node_type_t;

typedef struct expt_node_ {

     expt_node_type_t  expt_node_type;

    union {

        sql_op_t lop;
        where_cond_t *wc;
    } u;

    struct expt_node_ *left;
    struct expt_node_ *right;

} expt_node_t;

bool 
sql_evaluate_where_expression_tree ( expt_node_t *root, joined_row_t *joined_row);

#endif 