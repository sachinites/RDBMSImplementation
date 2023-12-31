#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include "rdbms_struct.h"
#include "qep.h"
#include "../../BPlusTreeLib/BPlusTree.h"
#include "sql_join.h"


bool 
sql_query_initialize_select_column_list  (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    return true;
}

static void 
debug_print_joined_row (joined_row_t *joined_row) {

    int i;
    void *rec;

    for (i = 0; i < joined_row->size; i++) {

        rec = joined_row->rec_array[i];
        printf ("%s ", (char *)rec);
    }
}


void
sql_process_select_query (qep_struct_t *qep) {

    while (qep_execute_join(qep)) {

        // Access qep->joined_row_template
        debug_print_joined_row (qep->joined_row_tmplate);
        printf("\n");
    }
}