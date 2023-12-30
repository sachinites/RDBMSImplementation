#ifndef __SQL_JOIN__
#define __SQL_JOIN__

#include <stdbool.h>

typedef struct qep_struct_ qep_struct_t;
typedef struct BPlusTree BPlusTree_t;

bool 
sql_query_initialize_join_clause  (qep_struct_t *qep, BPlusTree_t *tcatalog);

#endif 