#include <stddef.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "sql_const.h"
#include "rdbms_struct.h"
#include "Catalog.h"
#include "sql_io.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "sql_utils.h"
#include "sql_mexpr_intf.h"

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
sql_print_hdr (qp_col_t **col_list, int n_cols ) {

    int i;
    qp_col_t *col;
    int num_columns = n_cols;

    int column_width = 20; // Default column width
    if (num_columns > 0) {
        column_width = 80 / num_columns; // Adjust column width based on available space
    }

    // Print the top line
    print_line(num_columns, column_width);

    // Print the header row with column names
    for (i = 0; i < n_cols; i++) {
        col = col_list[i];
        printf("%-*s|", column_width, col->alias_name);
    }

    printf("\n");

    // Print the line below the header
    print_line(num_columns, column_width);
}


void sql_emit_select_output(int n_col,
                                              qp_col_t **col_list_head) {

    int i;
    qp_col_t *qp_col;
    sql_dtype_t dtype;
    int num_columns = n_col;

    int column_width = 20; // Default column width
    if (num_columns > 0) {
        column_width = 80 / num_columns; // Adjust column width based on available space
    }

    // Print each row of data
    for (i = 0; i < n_col; i++) {
        qp_col = col_list_head[i];
        mexpr_var_t *val = &qp_col->computed_value;

        if (qp_col->agg_fn == SQL_COUNT) {
            printf("%-*d|", column_width, *(int *)val);
            continue;
        }

        dtype = val->dtype;

        switch (dtype) {
            case MEXPR_DTYPE_STRING:
                printf("%-*s|", column_width, val->u.str_val);
                break;
            case MEXPR_DTYPE_INT:
                printf("%-*d|", column_width, val->u.int_val);
                break;
            case MEXPR_DTYPE_DOUBLE:
                if (mexpr_double_is_integer (val->u.d_val)) {
                    printf("%-*d|", column_width, val->u.int_val);
                }
                else {
                    printf("%-*f|", column_width, val->u.d_val);
                }
                break;
            case MEXPR_DTYPE_BOOL:
                assert(0);
                break;
            #if 0
            case SQL_IPV4_ADDR: {
                assert(0);
                unsigned char ipv4_addr_str[16];
                inet_ntop(AF_INET, val->u., ipv4_addr_str, 16);
                printf("%-*s|", column_width, ipv4_addr_str);
                break;
            }
            #endif
            default:
                assert(0);
        }
    }
    printf("\n");
}
