#include <stdio.h>
#include <stdlib.h>
#include "../BPlusTreeLib/BPlusTree.h"
#include "sql_intf.h"
#include "Catalog.h"
#include "sql_const.h"

extern BPlusTree_t TableCatalogDef;

bool 
sql_process_create_query (BPlusTree_t *TableCatalog, ast_node_t *root) {

    if (Catalog_insert_new_table (TableCatalog , root)) {
        printf ("CREATE TABLE\n");
    }
}

bool 
sql_process_insert_query (BPlusTree_t *TableCatalog, ast_node_t *root) {

    return true;
}

void 
sql_show_table_catalog (BPlusTree_t *TableCatalog) {

    int i;
    int rows = 0;
    BPluskey_t *key;
    BPlusTreeNode *bnode;
    unsigned char table_name[SQL_TABLE_NAME_MAX_SIZE];
    
    BPlusTree_t *tcatalog = TableCatalog ? TableCatalog : &TableCatalogDef;

    bnode = tcatalog->Root;
    
    if (!bnode) return;

    while (!bnode->isLeaf) {
        bnode = bnode->child[0];
    }

    if (!bnode) return;

    printf ("           List of relations\n");
    printf (" Schema    |           Name           | Type   | owner  \n");
    printf ("-----------+--------------------------+-------+--------------\n");

    while (bnode) {
        
        for (i = 0; i < bnode->key_num; i++) {

            key = &bnode->key[i];

            tcatalog->key_fmt_fn (key, table_name, SQL_TABLE_NAME_MAX_SIZE);
            
            printf (" public    | %-23s  | table | postgres  \n", table_name);
            if (bnode->next) {
                printf ("-----------+--------------------------+-------+--------------\n");
            }
            rows++;
        }

        bnode = bnode->next;
    }
    printf ("(%d rows)\n", rows);
}

