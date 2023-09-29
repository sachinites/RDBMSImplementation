#include "sql_create.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "Catalog.h"

extern BPlusTree_t TableCatalogDef;

void 
sql_create_data_destroy (sql_create_data_t *cdata) {

}

 void 
 sql_process_create_query (sql_create_data_t *cdata) {

    Catalog_insert_new_table (&TableCatalogDef, cdata);
 }