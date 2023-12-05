#ifndef __SQL_DELETE__
#define __SQL_DELETE__

typedef struct qep_struct_ qep_struct_t;

void
sql_drop_table (char *table_name);

void 
sql_process_delete_query (qep_struct_t *qep);

#endif