#include <stdbool.h>
#include <memory.h>
#include "../BPlusTreeLib/BPlusTree.h"
#include "../core/rdbms_struct.h"

extern int 
rdbms_key_comp_fn (char *key1, char *key2, key_mdata_t *key_mdata, int size);

BPlusTree_t TableCatalog;

static bool initialized = false;

bool 
Catalog_insert_new_table (char *table_name) {

}
