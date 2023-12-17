#include <stdio.h>
#include "sql_delete.h"
#include "Catalog.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "sql_const.h"

extern BPlusTree_t TableCatalogDef;

void
sql_drop_table (char *table_name) {

    BPluskey_t bpkey;
    
    bpkey.key = (void *)table_name;
    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;

    if (!BPlusTree_Query_Key (&TableCatalogDef, &bpkey)) {
        printf ("Error : Table does not exist\n");
        return;
    }

     if (BPlusTree_Delete (&TableCatalogDef, &bpkey)) {
        printf ("DROP TABLE\n");
     }
}
