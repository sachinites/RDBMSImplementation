#ifndef __SQL_GROUP_BY__
#define __SQL_GROUP_BY__

typedef struct qep_struct_ qep_struct_t;
typedef struct BPlusTree BPlusTree_t;

void 
 sql_group_by_clause_group_records (qep_struct_t *qep) ;

void 
sql_process_group_by_grouped_records (qep_struct_t *qep) ;

bool
sql_query_initialize_groupby_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) ;

bool
sql_query_initialize_having_clause (qep_struct_t *qep, BPlusTree_t *tcatalog);

#endif 