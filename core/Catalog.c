#include <stdbool.h>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include "../gluethread/glthread.h"
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
    list_node_t *lnode;
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
    strncpy(ctable_val->table_name, tble_name, SQL_TABLE_NAME_MAX_SIZE);
    ctable_val->schema_table = NULL;
    ctable_val->rdbms_table = NULL;
    init_glthread (&ctable_val->col_list_head);

     if (!BPlusTree_Insert (catalog_table, &bkey, (void *)ctable_val)) {

        printf ("Error : Table Already Exist\n");
        free (ctable_val);
        free(bkey.key);
        return false;
     }

    {
        bkey.key = (char *)calloc (1, SQL_TABLE_NAME_MAX_SIZE);
        snprintf (bkey.key, SQL_TABLE_NAME_MAX_SIZE, "%s2", tble_name);
        bkey.key_size = SQL_TABLE_NAME_MAX_SIZE;
        BPlusTree_Insert (catalog_table, &bkey, (void *)ctable_val);
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
        
        lnode = (list_node_t *)calloc (1, sizeof (list_node_t));
        init_glthread(&lnode->glue);
        lnode->data = (void *)bkeys[i]->key;
        glthread_add_last (&ctable_val->col_list_head, &lnode->glue);
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
    int offset = 0;
    bool offset_upd = false;
    ast_node_t *attr_node;
    ast_node_t *col_node;
    ast_node_t *identifier_node;
    ast_node_t *col_dtype_node;
    ast_node_t astnode_tmplate;
    ast_node_t *table_name_node;

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
                    } FOR_ALL_AST_CHILD_END;

                    if (!col_count) return 0;

                    *bkeys = (BPluskey_t **)calloc (col_count , sizeof (BPluskey_t *));
                    *crecords = (schema_rec_t **)calloc (col_count , sizeof ( schema_rec_t *));

                    i = 0;
                    FOR_ALL_AST_CHILD(table_name_node, col_node) {
                        
                        offset_upd = false;

                        (*bkeys)[i] = (BPluskey_t *)calloc (1, sizeof (BPluskey_t));
                        (*bkeys)[i]->key = (void *)calloc (1, SQL_COLUMN_NAME_MAX_SIZE);
                        strncpy ((*bkeys)[i]->key, col_node->u.identifier.identifier.name, SQL_COLUMN_NAME_MAX_SIZE);
                        (*bkeys)[i]->key_size = SQL_COLUMN_NAME_MAX_SIZE;

                        (*crecords)[i] =  (schema_rec_t *)calloc (1 , sizeof ( schema_rec_t ));
                        strncpy ((*crecords)[i]->column_name,  col_node->u.identifier.identifier.name, SQL_COLUMN_NAME_MAX_SIZE);
                        col_dtype_node = col_node->child_list;
                        assert (col_dtype_node->entity_type == SQL_DTYPE);
                        (*crecords)[i]->dtype = col_dtype_node->u.dtype;
                        (*crecords)[i]->dtype_size = sql_dtype_size (col_dtype_node->u.dtype);
                        (*crecords)[i]->offset = offset ;

                         FOR_ALL_AST_CHILD(col_dtype_node, attr_node) {

                            assert (attr_node->entity_type == SQL_DTYPE_ATTR);

                            switch (attr_node->u.dtype_attr)
                            {
                            case SQL_DTYPE_LEN:
                                identifier_node = attr_node->child_list;
                                assert(identifier_node->entity_type == SQL_IDENTIFIER);
                                assert(identifier_node->u.identifier.ident_type == SQL_INTEGER_VALUE);
                                memcpy (&(*crecords)[i]->dtype_size,
                                    identifier_node->u.identifier.identifier.name, sizeof (int));
                                offset += (*(int *)identifier_node->u.identifier.identifier.name ) * 
                                                sql_dtype_size(col_dtype_node->u.dtype);
                                offset_upd = true;
                                break;
                            case SQL_DTYPE_PRIMARY_KEY:
                                (*crecords)[i]->is_primary_key = true;
                                break;
                            case SQL_DTYPE_NOT_NULL:
                                (*crecords)[i]->is_non_null = true;
                                break;
                            }
                         } FOR_ALL_AST_CHILD_END;
                         if (offset_upd == false) {
                            offset += (*crecords)[i]->dtype_size;
                            offset_upd = true;
                         }
                         i++;
                    } FOR_ALL_AST_CHILD_END;
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

bool
Catalog_get_column (BPlusTree_t *tcatalog, 
                                    char *table_name, 
                                    char *col_name,
                                    qp_col_t *qp_col) {

    BPluskey_t bpkey;
    ctable_val_t *ctable_val;

    memset (qp_col, 0, sizeof (*qp_col));

    bpkey.key =  table_name;
    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;

    ctable_val = (ctable_val_t *)BPlusTree_Query_Key (
                                tcatalog ? tcatalog : &TableCatalogDef, &bpkey);

    if (!ctable_val) return false;

    BPlusTree_t *schema_table = ctable_val->schema_table;

    qp_col->ctable_val = ctable_val;

    bpkey.key = col_name;
    bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;

    qp_col->schema_rec = (schema_rec_t *) BPlusTree_Query_Key (schema_table, &bpkey);
    if (!qp_col->schema_rec) return false;
    qp_col->agg_fn = SQL_AGG_FN_NONE;

    return true;
}

bool
sql_process_select_wildcard (BPlusTree_t *tcatalog, ast_node_t *select_kw, ast_node_t *table_name_node) {

    void *rec;
    glthread_t *curr;
    list_node_t *lnode;
    ast_node_t *column_node;
    ctable_val_t *ctable_val;
    BPlusTree_t *tcatalog_to_use;
    BPluskey_t bpkey;

    tcatalog_to_use = tcatalog ? tcatalog : &TableCatalogDef;

    if (!tcatalog_to_use->Root)  {
        printf ("Error : relation does not exist\n");
        return false;
    }

    bpkey.key =  table_name_node->u.identifier.identifier.name;
    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;        

    ctable_val = (ctable_val_t *)BPlusTree_Query_Key (tcatalog_to_use, &bpkey);

    if (!ctable_val) {
        printf ("Error : relation does not exist\n");
        return false;
    }

    ITERATE_GLTHREAD_BEGIN(&ctable_val->col_list_head, curr) {

        lnode = glue_to_list_node (curr);
        column_node = (ast_node_t *) calloc (1, sizeof (ast_node_t));
        column_node->entity_type = SQL_IDENTIFIER;
        column_node->u.identifier.ident_type = SQL_COLUMN_NAME;
        snprintf (column_node->u.identifier.identifier.name, 
                      sizeof (column_node->u.identifier.identifier.name),
                      "%s.%s",
                      table_name_node->u.identifier.identifier.name, (char *)lnode->data);
        column_node->data = (int *)calloc (1, sizeof (int));
        *(int *)column_node->data = *(int *)table_name_node->data;
        ast_add_child (select_kw, column_node);

    } ITERATE_GLTHREAD_END(&ctable_val->col_list_head, curr) 

    return true;
}
