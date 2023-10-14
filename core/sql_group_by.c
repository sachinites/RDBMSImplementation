#include "qep.h"
#include "sql_group_by.h"
#include "SqlMexprIntf.h"

/* We have just got the first jouned row from Table(s) which has
     qualified the where clause. 

     From the first joined row, we will compute the key size required to
     setup hashtable. The first joined row serves as trigger to setup group by
     hashtable.    
*/

static void *
sql_group_by_compute_ht_key_from_joined_row (
                    qep_struct_t *qep,
                    joined_row_t *joined_row,
                    int *sizeout) {

    void *ht_key_buffer = NULL;

    *sizeout = 0;

    if (qep){}
    return NULL;
}


void 
sql_setup_group_by_hashtable (qep_struct_t *qep) {
    
    int i;
    char *ht_key;  
    Dtype *dtype;
    qp_col_t *qp_col;
    int ht_key_size = 0;

    if (qep->groupby.ht) return;

    ht_key = (char *)calloc (1024, 1);

    for (i = 0; i < qep->groupby.n; i++) {

        qp_col = qep->groupby.col_list[i];
        dtype = sql_evaluate_exp_tree (qp_col->sql_tree);
        ht_key_size +=   sql_dtype_serialize (dtype, ht_key);
    }
}