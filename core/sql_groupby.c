#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include "sql_utils.h"
#include "sql_where.h"
#include "qplanner.h"
#include "Catalog.h"
#include "../stack/stack.h"
#include "../Parsers/common.h"

void *
sql_compute_group_by_clause_keys (qep_struct_t *qep_struct,  joined_row_t  *joined_row) {

    int i;
    qp_col_t *qp_col;
    int key_size = 0;

    for (i = 0; i < qep_struct->groupby.n; i++) {

        qp_col = qep_struct->groupby.col_list[i];
        key_size += qp_col->schema_rec->dtype_size;    
    }

    unsigned char *key_out = (unsigned char *)calloc (key_size, 1);
    int offset = 0;
    void *key;

    for (i = 0; i < qep_struct->groupby.n; i++) {

        qp_col = qep_struct->groupby.col_list[i];
        key = sql_get_column_value_from_joined_row (joined_row, qp_col);
        assert(key);
        memcpy (key_out + offset, key, qp_col->schema_rec->dtype_size);
        offset += qp_col->schema_rec->dtype_size;
    }

    return (void *)key_out;
}

 void 
 sql_process_group_by (qep_struct_t *qep_struct) {

    
 }