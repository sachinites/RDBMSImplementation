#include <string.h>
#include <list>
#include "sql_delete.h"
#include "Catalog.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "rdbms_struct.h"
#include "SqlMexprIntf.h"
#include "qep.h"

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

void 
sql_process_delete_query (qep_struct_t *qep) {

    uint32_t count;
    uint32_t inc_count = 0;

    BPluskey_t *bpkey;
    BPluskey_t *bpkey_cpy;
    std::list<BPluskey_t *> key_lst;

    while (qep_execute_join (qep)) {

        bpkey = qep->joined_row_tmplate->key_array[0];
        bpkey_cpy = (BPluskey_t *) calloc (1, sizeof (BPluskey_t));
        bpkey_cpy->key = (void *)calloc (1, bpkey->key_size);
        bpkey_cpy->key_size = bpkey->key_size;
        memcpy (bpkey_cpy->key, bpkey->key, bpkey->key_size);
        key_lst.push_back (bpkey_cpy);
    }

    count = key_lst.size();

    while (!key_lst.empty()) {

        bpkey = key_lst.front();
        key_lst.pop_front();
        if (BPlusTree_Delete (qep->join.tables[0].ctable_val->rdbms_table, bpkey)) {
            inc_count++;
        }
        free(bpkey->key);
        free(bpkey);
    }

    printf ("DELETE %u\n", inc_count);
    if (inc_count != count) {
        printf ("Error : %d rows could not be deleted\n", count - inc_count);
    }
}