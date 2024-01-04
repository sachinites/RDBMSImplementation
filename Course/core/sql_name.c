#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "sql_const.h"
#include "sql_name.h"
#include "qep.h"
#include "Catalog.h"
#include "../BPlusTreeLib/BPlusTree.h"

extern BPlusTree_t TableCatalogDef;

sql_col_name_type_t
sql_col_get_name_type ( qep_struct_t *qep, 
                                         char *col_name) {

    BPluskey_t bpkey;
    const char del[2] = ".";
    char fqcn[SQL_FQCN_SIZE] = {0};

    strncpy (fqcn, col_name, SQL_FQCN_SIZE);

    char *str1 = strtok (fqcn, del);
    char *str2 = strtok (NULL, del);

    if (!str1) return SQL_COL_NAME_NOT_KNOWN;
    if (!str1 && !str2) return SQL_COL_NAME_NOT_KNOWN;
    if (str1 && !str2) return SQL_COL_NAME_LCN;

    auto it = qep->join.table_alias->find(str1);
    if (it !=  qep->join.table_alias->end()) return SQL_COL_NAME_ACN;

    bpkey.key = (void *)str1;
    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;

    ctable_val_t *ctable_val = (ctable_val_t *) BPlusTree_Query_Key (
                                &TableCatalogDef, &bpkey);
    
    if (ctable_val) {
        return SQL_COL_NAME_FQCN;
    }

    return SQL_COL_NAME_NOT_KNOWN;
}


void 
sql_get_column_table_names ( qep_struct_t *qep,
                                                    char *input_column_name, 
                                                    char *table_name_out,
                                                    char *col_name_out) {
    
    const char del[2] = ".";
    char fqcn[SQL_FQCN_SIZE] = {0};

    strncpy (fqcn, input_column_name, SQL_FQCN_SIZE);

    char *str1 = strtok (fqcn, del);
    char *str2 = strtok (NULL, del);

    sql_col_name_type_t col_name_type = sql_col_get_name_type (
                                                                        qep, input_column_name);

    switch (col_name_type) {

        case SQL_COL_NAME_NOT_KNOWN:
            table_name_out[0] = '\0';
            col_name_out[0] = '\0';
            break;

        case SQL_COL_NAME_FQCN:
            strncpy (table_name_out, str1, SQL_TABLE_NAME_MAX_SIZE);
            strncpy (col_name_out, str2, SQL_COLUMN_NAME_MAX_SIZE);
            break;

        case SQL_COL_NAME_ACN:
        {
            auto it = qep->join.table_alias->find(str1);
            strncpy (table_name_out, it->second.c_str(), SQL_TABLE_NAME_MAX_SIZE);
            strncpy (col_name_out, str2, SQL_COLUMN_NAME_MAX_SIZE);
        }
        break;

        case SQL_COL_NAME_LCN:
            strncpy (table_name_out, qep->join.tables[0].table_name, 
                            SQL_TABLE_NAME_MAX_SIZE);
            strncpy (col_name_out, str1, SQL_COLUMN_NAME_MAX_SIZE);
            break;
        
        default:
            assert(0);
    }
}