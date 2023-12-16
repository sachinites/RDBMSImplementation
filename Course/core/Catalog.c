#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "Catalog.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "sql_create.h"

/* Global Default Catalog Table, one per Database */
BPlusTree_t TableCatalogDef;

extern int 
rdbms_key_comp_fn ( BPluskey_t *key1, BPluskey_t *key2, key_mdata_t *key_mdata, int key_mdata_size) ;

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
 schema_table_record_free ( void *ptr) {

    free(ptr);
 }


// key : column table
// Record : pointer to schema_rec_t object
static void
Catalog_create_schema_table_records (BPlusTree_t *schema_table, sql_create_data_t *cdata) {

    int i;
    int offset = 0;
    BPluskey_t bpkey_tmplate;
    schema_rec_t *schema_rec;

    for (i = 0; i < cdata->n_cols; i++) {

        /* Setup the key*/
        bpkey_tmplate.key = (void *)calloc (1, SQL_COLUMN_NAME_MAX_SIZE);
        strncpy ((char *) bpkey_tmplate.key, 
                        cdata->column_data[i].col_name,
                        SQL_COLUMN_NAME_MAX_SIZE);
        bpkey_tmplate.key_size = SQL_COLUMN_NAME_MAX_SIZE;

        /* Set up the value (a.k.a record)*/
        schema_rec = (schema_rec_t *)calloc (1, sizeof (schema_rec_t));
        strncpy (schema_rec->column_name,
                        cdata->column_data[i].col_name, 
                        SQL_COLUMN_NAME_MAX_SIZE);
        schema_rec->dtype = cdata->column_data[i].dtype;
        schema_rec->dtype_size = cdata->column_data[i].dtype_len;
        schema_rec->offset = offset;
        offset += cdata->column_data[i].dtype_len;
        schema_rec->is_primary_key = cdata->column_data[i].is_primary_key;

        /* Insert into Schema Table*/
        assert (BPlusTree_Insert (schema_table, &bpkey_tmplate, (void *)schema_rec));
    }
}

static bool initialized = false;

bool 
Catalog_insert_new_table (BPlusTree_t *catalog, sql_create_data_t *cdata) {

    int i;
    BPluskey_t bpkey;

    // Implementation : Creation and initialization of Catalog table
    static key_mdata_t key_mdata1[] = {  {SQL_STRING, SQL_TABLE_NAME_MAX_SIZE}};

    if (!initialized) {

        BPlusTree_init (catalog, 
                                rdbms_key_comp_fn,
                                NULL, 
                                NULL,
                                SQL_BTREE_MAX_CHILDREN_CATALOG_TABLE, 
                                catalog_table_free_fn,
                                key_mdata1, 
                                sizeof(key_mdata1) / sizeof (key_mdata1[0]));

        initialized = true;
    }
    
    // prepare the key to be inserted
    bpkey.key = (void *)calloc (1, SQL_TABLE_NAME_MAX_SIZE);
    bpkey.key_size =  SQL_TABLE_NAME_MAX_SIZE;
    strncpy((char *)bpkey.key, cdata->table_name, SQL_TABLE_NAME_MAX_SIZE);

    // Now let us check for SQL table existence
    if (BPlusTree_Query_Key (catalog, &bpkey)) {
        printf ("Error : Table Already Exist\n");
        free(bpkey.key);
        return false;
    }

    // Now let us create the value/record for catalog table
    ctable_val_t *ctable_val = (ctable_val_t *)calloc (1, sizeof (ctable_val_t));
    strncpy (ctable_val->table_name, cdata->table_name, SQL_TABLE_NAME_MAX_SIZE);
    ctable_val->schema_table = NULL;
    ctable_val->record_table = NULL;

    // Fill the column array also in ctable_val 
    for (i = 0; i < cdata->n_cols; i++) {
        
        strncpy (ctable_val->column_lst[i],
            cdata->column_data[i].col_name,
            SQL_COLUMN_NAME_MAX_SIZE );
     }

    /* Represent the end of array, be careful !*/
     ctable_val->column_lst[cdata->n_cols][0] = '\0';     

    // Implementation  : creating of schema table of SQL table ( emp )

    static key_mdata_t key_mdata2[] = {   {SQL_STRING, SQL_COLUMN_NAME_MAX_SIZE}  }  ;

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


    // Implementation  : Creation of Record Table of SQL table ( emp )

        /* Now make the actual rdbms table ( record table ) to hold records */
    BPlusTree_t *record_table = (BPlusTree_t *)calloc (1, sizeof (BPlusTree_t));

    /* Construct key meta data for this Table Schema*/
    int key_mdata_size3;
    key_mdata_t *key_mdata3 = sql_construct_table_key_mdata (cdata, &key_mdata_size3);

    if (!key_mdata3) {
        BPlusTree_Destroy (record_table);
        BPlusTree_Destroy (schema_table);
        printf ("Error : Table Must have atleast one primary key\n");
        free(record_table);
        free(schema_table);
        free(ctable_val);
        free(bpkey.key);
        return false;
    }

    BPlusTree_init (record_table, 
                               rdbms_key_comp_fn,
                               NULL, NULL, 
                               SQL_BTREE_MAX_CHILDREN_RDBMS_TABLE, free,
                               key_mdata3, key_mdata_size3);

    ctable_val->schema_table = schema_table;
    ctable_val->record_table = record_table;

    BPlusTree_Insert (catalog, &bpkey, (void *)ctable_val);
    printf ("CREATE TABLE\n");
    return true;
}

void 
Schema_table_print (BPlusTree_t *schema_table) {

    void *rec;
    BPluskey_t *bpkey;
    schema_rec_t *schema_rec;

    BPTREE_ITERATE_ALL_RECORDS_BEGIN(schema_table, bpkey, rec) {
        
        schema_rec = (schema_rec_t *)rec;

        printf ("Column Name : %s  Dtype = %d  Dtype Len = %d  Is_Primary_key = %s  offset = %d\n",
            (char *)bpkey->key, 
            schema_rec->dtype, 
            schema_rec->dtype_size, 
            schema_rec->is_primary_key ? "Y" : "N" , 
            schema_rec->offset); 

    } BPTREE_ITERATE_ALL_RECORDS_END(schema_table, bpkey, rec);

}

void 
Catalog_table_print (BPlusTree_t *catalog) {

    int i = 0;
    void *rec;
    BPluskey_t *bpkey;
    ctable_val_t *ctable_val;
    BPlusTree_t *schema_table;

    BPTREE_ITERATE_ALL_RECORDS_BEGIN(catalog, bpkey, rec) {

        printf ("Record Table Name = %s\n", (char *)bpkey->key);
        ctable_val = (ctable_val_t *)rec;

        printf ("Schema Table : \n");
        Schema_table_print (ctable_val->schema_table);

        printf ("Record Table Ptr : %p\n", ctable_val->record_table);

        printf ("Column List : \n");

        i = 0;
        while (ctable_val->column_lst[i][0] != '\0') {

            printf ("%s ", ctable_val->column_lst[i]);
            i++;
        }

        printf ("\n======\n");
        
    } BPTREE_ITERATE_ALL_RECORDS_END(catalog, bpkey, rec);
}
