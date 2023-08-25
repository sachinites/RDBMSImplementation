#ifndef __SQL_DELETE__
#define __SQL_DELETE__

typedef struct BPlusTree BPlusTree_t ;
typedef struct ast_node_ ast_node_t;

#include <stdbool.h>

void 
sql_process_delete_query_internal (BPlusTree_t *TableCatalog, ast_node_t *delete_root);

bool 
sql_validate_delete_query_data (BPlusTree_t *schema_table, ast_node_t *delete_root);

#endif