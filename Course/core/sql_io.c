#include <stddef.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "sql_const.h"
#include "rdbms_struct.h"
#include "qep.h"
#include "Catalog.h"
#include "sql_io.h"
#include "SqlMexprIntf.h"

#define SCREEN_WIDTH    80
#define COLUMN_WIDTH   20

static void 
print_line(int num_columns, int column_width) {
    for (int i = 0; i < num_columns; i++) {
        for (int j = 0; j < column_width; j++) {
            putchar('-');
        }
        putchar('+');
    }
    putchar('\n');
}

void 
sql_print_hdr (qep_struct_t *qep, qp_col_t **col_list, int n_cols ) {

/*
    If select column is single-operand resolution tree, then Hdr would be :
        Alias if available
        FQCN 

    If select column is multi-operand resolution tree, then Hdr would be : 
        Alias if available
        Leave blank
    */

    int i;
    qp_col_t *qp_col;

   int column_width = SCREEN_WIDTH / n_cols;

   print_line (n_cols, column_width);

    for (i = 0; i < n_cols; i++) {

        qp_col = col_list[i];

        if (sql_is_single_operand_expression_tree (qp_col->sql_tree)) {

            if (qp_col->alias_provided_by_user) {

                 printf("%-*s|", column_width, qp_col->alias_name);
            }
            else {

                printf("%-*s|", column_width, 
                    sql_get_opnd_variable_name(sql_tree_get_root(qp_col->sql_tree)).c_str());
            }
        }

        else {

            if (qp_col->alias_provided_by_user) {

                    printf("%-*s|", column_width, qp_col->alias_name);
                }
            else {

                printf("%-*s|", column_width, "");
            }
        }
    }
    printf("\n");
    print_line (n_cols, column_width);
}

void sql_emit_select_output(qep_struct_t *qep,
                                               int n_col,
                                               qp_col_t **col_list_head) {

    int i;
    Dtype *dtype;
    qp_col_t *qp_col;
    int num_columns = n_col;

    int column_width =  SCREEN_WIDTH / num_columns;

    dtype_value_t dtype_value;

    // Print each row of data
    for (i = 0; i < n_col; i++) {

        qp_col = col_list_head[i];
        dtype =  qp_col->computed_value;

        dtype_value = DTYPE_GET_VALUE(dtype);

        switch (dtype_value.dtype) {

            case SQL_STRING:
                printf("%-*s|", column_width, dtype_value.u.str_val);
                break;

            case SQL_INT:
                printf("%-*d|", column_width, dtype_value.u.int_val);
                break;

            case SQL_DOUBLE:
                printf("%-*f|", column_width, dtype_value.u.d_val);
                break;

            default:
                assert(0);
        }
    }
    printf("\n");
    print_line(num_columns, column_width);
}
