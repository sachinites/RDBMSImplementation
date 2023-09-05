#ifndef __SQL_GROUPBY__
#define __SQL_GROUPBY__

typedef struct  joined_row_  joined_row_t;
typedef struct qep_struct_ qep_struct_t ;

void *
sql_compute_group_by_clause_keys (qep_struct_t *qep_struct,  joined_row_t  *joined_row) ;

 void 
 sql_process_group_by (qep_struct_t *qep_struct) ;
 
#endif 