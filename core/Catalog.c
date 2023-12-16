#include <stdbool.h>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include "../BPlusTreeLib/BPlusTree.h"
#include "rdbms_struct.h"
#include "sql_const.h"
#include "sql_utils.h"
#include "sql_create.h"
#include "Catalog.h"

/* Global Default Catalog Table, one per Database */
BPlusTree_t TableCatalogDef;

extern  int 
rdbms_key_comp_fn (BPluskey_t *key_1, BPluskey_t *key_2, key_mdata_t *key_mdata, int size);

static bool initialized = false;

/* A fn used to free the 'value' of catalog table*/
static void 
catalog_table_free_fn (void *ptr) {

    ctable_val_t *ctable_val = (ctable_val_t *)ptr;

    BPlusTree_Destroy (ctable_val->schema_table);
    free (ctable_val->schema_table);
    BPlusTree_Destroy (ctable_val->record_table);
    free  (ctable_val->record_table);
    free(ptr);
}

static void 
schema_table_record_free (void *ptr) {

    schema_rec_t *schema_rec = (schema_rec_t *)ptr;
    free (schema_rec);
}

static void
Catalog_create_schema_table_records (
            BPlusTree_t *schema_table,
            sql_create_data_t *cdata) {

    int i;
    int offset = 0;
    BPluskey_t bpkey_tmplate;
    schema_rec_t *schema_rec;

    for (i = 0; i < cdata->n_cols; i++) {

        /* Setup the key */
        bpkey_tmplate.key = (void *)calloc(1, SQL_COLUMN_NAME_MAX_SIZE);
        strncpy( (char *)bpkey_tmplate.key, 
                      cdata->column_data[i].col_name,
                      SQL_COLUMN_NAME_MAX_SIZE);
        bpkey_tmplate.key_size = SQL_COLUMN_NAME_MAX_SIZE;

        /* Setup the value (a.k.a record)*/
        schema_rec = (schema_rec_t *)calloc(1, sizeof(schema_rec_t));
        strncpy(schema_rec->column_name,  
                     cdata->column_data[i].col_name,
                     SQL_COLUMN_NAME_MAX_SIZE);
        schema_rec->dtype = cdata->column_data[i].dtype;
        schema_rec->dtype_size = cdata->column_data[i].dtype_len;
        schema_rec->offset = offset;
        offset += cdata->column_data[i].dtype_len;
        schema_rec->is_primary_key = cdata->column_data[i].is_primary_key;

        /* Insert into Schema table now */
        assert (BPlusTree_Insert (schema_table, &bpkey_tmplate, (void *)schema_rec));
    }
}

bool 
Catalog_insert_new_table (BPlusTree_t *catalog_table, sql_create_data_t *cdata) {

    int i;
    BPluskey_t bkey;

    if (!catalog_table) {
        catalog_table = &TableCatalogDef;
    }

    static key_mdata_t key_mdata1[] = {  
                {SQL_INT,  4},
                {SQL_STRING, SQL_TABLE_NAME_MAX_SIZE} ,
                {SQL_INT, 4},
                {SQL_STRING, 32} ,
            };

    if (!initialized) {
        /* If this is the first table we are creating in a default DB, then initialize the catalog table. Catalog table is the collection of all tables in single DB along with their schema details*/
        BPlusTree_init(catalog_table,
                       rdbms_key_comp_fn,
                       NULL,
                       NULL,
                       SQL_BTREE_MAX_CHILDREN_CATALOG_TABLE, 
                       catalog_table_free_fn,
                       key_mdata1, sizeof(key_mdata1) / sizeof (key_mdata1[0]));

        initialized = true;
    }

    /* Prepare the key to be inserted in the catalog table*/
    catalog_table_key_t *catalog_table_key = (catalog_table_key_t *)calloc (1, sizeof (catalog_table_key_t));
    catalog_table_key->scope = PUBLIC;
    strncpy (catalog_table_key->entity_name, cdata->table_name, sizeof (catalog_table_key->entity_name));
    catalog_table_key->type = TABLE;
    strncpy (catalog_table_key->owner, "postgres", sizeof (catalog_table_key->owner));
    bkey.key = (void *)catalog_table_key;
    bkey.key_size = sizeof (catalog_table_key_t);

    if (BPlusTree_Query_Key (catalog_table, &bkey)) {
        printf ("Error : Table Already Exist\n");
        free(bkey.key);
        return false;
     }

    /* Let us create a VALUE for catalog table, so that we can attempt to do insertion of this record as early as possible in catalog table before creating other data structures. This would help us to rewind back if there is any error*/
    ctable_val_t *ctable_val = (ctable_val_t *)calloc (1, sizeof (ctable_val_t));
    strncpy(ctable_val->table_name, cdata->table_name, SQL_TABLE_NAME_MAX_SIZE);
    ctable_val->schema_table = NULL;
    ctable_val->record_table = NULL;

     for (i = 0; i < cdata->n_cols; i++) {
        
        strncpy (ctable_val->column_lst[i],
            cdata->column_data[i].col_name,
            SQL_COLUMN_NAME_MAX_SIZE );
     }

     /* Represent the end of array, be careful !*/
     ctable_val->column_lst[cdata->n_cols][0] = '\0';     

    /* Now create a Schema table for this new table. Schema table stores all the attributes and details of a RDBMS table. Every RDBMS table has a schema table*/
    
    static key_mdata_t key_mdata2[] = {{SQL_STRING, SQL_COLUMN_NAME_MAX_SIZE}};

    BPlusTree_t *schema_table = (BPlusTree_t *)calloc (1, sizeof (BPlusTree_t));
    BPlusTree_init (schema_table, 
                               rdbms_key_comp_fn,
                               NULL, 
                               NULL, 
                               SQL_BTREE_MAX_CHILDREN_SCHEMA_TABLE, 
                               schema_table_record_free,
                               key_mdata2, sizeof(key_mdata2) / sizeof (key_mdata2[0]));

    /* Schema table has been created, now insert records in it. Each record is of the type : 
       key::  <column name>   value :: <catalog_rec_t >  */
     Catalog_create_schema_table_records (schema_table, cdata);

    /* Now make the actual rdbms table to hold records */
    BPlusTree_t *record_table = (BPlusTree_t *)calloc (1, sizeof (BPlusTree_t));

    /* Construct key meta data for this Table Schema*/
    int key_mdata_size3;
    key_mdata_t *key_mdata3 = sql_construct_table_key_mdata (cdata, &key_mdata_size3);

    if (!key_mdata3) {
        BPlusTree_Destroy (record_table);
        BPlusTree_Destroy (schema_table);
        free(record_table);
        free(schema_table);
        free(ctable_val);
        free(bkey.key);
        printf ("Error : Table Must have atleast one primary key\n");
        return false;
    }

    BPlusTree_init (record_table, 
                               rdbms_key_comp_fn,
                               NULL, NULL, 
                               SQL_BTREE_MAX_CHILDREN_RDBMS_TABLE, free,
                               key_mdata3, key_mdata_size3);

    /* Now store the Schema Table and RDBMS table as VALUE of the catalog table*/
    ctable_val->schema_table = schema_table;
    ctable_val->record_table = record_table;

    BPlusTree_Insert (catalog_table, &bkey, (void *)ctable_val);
    printf ("CREATE TABLE\n");
    return true;
}

void 
sql_show_table_catalog (BPlusTree_t *TableCatalog) {

    int i;
    int rows = 0;
    void *rec_ptr;
    BPluskey_t *key_ptr;
    unsigned char table_name[SQL_TABLE_NAME_MAX_SIZE];
    
    BPlusTree_t *tcatalog = TableCatalog ? TableCatalog : &TableCatalogDef;

    printf ("           List of relations\n");
    printf (" Schema    |           Name           | Type  | Owner  \n");
    printf ("-----------+--------------------------+-------+--------------\n");

    BPTREE_ITERATE_ALL_RECORDS_BEGIN(tcatalog, key_ptr, rec_ptr) {

        tcatalog->key_fmt_fn (key_ptr, table_name, SQL_TABLE_NAME_MAX_SIZE);
        printf (" public    | %-23s  | table | postgres  \n", table_name);
        rows++;

    } BPTREE_ITERATE_ALL_RECORDS_END(tcatalog, key_ptr, rec_ptr)

    printf ("(%d rows)\n", rows);
}

ctable_val_t *
sql_catalog_table_lookup_by_table_name (BPlusTree_t *TableCatalog, 
                                                                      char *entity_name) {

    BPluskey_t bkey;
    catalog_table_key_t ckey;

    ckey.scope = PUBLIC;
    strncpy (ckey.entity_name, entity_name, sizeof (ckey.entity_name));
    ckey.type = TABLE;
    strncpy (ckey.owner, "postgres", sizeof (ckey.owner));

    bkey.key = (void *)&ckey;
    bkey.key_size = sizeof (ckey);

    return (ctable_val_t *)BPlusTree_Query_Key (TableCatalog, &bkey);
}
