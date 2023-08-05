#ifndef __SQL_SELECT__
#define __SQL_SELECT__

typedef struct BPlusTree BPlusTree_t ;
typedef struct ast_node_ ast_node_t;

#include <stdbool.h>

void 
sql_process_select_query_internal (BPlusTree_t *schema_table, 
                                                         BPlusTree_t *data_table,
                                                         ast_node_t *root) ;

bool 
sql_validate_select_query_data (BPlusTree_t *schema_table, ast_node_t *root);

#endif 
