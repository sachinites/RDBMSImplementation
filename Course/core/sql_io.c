#include <stddef.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <vector>
#include "sql_const.h"
#include "rdbms_struct.h"
#include "qep.h"
#include "Catalog.h"
#include "sql_io.h"
#include "../BPlusTreeLib/BPlusTree.h"
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

void sql_emit_select_output(qep_struct_t *qep,
                                               int n_col,
                                               qp_col_t **col_list_head) {

    int i;
    Dtype *dtype;
    qp_col_t *qp_col;
    int num_columns = n_col;
    std::vector<Dtype *> dtype_vector;

    int column_width = COLUMN_WIDTH; // Default column width

    if (num_columns > 0) {
        column_width = SCREEN_WIDTH / num_columns; // Adjust column width based on available space
    }

    print_line(num_columns, COLUMN_WIDTH);

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
}
