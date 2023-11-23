#include <string.h>
#include "sql_delete.h"
#include "Catalog.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "sql_const.h"
#include "rdbms_struct.h"

extern BPlusTree_t TableCatalogDef;

void
sql_drop_table (char *table_name) {

    BPluskey_t bkey;
    catalog_table_key_t catalog_table_key;

    if (!sql_catalog_table_lookup_by_table_name (&TableCatalogDef, table_name)) {
        printf ("Error : Table does not exist\n");
        return;
    }

     catalog_table_key.scope = PUBLIC;
     strncpy (catalog_table_key.entity_name,
                  table_name,
                  sizeof (catalog_table_key.entity_name));
     catalog_table_key.type = TABLE;
     strncpy (catalog_table_key.owner, "postgres", sizeof (catalog_table_key.owner));
     bkey.key = (void *)&catalog_table_key;
     bkey.key_size = sizeof (catalog_table_key_t);
     BPlusTree_Delete (&TableCatalogDef, &bkey);
}