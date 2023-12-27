#include <stdlib.h>
#include "qep.h"
#include "SqlMexprIntf.h"
#include "rdbms_struct.h"

void 
qep_deinit (qep_struct_t *qep) {

    int i;
    qp_col_t *qp_col;

    if (qep->select.n) {

        for (i = 0; i < qep->select.n; i++) {

            qp_col = qep->select.sel_colmns[i];
            if (qp_col->sql_tree) {
                 sql_destroy_exp_tree (qp_col->sql_tree);
                qp_col->sql_tree = NULL;
            }           
        }
    }
}


void
sql_execute_qep (qep_struct_t *qep) {


}