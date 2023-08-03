#include <stdbool.h>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include "../BPlusTreeLib/BPlusTree.h"
#include "../core/rdbms_struct.h"
#include "../core/sql_const.h"
#include "../Parsers/Ast.h"
#include "../core/sql_utils.h"
#include "Catalog.h"

/* Global Default Catalog Table, one per Database */
BPlusTree_t TableCatalogDef;
static bool initialized = false;

/* A fn used to free the 'value' of catalog table*/
static void 
catalog_table_free_fn (void *ptr) {

    ctable_val_t *ctable_val = (ctable_val_t *)ptr;
    BPlusTree_Destroy (ctable_val->schema_table);
    BPlusTree_Destroy (ctable_val->rdbms_table);
    free(ptr);
}

static void 
schema_table_record_free (void *ptr) {

    schema_rec_t *schema_rec = (schema_rec_t *)ptr;
    free (schema_rec);
}

bool 
Catalog_insert_new_table (BPlusTree_t *catalog_table, ast_node_t *root) {

    int n, i;
    BPluskey_t bkey;
    ast_node_t *ast_node;
    BPluskey_t **bkeys;
    schema_rec_t **crecords;
    ast_node_t astnode_tmplate;

    if (!catalog_table) {
        catalog_table = &TableCatalogDef;
    }

    if (!initialized) {
        /* If this is the first table we are creating in a default DB, then initialize the catalog table. Catalog table is the collection of all tables in single DB along with their schema details*/
        BPlusTree_init(catalog_table,
                       rdbms_key_comp_fn,
                       BPlusTree_key_format_fn_default,
                       NULL,
                       SQL_BTREE_MAX_CHILDREN_CATALOG_TABLE, 
                       catalog_table_free_fn);

        static key_mdata_t key_mdata[] = {  {SQL_STRING, SQL_TABLE_NAME_MAX_SIZE} };
        catalog_table->key_mdata = key_mdata;
        catalog_table->key_mdata_size = 1;
        initialized = true;
    }

    /* Prepare the key to be inserted in the catalog table*/
    char *tble_name = (char *)calloc (1, SQL_TABLE_NAME_MAX_SIZE);
    memset (&astnode_tmplate, 0, sizeof (astnode_tmplate));
    astnode_tmplate.entity_type = SQL_IDENTIFIER;
    astnode_tmplate.u.identifier.ident_type = SQL_TABLE_NAME;
    ast_node = ast_find (root, &astnode_tmplate);
    assert (ast_node);
    strncpy (tble_name, ast_node->u.identifier.identifier.name, SQL_TABLE_NAME_MAX_SIZE);
    bkey.key = (void *)tble_name;
    bkey.key_size = SQL_TABLE_NAME_MAX_SIZE;

    /* Let us create a VAUE for catalog table, so that we can attempt to do insertion of this record as early as possible in catalog table before creating other data structures. This would help us to rewind back if there is any error*/
    ctable_val_t *ctable_val = (ctable_val_t *)calloc (1, sizeof (ctable_val_t));
    ctable_val->schema_table = NULL;
    ctable_val->rdbms_table = NULL;

     if (!BPlusTree_Insert (catalog_table, &bkey, (void *)ctable_val)) {

        printf ("Error : Table Already Exist\n");
        free (ctable_val);
        free(bkey.key);
        return false;
     }

    /* Now create a Schema table for this new table. Schema table stores all the attributes and details of a RDBMS table. Every RDBMS table has a schema table*/

    BPlusTree_t *schema_table = (BPlusTree_t *)calloc (1, sizeof (BPlusTree_t));
    BPlusTree_init (schema_table, 
                               rdbms_key_comp_fn,
                               BPlusTree_key_format_fn_default , 
                               NULL, 
                               SQL_BTREE_MAX_CHILDREN_SCHEMA_TABLE, 
                               schema_table_record_free);

    static key_mdata_t key_mdata[] = {{SQL_STRING, SQL_COLUMN_NAME_MAX_SIZE}};
    catalog_table->key_mdata = key_mdata;
    catalog_table->key_mdata_size = 1;

    /* Schema table has been created, now insert records in it. Each record is of the type : 
       key::  <column name>   value :: <catalog_rec_t >  */

     n = Catalog_create_schema_table_records (root, &bkeys,  &crecords);

     if (n == 0) {

        BPlusTree_Destroy (schema_table);
        printf ("Error : Schema Table Creation Failed\n");
        return false;
     }

     for (i = 0; i < n; i++) {

        assert (BPlusTree_Insert (schema_table, bkeys[i], (void *)crecords[i]));
        /* insrt create a copy of the keys, so free what we have*/
        free(bkeys[i]);
     }

    /* Cleanup the temporary arrays*/
    free(bkeys);
    free(crecords);


    /* Now make the actual rdbms table to hold records */
    BPlusTree_t *table = (BPlusTree_t *)calloc (1, sizeof (BPlusTree_t));
    BPlusTree_init (table, 
                               rdbms_key_comp_fn,
                               NULL, NULL, 
                               SQL_BTREE_MAX_CHILDREN_RDBMS_TABLE, free);

    /* Construct key meta data for this Table Schema*/
    int key_mdata_size2;
    key_mdata_t *key_mdata2 = sql_construct_table_key_mdata (root, &key_mdata_size2);

    if (!key_mdata2) {
        BPlusTree_Destroy (table);
        printf ("Error : Table Must have atleast one primary key\n");
        return false;
    }

    table->key_mdata = key_mdata2;
    table->key_mdata_size = key_mdata_size2;

    /* Now store the Schema Table and RDBMS table as VALUE of the catalog table*/
    ctable_val->schema_table = schema_table;
    ctable_val->rdbms_table = table;

    return true;
}

int
Catalog_create_schema_table_records (ast_node_t *root,  
                                                                BPluskey_t ***bkeys,
                                                                schema_rec_t ***crecords) {

    int i;
    int col_count = 0;
    ast_node_t *attr_node;
    ast_node_t *col_node;
    ast_node_t *identifier_node;
    ast_node_t *col_dtype_node;
    ast_node_t astnode_tmplate;
    ast_node_t *table_name_node;

    bkeys = NULL;
    crecords = NULL;

    switch (root->entity_type) {

        case SQL_QUERY_TYPE:

            switch (root->u.q_type) {

                case SQL_CREATE_Q:

                    memset (&astnode_tmplate, 0, sizeof (ast_node_t ));
                    astnode_tmplate.entity_type = SQL_IDENTIFIER;
                    astnode_tmplate.u.identifier.ident_type = SQL_TABLE_NAME;
                    table_name_node = ast_find (root, &astnode_tmplate);
                    assert (table_name_node);
                    col_count = 0;

                    FOR_ALL_AST_CHILD(table_name_node, col_node) {

                        col_count++;
                    }

                    if (!col_count) return 0;

                    *bkeys = (BPluskey_t **)calloc (col_count , sizeof (BPluskey_t *));
                    *crecords = (schema_rec_t **)calloc (col_count , sizeof ( schema_rec_t *));

                    i = 0;
                    FOR_ALL_AST_CHILD(table_name_node, col_node) {

                        (*bkeys)[i] = (BPluskey_t *)calloc (1, sizeof (BPluskey_t));
                        (*bkeys[i])->key = (void *)calloc (1, SQL_COLUMN_NAME_MAX_SIZE);
                        strncpy ((*bkeys[i])->key, col_node->u.identifier.identifier.name, SQL_COLUMN_NAME_MAX_SIZE);
                        (*bkeys[i])->key_size = SQL_COLUMN_NAME_MAX_SIZE;

                        (*crecords)[i] =  (schema_rec_t *)calloc (1 , sizeof ( schema_rec_t ));

                        col_dtype_node = col_node->child_list;
                        assert (col_dtype_node->entity_type == SQL_DTYPE);

                        (*crecords)[i]->dtype = col_dtype_node->u.dtype;

                        (*crecords [i])->dtype_size = 1;

                         FOR_ALL_AST_CHILD(col_dtype_node, attr_node) {

                            assert (attr_node->entity_type == SQL_DTYPE_ATTR);
                            switch (attr_node->u.dtype_attr)
                            {
                            case SQL_DTYPE_LEN:
                                identifier_node = attr_node->child_list;
                                assert(identifier_node->entity_type == SQL_IDENTIFIER);
                                assert(identifier_node->u.identifier.ident_type == SQL_INTEGER_VALUE);
                                memcpy (&(*crecords [i])->dtype_size, identifier_node->u.identifier.identifier.name, sizeof (int));
                                break;
                            case SQL_DTYPE_PRIMARY_KEY:
                                (*crecords [i])->is_primary_key = true;
                                break;
                            case SQL_DTYPE_NOT_NULL:
                                (*crecords [i])->is_non_null = true;
                                break;
                            }
                         }
                         i++;
                    }
                break;
                default:
                    assert(0);
            }
        break;
        default:
            assert(0);
    }
     return col_count;
}

