
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include "sql_const.h"
#include "Catalog.h"
#include "sql_utils.h"
#include "sql_create.h"
#include "SqlMexprIntf.h"

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
parser_split_table_column_name ( char *composite_col_name, 
                                                        char *table_name_out,
                                                        char *col_name_out) {

    const char del[2] = ".";
    char composite_col_name_dup[SQL_COMPOSITE_COLUMN_NAME_SIZE] = {0};

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

qp_col_t *
sql_get_qp_col_by_name (   qp_col_t **qp_col_array, 
                                                        int n, 
                                                        char *name, 
                                                        bool is_alias) {

    int i;
    int len;
    qp_col_t *qp_col;

    len = strlen (name) ;
    
    for (i = 0; i < n; i++) {

        qp_col = qp_col_array[i];

        if (is_alias) {

            if (!qp_col->alias_provided_by_user) continue;
            if (strncmp (name, qp_col->alias_name, SQL_ALIAS_NAME_LEN)) continue;
            if (len != strlen (qp_col->alias_name)) continue;
            return qp_col;
        }
        else {

            if (!sql_is_single_operand_expression_tree  (qp_col->sql_tree)) continue;

            if (strncmp (
                name, 
                sql_get_opnd_variable_name (sql_tree_get_root (qp_col->sql_tree)).c_str(),
                SQL_COMPOSITE_COLUMN_NAME_SIZE)) continue;

            return qp_col;
        }
    }

    return NULL;
}