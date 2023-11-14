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
#include "sql_utils.h"
#include "SqlMexprIntf.h"
#include "../../MathExpressionParser/Dtype.h"

#define SCREEN_WIDTH    80
#define COLUMN_WIDTH   20

extern BPlusTree_t TableCatalogDef;


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

    int i;
    qp_col_t *col;
    int num_columns = n_cols;

    char table_name[SQL_TABLE_NAME_MAX_SIZE];
    char column_name[SQL_COLUMN_NAME_MAX_SIZE];
    char composite_col_name[SQL_COMPOSITE_COLUMN_NAME_SIZE];

    int column_width = COLUMN_WIDTH; // Default column width
    
    if (qep->select.sql_record_reader) {
        return;
    }

    if (num_columns > 0) {
        column_width = SCREEN_WIDTH / num_columns; // Adjust column width based on available space
    }

    // Print the top line
    print_line(num_columns, column_width);

    for (i = 0; i < n_cols; i++) {

        col = col_list[i];

        if (qep->join.table_cnt > 1) {
        
            printf("%-*s|", column_width, col->alias_name);
        }
        else {
        
            parser_split_table_column_name (qep->join.table_alias,
                                                                   &TableCatalogDef, 
                                                                   col->alias_name,
                                                                   table_name,
                                                                   column_name );
            printf("%-*s|", column_width, column_name);
        }
    }

    printf("\n");

    // Print the line below the header
    print_line(num_columns, column_width);
}


void sql_emit_select_output(qep_struct_t *qep,
                                               int n_col,
                                               qp_col_t **col_list_head) {

    int i;
    Dtype *dtype;
    qp_col_t *qp_col;
    int num_columns = n_col;
    std::vector<Dtype *> dtype_vector;

    if (qep->select.sql_record_reader) {

        
        for (i = 0; i < n_col; i++) {
            
            qp_col = col_list_head[i];
            dtype = (qp_col->agg_fn != SQL_AGG_FN_NONE ) ? \
                                    sql_column_get_aggregated_value (qp_col) :  \
                                    qp_col->computed_value;
            dtype_vector.push_back (dtype);
        }
        qep->select.sql_record_reader (qep->select.app_data, &dtype_vector);
        dtype_vector.clear();
        return;
    }


    int column_width = COLUMN_WIDTH; // Default column width

    if (num_columns > 0) {
        column_width = SCREEN_WIDTH / num_columns; // Adjust column width based on available space
    }

    // Print each row of data
    for (i = 0; i < n_col; i++) {

        qp_col = col_list_head[i];
        Dtype *dtype = (qp_col->agg_fn != SQL_AGG_FN_NONE ) ? \
                                    sql_column_get_aggregated_value (qp_col) :  \
                                    qp_col->computed_value;

        switch (dtype->did) {

            case MATH_CPP_STRING:
                printf("%-*s|", column_width, (dynamic_cast <Dtype_STRING*>(dtype))->dtype.str_val.c_str());
                break;

            case MATH_CPP_INT:
                printf("%-*d|", column_width, (dynamic_cast <Dtype_INT *>(dtype))->dtype.int_val);
                break;

            case MATH_CPP_DOUBLE:
                if (mexpr_double_is_integer ( (dynamic_cast <Dtype_DOUBLE *>(dtype))->dtype.d_val)) {
                    printf("%-*d|", column_width, (int)(dynamic_cast <Dtype_DOUBLE *>(dtype))->dtype.d_val);
                }
                else {
                    printf("%-*f|", column_width, (dynamic_cast <Dtype_DOUBLE *>(dtype))->dtype.d_val);
                }
                break;

            case MATH_CPP_BOOL:
                assert(0);
                break;
                
            case MATH_CPP_IPV4: {
                printf("%-*s|", column_width, (dynamic_cast <Dtype_IPv4_addr*>(dtype))->dtype.ip_addr_str.c_str());
                break;
            }

            case MATH_CPP_INTERVAL: 
            {
                Dtype_STRING *dtype_string = dtype->toString();
                std::string str = dtype_string->dtype.str_val;
                printf("%-*s|", column_width, str.c_str());
                delete dtype_string;
                break;
            }
            break;
            default:
                assert(0);
        }
    }
    printf("\n");
}
