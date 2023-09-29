
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include "sql_const.h"
#include "Catalog.h"
#include "sql_utils.h"
#include "sql_create.h"

int 
rdbms_key_comp_fn (BPluskey_t *key_1, BPluskey_t *key_2, key_mdata_t *key_mdata, int size) {

    int i , rc;
    int dsize;
    int offset = 0;

    sql_dtype_t dtype;

    if (!key_1 || !key_1->key || !key_1->key_size) return 1;
    if (!key_2 || !key_2->key || !key_2->key_size) return -1;

    char *key1 = (char *)key_1->key;
    char *key2 = (char *)key_2->key;

    if (!key_mdata || !size) {
        /* Implement a general memcmp function */
        assert (key_1->key_size == key_2->key_size);
        rc = memcmp (key1, key2, key_1->key_size);
        if (rc > 0) return -1;
        if (rc < 0) return 1;
        return 0; 
    }

    for (i = 0; i < size ; i++) {

        dtype = key_mdata->dtype;
        dsize = key_mdata->size;

        switch (dtype) {

            case SQL_STRING:
                rc = strncmp (key1 + offset, key2 + offset, dsize);
                if (rc < 0) return 1;
                if (rc > 0) return -1;
                offset += dsize;
                break;
            case  SQL_INT:
                {
                    int *n1 = (int *)(key1 + offset);
                    int *n2 = (int *)(key2 + offset);
                    if (*n1 < *n2) return 1;
                    if (*n1 > *n2) return -1;
                    offset += dsize;
                }
                break;
            case SQL_IPV4_ADDR:
                {
                    uint32_t *n1 = (uint32_t *)(key1 + offset);
                    uint32_t *n2 = (uint32_t *)(key2 + offset);
                    if (*n1 < *n2) return 1;
                    if (*n1 > *n2) return -1;
                    offset += dsize;
                }
                break;
            case SQL_DOUBLE:
              {
                    double *n1 = (double *)(key1 + offset);
                    double*n2 = (double *)(key2 + offset);
                    if (*n1 < *n2) return 1;
                    if (*n1 > *n2) return -1;
                    offset += dsize;
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

int
BPlusTree_key_format_fn_default (BPluskey_t *key, unsigned char *obuff, int buff_size) {

	assert (key->key_size <= buff_size);
	memset (obuff, 0, buff_size);
	memcpy (obuff, key->key, key->key_size);
	return  key->key_size;
}

int
BPlusTree_value_format_fn_default (void *value, unsigned char *obuff, int buff_size) {

	memset (obuff, 0, buff_size);
	strncpy ( (char *)obuff, (char *)value, buff_size);
	return 0;
}

key_mdata_t *
sql_construct_table_key_mdata (sql_create_data_t *cdata, int *key_mdata_size) {

    int i, j;
    bool primary_key_set = false;

    key_mdata_t *key_mdata = (key_mdata_t *)calloc
         (SQL_MAX_PRIMARY_KEYS_SUPPORTED, sizeof (key_mdata_t));

    for (i = 0, j = 0; i < cdata->n_cols; i++) {

        if (cdata->column_data[i].is_primary_key) {
            key_mdata[j].dtype = cdata->column_data[i].dtype;
            key_mdata[j].size = cdata->column_data[i].dtype_len;
            primary_key_set = true;
            j++;
        }
    }

    if (!primary_key_set ) {
        free(key_mdata);
        key_mdata = NULL;
    }

    *key_mdata_size = j;
    return key_mdata;
}

void 
sql_compute_aggregate (sql_agg_fn_t agg_fn, 
                                        void *src, void *dst, 
                                        sql_dtype_t dtype, 
                                        int dype_size, int row_no) {

    switch (agg_fn) {

        case SQL_SUM:
            {
                switch (dtype) {

                    case SQL_INT:
                    {
                        int *src_int = (int *)src;
                        int *dst_int = (int *)dst;
                        *dst_int  += *src_int ;
                    }
                    break;
                    default: ;
                }
            }
            break;


        case SQL_AVG:
        {
            switch (dtype) {
                case SQL_INT:
                {
                    int *src = (int *)src;
                    int *dst = (int *)dst;                    
                    if (row_no == 1) {
                        *dst  = *src;
                        break;
                    }
                    *dst = ((*dst) * (row_no -1));
                    *dst += *src;
                    *dst = *dst / row_no;
                }
                break;
                case SQL_DOUBLE:
                {
                    double *src = (double *)src;
                    double *dst = (double *)dst;                    
                    if (row_no == 1) {
                        *dst  = *src;
                        break;
                    }
                    *dst = ((*dst) * (row_no -1));
                    *dst += *src;
                    *dst = *dst / row_no;
                }                
            }
        }
        break;

        case SQL_MAX:
        {
                switch (dtype)
                {
                case SQL_INT:
                {
                        int *src_int = (int *)src;
                        int *dst_int = (int *)dst;
                        if (row_no == 1) {
                            *dst_int = INT32_MIN;
                        }
                       if (*dst_int < *src_int) {
                            *dst_int = *src_int;
                       }
                }
                break;
                default:;
                }
        }
        break;


        case SQL_MIN:
        {
                switch (dtype)
                {
                case SQL_INT:
                {
                        int *src_int = (int *)src;
                        int *dst_int = (int *)dst;
                        if (row_no == 1) {
                            *dst_int = INT32_MAX;
                        }
                       if (*dst_int > *src_int) {
                            *dst_int = *src_int;
                       }
                }
                break;
                default:;
                }
        }
        break;        


        case SQL_COUNT:
        {
                switch (dtype)
                {
                default:
                {
                        int *dst_int = (int *)dst;
                         (*dst_int)++;
                }
                break;
                }
        }
        break;       



        default: ;
    }
}

void 
sql_compute_column_text_name (qp_col_t *col, unsigned char *column_name, int size) {

    memset (column_name, 0, size);

    if (col->agg_fn != SQL_AGG_FN_NONE) {
        snprintf (column_name, size,
                    "%s(%s)", sql_agg_fn_tostring(col->agg_fn), col->schema_rec->column_name);
    }
    else {
        snprintf (column_name, size, 
                    "%s", col->schema_rec->column_name);

    }
}


static void *
joined_row_search ( int table_id, joined_row_t *joined_row) {

    if (joined_row->size == 1) {

        if (joined_row->table_id_array[0] == table_id) {
            return  joined_row->rec_array[0];
        }

        return NULL;
    }

   return joined_row->rec_array[table_id];
}

void *
sql_get_column_value_from_joined_row (joined_row_t *joined_row, qp_col_t *col) {

   void *rec = joined_row_search (col->owner_table_id, joined_row);
    if (!rec) return NULL;
    return (void *)((char *)rec + col->schema_rec->offset);
}

void 
parser_split_table_column_name ( unsigned char *composite_col_name, 
                                                        unsigned char *table_name_out,
                                                        unsigned char *col_name_out) {

    const char del[2] = ".";
    unsigned char composite_col_name_dup[SQL_COMPOSITE_COLUMN_NAME_SIZE] = {0};

    strncpy (composite_col_name_dup, composite_col_name, SQL_COMPOSITE_COLUMN_NAME_SIZE);
    char *str1 = strtok (composite_col_name_dup, del);
    char *str2 = strtok (NULL, del);
    if (str2 == NULL) {
        strncpy (col_name_out, str1, SQL_COLUMN_NAME_MAX_SIZE);
        table_name_out[0] = '\0';
        return;
    }
    strncpy (table_name_out, str1, SQL_TABLE_NAME_MAX_SIZE);
    strncpy (col_name_out, str2, SQL_COLUMN_NAME_MAX_SIZE);
}

#if 0
bool 
qp_col_is_equal (qp_col_t *col1, qp_col_t *col2) {

    //if (col1->ctable_val != col2->ctable_val) return false;
    if (col1->owner_table_id != col2->owner_table_id) return false;
    if (col1->schema_rec != col2->schema_rec) return false;
    if (col1->agg_fn != col2->agg_fn) return false;
    return true;
}

qp_col_t *
qp_col_lookup_identical (qp_col_t **col_list, int size, qp_col_t *key_col) {

    int i;
    for (i = 0; i < size; i++) {

        if (qp_col_is_equal (col_list[i] , key_col)) return col_list[i];
    }
    return NULL;
}
#endif 