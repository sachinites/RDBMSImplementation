#ifndef __WHERE__
#define __WHERE__

#include <stdbool.h>
#include "rdbms_struct.h"

typedef struct  joined_row_  joined_row_t;

/* Type of operand */
typedef enum {

    WH_COL,
    WH_VAUE,
    LOG_OP

} wh_opd_type_t ;

/* Structure to represent where operand, which could be col name,
    fixed value or logical operator (internal node of expression tree )*/
typedef struct wh_opd_ {

    wh_opd_type_t wh_opd_type;

    union {

        qp_col_t col;

        struct {
            sql_dtype_t dtype;
            int size;           // size in bytes
            void *val;
        } value;

        sql_op_t op;  // logical operatpr AND OR NOT

    } u;

    struct wh_opd_ *left;
    struct wh_opd_ *right;

} wh_opd_t;


typedef struct where_cond_ {

    qp_col_t col;   // left operand
    sql_op_t op;   // mathematical operator < | > | = | !=
    wh_opd_t right_op;

} where_cond_t;

bool 
sql_where_compute (where_cond_t *wc, joined_row_t *joined_row);

#endif 