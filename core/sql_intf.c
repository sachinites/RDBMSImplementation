#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include <string.h>
#include <memory.h>
#include "../BPlusTreeLib/BPlusTree.h"
#include "sql_intf.h"
#include "Catalog.h"
#include "sql_const.h"
#include "select.h"
#include "../gluethread/glthread.h"

extern BPlusTree_t TableCatalogDef;

void 
sql_process_select_query (BPlusTree_t *TableCatalog, ast_node_t *root) {

    BPluskey_t bpkey;
    ctable_val_t *ctable_val;
    BPlusTree_t *tcatalog = TableCatalog ? TableCatalog : &TableCatalogDef;
    ast_node_t *table_name_node = root->child_list;
    unsigned char *table_name = table_name_node->u.identifier.identifier.name;

    bpkey.key =  table_name;
    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;

    ctable_val = (ctable_val_t *)BPlusTree_Query_Key (tcatalog, &bpkey);
    
    if (!ctable_val || !ctable_val->schema_table) {
        printf ("ERROR : relation does not exist\n");
        return;
    }

    BPlusTree_t *schema_table = ctable_val->schema_table;
    BPlusTree_t *data_table = ctable_val->rdbms_table;

    if (!sql_validate_select_query_data (schema_table, root)) {
        return ;
    }

    sql_process_select_query_internal (tcatalog , root);
}

bool 
sql_process_create_query (BPlusTree_t *TableCatalog, ast_node_t *root) {

    ast_node_t *table_name_node;

    if (Catalog_insert_new_table (TableCatalog , root)) {
        printf ("CREATE TABLE\n");
        return true;
    }

    printf ("ERROR:  relation already exist\n");
    return false;
}


static bool 
sql_validate_insert_query_data ( BPlusTree_t *TableCatalog, ast_node_t *root) {

    return true;
}

bool 
sql_process_insert_query (BPlusTree_t *TableCatalog, ast_node_t *root) {

    int i;
    list_node_t *lnode;
    glthread_t *curr;
    BPluskey_t bpkey, new_bpkey;
    ctable_val_t *ctable_val;
    BPlusTree_t *tcatalog = TableCatalog ? TableCatalog : &TableCatalogDef;
    ast_node_t *table_name_node = root->child_list;
    unsigned char *table_name = table_name_node->u.identifier.identifier.name;

    if (!sql_validate_insert_query_data (tcatalog, root)) {
        return false;
    }

    bpkey.key =  table_name;
    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;

    ctable_val = (ctable_val_t *)BPlusTree_Query_Key (tcatalog, &bpkey);
    assert (ctable_val);

    BPlusTree_t *schema_table = ctable_val->schema_table;
    BPlusTree_t *data_table = ctable_val->rdbms_table;
    glthread_t *col_list_head = &ctable_val->col_list_head;

    int key_size = 0;
    int rec_size = 0;
    schema_rec_t *rec;
    BPlusTreeNode *bnode;

    ITERATE_GLTHREAD_BEGIN (col_list_head, curr) {

        lnode = glue_to_list_node(curr);
        bpkey.key = lnode->data;
        bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
        rec = (schema_rec_t *) BPlusTree_Query_Key (schema_table, &bpkey);
        assert(rec);
        if (rec->is_primary_key) {
             key_size += rec->dtype_size;
        }
        rec_size += rec->dtype_size;
    }  ITERATE_GLTHREAD_END (col_list_head, curr)

    new_bpkey.key = calloc (1, key_size);
    new_bpkey.key_size = key_size;
    void *record = calloc(1, rec_size);
    ast_node_t *column_node = table_name_node->child_list;
    int key_offset = 0;

    /* Now fill the key content*/

 ITERATE_GLTHREAD_BEGIN (col_list_head, curr) {

            lnode = glue_to_list_node(curr);
            bpkey.key = lnode->data;
            bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
            rec = (schema_rec_t *) BPlusTree_Query_Key (schema_table, &bpkey);

            if (rec->is_primary_key) {
    
                switch (rec->dtype) {

                case SQL_STRING:
                    strncpy((char *)new_bpkey.key + key_offset,
                            column_node->u.identifier.identifier.name,
                            rec->dtype_size);
                    key_offset += rec->dtype_size;
                    break;
                case SQL_INT:
                case SQL_FLOAT:
                    memcpy((char *)new_bpkey.key + key_offset,
                           column_node->u.identifier.identifier.name,
                           rec->dtype_size);
                    key_offset += rec->dtype_size;
                    break;
                case SQL_IPV4_ADDR:
                    /* We store IP Address as integers*/
                    inet_pton(AF_INET,
                              (const char *)column_node->u.identifier.identifier.name,
                              (void *)((char *)new_bpkey.key + key_offset));
                    key_offset += rec->dtype_size;
                    break;
                default:
                    assert(0);
                }
            }
            /* Fill the record material*/
            switch (rec->dtype) {

            case SQL_STRING:
                strncpy((char *)record + rec->offset,
                        column_node->u.identifier.identifier.name,
                        rec->dtype_size);
                break;
            case SQL_INT:
            case SQL_FLOAT:
                memcpy((char *)record + rec->offset,
                       column_node->u.identifier.identifier.name,
                       rec->dtype_size);
                break;
            case SQL_IPV4_ADDR:
                /* We store IP Address as integers*/
                inet_pton(AF_INET,
                          (const char *)column_node->u.identifier.identifier.name,
                          (void *)((char *)record + rec->offset));
                break;
            default:
                assert(0);
            }
            column_node = column_node->next;
        } ITERATE_GLTHREAD_END (col_list_head, curr)

    /* Check for Duplication */
    if (BPlusTree_Query_Key (data_table, &new_bpkey)) {
        free (new_bpkey.key);
        free(record);
        printf ("ERROR:  duplicate key value violates unique constraint \"%s_pkey\"\n", table_name);
        return false;
    }

    /* Now key and records are ready, insert it into data table*/
    bool rc =  (BPlusTree_Insert (data_table, &new_bpkey, record));

    if (!rc) {
        free (new_bpkey.key);
        free(record);
        printf ("Record Insertion Failed\n");
        return false;
    }

    printf ("INSERT 0 1\n");
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
    printf (" Schema    |           Name           | Type  | Owner  \n");
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

