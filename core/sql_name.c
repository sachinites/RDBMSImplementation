#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "sql_const.h"
#include "sql_name.h"
#include "qep.h"

bool 
sql_is_name_FQCN ( std::unordered_map<std::string, std::string> *table_alias, 
                                    char *name)  {

    const char del[2] = ".";
    char fqcn[SQL_FQCN_SIZE] = {0};

    strncpy (fqcn, name, SQL_FQCN_SIZE);

    char *str1 = strtok (fqcn, del);
    char *str2 = strtok (NULL, del);

    if (!str1 || !str2 ) return false;

    // str1 is prefix and str2 is suffix
    auto it = table_alias->find(str1);

    // entry is found in alias table, then it was ACN
    if (it != table_alias->end()) return false;

    // else, we assume it was FQCN
    return true;
}

bool 
sql_is_name_ACN (std::unordered_map<std::string, std::string> *table_alias, 
                                char *name)  {


    const char del[2] = ".";
    char fqcn[SQL_FQCN_SIZE] = {0};

    strncpy (fqcn, name, SQL_FQCN_SIZE);

    char *str1 = strtok (fqcn, del);
    char *str2 = strtok (NULL, del);

    if (!str1 || !str2 ) return false;

    // str1 is prefix and str2 is suffix
    auto it = table_alias->find(str1);

    // entry is found in alias table, then it was ACN
    if (it != table_alias->end()) return true;

    // else, we assume it was FQCN
    return false;

}

bool 
sql_is_name_LCN (char *name) {

    const char del[2] = ".";
    char fqcn[SQL_FQCN_SIZE] = {0};

    strncpy (fqcn, name, SQL_FQCN_SIZE);

    char *str1 = strtok (fqcn, del);
    char *str2 = strtok (NULL, del);

    if (str1 && !str2 ) return true;

    return false;
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

    if (sql_is_name_FQCN (qep->join.table_alias, input_column_name)) {

        strncpy (table_name_out, str1, SQL_TABLE_NAME_MAX_SIZE);
        strncpy (col_name_out, str2, SQL_COLUMN_NAME_MAX_SIZE);
        return;
    }

    if (sql_is_name_ACN (qep->join.table_alias, input_column_name)) {

        auto it = qep->join.table_alias->find(str1);
        strncpy (table_name_out, it->second.c_str(), SQL_TABLE_NAME_MAX_SIZE);
        strncpy (col_name_out, str2, SQL_COLUMN_NAME_MAX_SIZE);
        return;
    }

    if (sql_is_name_LCN (input_column_name)) {

        strncpy (table_name_out, qep->join.tables[0].table_name, 
            SQL_TABLE_NAME_MAX_SIZE);
        strncpy (col_name_out, str1, SQL_COLUMN_NAME_MAX_SIZE);
        return;
    }

}

void 
sql_convert_name_to_FQCN (qep_struct_t *qep, 
                                                char *input_column_name,
                                                int input_column_name_size) {

    char table_name [SQL_TABLE_NAME_MAX_SIZE];
    char column_name [SQL_COLUMN_NAME_MAX_SIZE];
    
    assert (input_column_name_size >= SQL_FQCN_SIZE);

    if (sql_is_name_FQCN (qep->join.table_alias, input_column_name)) { 
        return;
    }

    sql_get_column_table_names (qep, input_column_name, table_name, column_name);
    snprintf (input_column_name, input_column_name_size, "%s.%s", table_name, column_name);
}
