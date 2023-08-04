#ifndef __SQL_INTF__
#define SQL_INTF__

#include <stdbool.h>
#include "../Parsers/Ast.h"

typedef struct BPlusTree BPlusTree_t ;

bool 
sql_process_create_query (BPlusTree_t *TableCatalog, ast_node_t *root);

bool 
sql_process_insert_query (BPlusTree_t *TableCatalog, ast_node_t *root);

void 
sql_show_table_catalog (BPlusTree_t *TableCatalog);

void 
sql_print_record (BPlusTree_t *TableSchema, unsigned char *record, bool hr);

#endif 