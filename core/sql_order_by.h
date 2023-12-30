#ifndef __ORDER_BY__
#define __ORDER_BY__

#include <stdbool.h>
typedef struct qep_struct_ qep_struct_t;
typedef struct BPlusTree BPlusTree_t;

bool 
qep_collect_dtypes_for_sorting (qep_struct_t *qep);

void 
qep_orderby_sort (qep_struct_t *qep) ;

bool
qep_order_by_reassign_select_columns (qep_struct_t *qep);

bool
sql_query_initialize_orderby_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) ;

#endif 