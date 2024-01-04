#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include "rdbms_struct.h"
#include "qep.h"
#include "../../BPlusTreeLib/BPlusTree.h"
#include "SqlMexprIntf.h"
#include "sql_join.h"
#include "sql_io.h"

bool 
sql_query_initialize_select_column_list  (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    int i;

    for (i = 0; i < qep->select.n; i++) {

        sql_tree_expand_all_aliases (qep, qep->select.sel_colmns[i]->sql_tree);
        sql_tree_operand_names_to_fqcn (qep, qep->select.sel_colmns[i]->sql_tree);
        if (!sql_resolve_exptree (tcatalog, 
                                                qep->select.sel_colmns[i]->sql_tree,
                                                qep, &qep->joined_row_tmplate)) {
            
            printf ("Error : Failed to resolve Expression Tree for %dth select column\n", i);
            return false;
        }
    }    

    return true;
}

void
sql_process_select_query (qep_struct_t *qep) {

    int i;
    int row_no = 0; 
    qp_col_t *sqp_col;

    while (qep_execute_join(qep)) {

        for (i = 0; i < qep->select.n; i++) {

            sqp_col = qep->select.sel_colmns[i];

            if (sqp_col->computed_value ) {

                sql_destroy_Dtype_value_holder (sqp_col->computed_value);
                sqp_col->computed_value = NULL;
            }
            sqp_col->computed_value = sql_evaluate_exp_tree (sqp_col->sql_tree);  
        }

        row_no++;
        
        if (row_no == 1) {
            sql_print_hdr (qep, qep->select.sel_colmns, qep->select.n);
        }

        sql_emit_select_output (qep, qep->select.n, qep->select.sel_colmns);

    } //  join while loop ends
}