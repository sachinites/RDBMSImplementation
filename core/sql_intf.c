#include <stdio.h>
#include <stdlib.h>
#include "../BPlusTreeLib/BPlusTree.h"
#include "sql_intf.h"
#include "Catalog.h"
#include "sql_const.h"

extern BPlusTree_t TableCatalogDef;

bool 
sql_process_create_query (BPlusTree_t *TableCatalog, ast_node_t *root) {

    return Catalog_insert_new_table (TableCatalog , root);
}

void 
sql_show_table_catalog (BPlusTree_t *TableCatalog) {

    int i;
    BPluskey_t *key;
    BPlusTreeNode *bnode;
    unsigned char table_name[SQL_TABLE_NAME_MAX_SIZE];
    
    BPlusTree_t *tcatalog = TableCatalog ? TableCatalog : &TableCatalogDef;

    bnode = tcatalog->Root;
    
    if (!bnode) return;

    while (!bnode->isLeaf) {
        bnode = bnode->child[0];
    }

    while (bnode != NULL) {
        
        for (i = 0; i < bnode->key_num; i++) {

            key = &bnode->key[i];

            tcatalog->key_fmt_fn (key, table_name, SQL_TABLE_NAME_MAX_SIZE);
            
            printf (" %s\n", table_name);
        }
        bnode = bnode->next;
    }
    return;
}

