
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include "../SqlParser/SqlEnums.h"
#include "sql_insert_into.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "Catalog.h"

extern BPlusTree_t TableCatalogDef;

static void 
sql_print_record (BPlusTree_t *schema_table, void *record) {

    /* The fn to print the record individual fields, not necessarily in the order
        in which they exist in a record */
    void *rec;
    BPluskey_t *bpkey_ptr;
    schema_rec_t *schema_rec;

    BPTREE_ITERATE_ALL_RECORDS_BEGIN(schema_table, bpkey_ptr, rec) {

        schema_rec = (schema_rec_t *)rec;

        switch (schema_rec->dtype) {

            case SQL_STRING:
                printf ("%s ", (char *)record + schema_rec->offset);
                break;
            case SQL_INT:
                printf ("%d ", *(int *)((char *)record + schema_rec->offset));
                break;
            case SQL_DOUBLE:
                printf ("%lf ", *(double *)((char *)record + schema_rec->offset));
                break;
            default:
                assert(0);
        }

    } BPTREE_ITERATE_ALL_RECORDS_END(schema_table, bpkey_ptr, rec);
}


static bool
sql_insert_new_record ( BPlusTree_t *tcatalog, sql_insert_into_data_t *idata) {

#if 0

    /* Let us understand the steps by taking the below SQL setup as an example */
    create table emp ( emp_id int primary key, emp_name varchar(32), des_code int )

    insert into emp values (21, “Abhishek”, 104 )
        key : <21>  record : <21 Abhishek 104>

    insert into emp values (22, “Ankit Sharma”, 104 )
        key : <22>  record : <22 Ankit Sharma 104>

#endif

    /* Step 1 : If the table 'emp' do not exist in Catalog table, return false*/
    BPluskey_t bpkey;
    char *table_name = idata->table_name;

    bpkey.key = (void *)table_name;
    bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;

    ctable_val_t *ctable_val = (ctable_val_t *)BPlusTree_Query_Key (tcatalog, &bpkey);
    if (!ctable_val) {
        printf ("Error : Table not found\n");
        return false;
    }    

    BPlusTree_t *schema_table = ctable_val->schema_table;
    BPlusTree_t *record_table = ctable_val->record_table;

    /* Step 2 : We need to insert <key><Record> in table emp - this is our end goal.
        Therefore, we need to malloc memory to accomodate key and Record respectively.
        For this , we need to know the size of key and size of record in Bytes. 
        
            For each schema_rec_t in 'emp' schema table
                if the column is primary key
                        key_size += schema_rec->dtype_size

                rec_size += schema_rec->dtype_size ( key is also part of record )
        */
    void *rec;
    int key_size = 0;
    int rec_size = 0;
    BPluskey_t *bpkey_ptr;
    schema_rec_t *schema_rec;

   BPTREE_ITERATE_ALL_RECORDS_BEGIN(schema_table, bpkey_ptr, rec) {

        schema_rec = (schema_rec_t *)rec;

        if (schema_rec->is_primary_key) {
             key_size += schema_rec->dtype_size;
        }
        rec_size += schema_rec->dtype_size;

   } BPTREE_ITERATE_ALL_RECORDS_END(schema_table, bpkey_ptr, rec);

    /* Step 3 : Allocate memory for key, and Calculate key content 
        For each column COL in ctable_val->column_lst list 
            get schema_rec_t 'schema_rec' for this COL from 'emp' schema table
            if schema_rec->is_primary_key 
                copy value of this COL from idata into key buffer ( serialize )
    */
    int i = 0;
    int key_offset = 0;
    BPluskey_t new_bpkey;

    new_bpkey.key = (void *)calloc (1, key_size);
    new_bpkey.key_size = key_size;

    /* We need to iterate in the same order in which columns 
         were specified by the user in create table query */
    while (ctable_val->column_lst[i][0] != '\0') {

        bpkey.key = (void *)ctable_val->column_lst[i];
        bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
        schema_rec = (schema_rec_t *) BPlusTree_Query_Key (schema_table, &bpkey);

        if (schema_rec->is_primary_key == false) {
            i++;
            continue;
        }

        switch (schema_rec->dtype) {

            case SQL_STRING:
                strncpy((char *)new_bpkey.key + key_offset,
                        idata->sql_values[i].u.str_val,
                        schema_rec->dtype_size);
                key_offset += schema_rec->dtype_size;
                break;
            case SQL_INT:
                memcpy((char *)new_bpkey.key + key_offset,
                    (unsigned char *)&idata->sql_values[i].u.int_val,
                    schema_rec->dtype_size);
                key_offset += schema_rec->dtype_size;
                break;
            case SQL_DOUBLE:
                memcpy((char *)new_bpkey.key + key_offset,
                    (unsigned char *)&idata->sql_values[i].u.d_val,
                    schema_rec->dtype_size);
                key_offset += schema_rec->dtype_size;
                break;
            default:
                assert(0);
        }
        i++;
    }

    /* Step 4 : Allocate memory for record, and Calculate Record content 
            For each column COL in ctable_val->column_lst list 
                get schema_rec_t 'schema_rec' for this COL from 'emp' schema table
                copy value of this COL from idata into record buffer ( serialize )
    */
    i = 0;
    void *record = (void *)calloc(1, rec_size);

    while (ctable_val->column_lst[i][0] != '\0') {

        bpkey.key = (void *)ctable_val->column_lst[i];
        bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
        schema_rec = (schema_rec_t *) BPlusTree_Query_Key (schema_table, &bpkey);

        switch (schema_rec->dtype) {

            case SQL_STRING:
                strncpy((char *)record + schema_rec->offset,
                        idata->sql_values[i].u.str_val,
                        schema_rec->dtype_size);
                break;
            case SQL_INT:
                memcpy((char *)record + schema_rec->offset,
                    (unsigned char *)&idata->sql_values[i].u.int_val,
                    schema_rec->dtype_size);
                break;
            case SQL_DOUBLE:
                memcpy((char *)record + schema_rec->offset,
                    (unsigned char *)&idata->sql_values[i].u.d_val,
                    schema_rec->dtype_size);
                break;
            default:
                assert(0);
        }
        i++;                
    }

    /* Step 5 : Check if the record already exist in table 'emp' with same key, if yes, print error and return false */

    /* Check for Duplication */
    if (BPlusTree_Query_Key (record_table, &new_bpkey)) {
        free (new_bpkey.key);
        free(record);
        printf ("ERROR:  duplicate key value violates unique constraint \"%s_pkey\"\n", table_name);
        return false;
    }

    /* Stpe 6 : Insert <key> <Record > in emp table */
    BPlusTree_Insert (record_table, &new_bpkey, record);

    /* Step 7 : print Success Msg, and return true */
    printf ("INSERT 0 1\n");
    return true;    
}

void
 sql_process_insert_query (sql_insert_into_data_t *idata) {

     sql_insert_new_record (&TableCatalogDef, idata);
 }

