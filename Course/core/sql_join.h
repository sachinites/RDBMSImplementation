#ifndef __SQL_JOIN__
#define __SQL_JOIN__

#include <stdbool.h>

typedef struct qep_struct_ qep_struct_t;
typedef struct BPlusTree BPlusTree_t;
typedef struct BPlusTreeNode BPlusTreeNode;
typedef struct catalog_table_value ctable_val_t ;

typedef struct table_iter_data_ {

    BPlusTreeNode *bpnode;
    int index;
    ctable_val_t *ctable_val;

} table_iter_data_t;

typedef struct table_iterators_ {

    int table_cnt;
    table_iter_data_t table_iter_data[0];
    
} table_iterators_t;


bool 
sql_query_initialize_join_clause  (qep_struct_t *qep, BPlusTree_t *tcatalog);

bool
qep_execute_join(qep_struct_t *qep) ;

void 
table_iterators_init (qep_struct_t *qep,
                                table_iterators_t **_titer) ;

#endif 