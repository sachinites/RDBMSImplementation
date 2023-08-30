#ifndef __WHERE__
#define __WHERE__

#include <stdbool.h>
#include "rdbms_struct.h"

typedef struct  joined_row_  joined_row_t;
typedef struct qep_struct_ qep_struct_t ;

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
sql_where_compute (qep_struct_t *qep_struct, where_cond_t *wc, joined_row_t *joined_row);

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

typedef enum {

    WHERE_LITERAL_OPERATOR_TOKEN_CODE,  // represent ( , ), Logical Operators
    WHERE_LITERAL_WHERE_COND, // represent 1 single condition unit

} where_token_type_t;

typedef struct where_literal_ {

    where_token_type_t where_token_type;

    union {
        int token_id;
        where_cond_t wc;
    } u;

} where_literal_t;

void 
sql_debug_print_where_literals (where_literal_t *arr);

void 
sql_debug_print_where_literals2 (where_literal_t **arr, int size) ;

where_literal_t **
sql_where_clause_infix_to_postfix (where_literal_t *arr_in, int *size_out);

expt_node_t *
sql_where_convert_postfix_to_expression_tree (where_literal_t **arr, int size);

bool 
sql_evaluate_where_expression_tree (qep_struct_t *qep_struct, expt_node_t *root, joined_row_t *joined_row) ;

void 
expt_destroy(expt_node_t *root);

void 
sql_where_literals_array_free (where_literal_t *where_literal_arr) ;

void 
sql_debug_print_where_cond (where_cond_t *wc) ;

expt_node_t *
sql_where_convert_postfix_to_expression_tree (where_literal_t **wlit, int size);

void 
sql_debug_print_expression_tree_node (expt_node_t *root);

void 
sql_debug_print_expression_tree (expt_node_t *root) ;

#endif 