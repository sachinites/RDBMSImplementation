
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>
#include "qep.h"
#include "sql_const.h"
#include "Catalog.h"
#include "sql_utils.h"
#include "sql_create.h"
#include "SqlMexprIntf.h"

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
parser_split_table_column_name ( std::unordered_map<std::string, std::string> *map,
                                                        BPlusTree_t *tcatalog,
                                                        char *composite_col_name, 
                                                        char *table_name_out,
                                                        char *col_name_out) {

    ctable_val_t *ctable_val;
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

    ctable_val = sql_catalog_table_lookup_by_table_name (tcatalog, str1);

    if (ctable_val) {
        strncpy (table_name_out, str1, SQL_TABLE_NAME_MAX_SIZE);
    }
    else {
        /* str1 should be table alias name */
        std::string table_name = (*map)[std::string (str1)];
        strncpy (table_name_out, table_name.c_str(), SQL_TABLE_NAME_MAX_SIZE);
    }
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
                qp_col->alias_name,
                SQL_COMPOSITE_COLUMN_NAME_SIZE)) continue;

            return qp_col;
        }
    }

    return NULL;
}

bool
sql_read_interval_values (char *string_fmt,  // "[ a , b]"
                                            int *a, int *b) {

    int i = 0, j = 0;
    char dst_buffer[64];

    int string_fmt_len = strlen (string_fmt);

    if (string_fmt_len > sizeof(dst_buffer)) {
        return false;
    }

    while (string_fmt[i] != '\0' && i < string_fmt_len)
    {
        if (string_fmt[i] == ' ')
        {
            i++;
            continue;
        }
        dst_buffer[ j++] = string_fmt[i];
        i++;
    }
    dst_buffer[j] = '\0';
    sscanf(dst_buffer, "[%d,%d]", a, b);
    return true;
}

void 
sql_select_flush_computed_values (qep_struct_t *qep) {

    int i;
    qp_col_t *sqp_col;

    for (i = 0; i < qep->select.n; i++) {

        sqp_col = qep->select.sel_colmns[i];

        if (sqp_col->computed_value) {
            sql_destroy_Dtype_value_holder (sqp_col->computed_value);
            sqp_col->computed_value = NULL;
        }

        if (sqp_col->aggregator) {
            sql_destroy_aggregator (sqp_col);
            sqp_col->aggregator = NULL;
        }
    }
}