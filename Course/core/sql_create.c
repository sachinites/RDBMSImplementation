#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "sql_create.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "Catalog.h"

extern BPlusTree_t TableCatalogDef;

void 
sql_create_data_destroy (sql_create_data_t *cdata) {

    memset (cdata, 0, sizeof (*cdata));
}

 void 
 sql_process_create_query (sql_create_data_t *cdata) {

    Catalog_insert_new_table (&TableCatalogDef, cdata);
 }

key_mdata_t *
sql_construct_table_key_mdata (sql_create_data_t *cdata, int *key_mdata_size) {

    int i, j;
    int primary_key_count = 0;

   for (i = 0; i < cdata->n_cols; i++) {

      if (cdata->column_data[i].is_primary_key) primary_key_count++;
   }

   if (primary_key_count == 0) {
      *key_mdata_size = 0;
      return NULL;
   }

    key_mdata_t *key_mdata = (key_mdata_t *)calloc (primary_key_count, sizeof (key_mdata_t));

    for (i = 0, j = 0; i < cdata->n_cols; i++) {

        if (cdata->column_data[i].is_primary_key) {
            key_mdata[j].dtype = cdata->column_data[i].dtype;
            key_mdata[j].size = cdata->column_data[i].dtype_len;
            j++;
        }
    }

    *key_mdata_size = j;
    return key_mdata;
}