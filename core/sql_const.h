#ifndef __SQL_CONST__
#define __SQL_CONST__

#define SQL_TABLE_NAME_MAX_SIZE 32
#define SQL_COLUMN_NAME_MAX_SIZE 32
#define SQL_MAX_COLUMNS_SUPPORTED_PER_TABLE 16
#define SQL_BTREE_MAX_CHILDREN_CATALOG_TABLE 4
#define SQL_BTREE_MAX_CHILDREN_SCHEMA_TABLE 4
#define SQL_BTREE_MAX_CHILDREN_RDBMS_TABLE 4
#define SQL_MAX_PRIMARY_KEYS_SUPPORTED  SQL_MAX_COLUMNS_SUPPORTED_PER_TABLE
#define SQL_MAX_GROUP_BY_N_SUPPORTED    3
#define SQL_STRING_MAX_VAUE_LEN   256
#define SQL_MAX_JOIN_TABLES_SUPPORTED   3
#define SQL_MAX_AGG_FN_NAME_LEN 8
#define SQL_MAX_DELETE_ACCUMULATE_COUNT 5
#define SQL_MAX_COLS_IN_SELECT_LIST 128
#define SQL_MAX_COLS_IN_GROUPBY_LIST    16
#define SQL_MAX_COLS_IN_UPDATE_LIST 16
#define SQL_MAX_TABLES_IN_JOIN_LIST 8
#define SQL_MAX_ENTITY_NAME_LEN 64 
#define SQL_MAX_HT_KEY_SIZE 1024
#define SQL_FQCN_SIZE  \
    (SQL_TABLE_NAME_MAX_SIZE + SQL_COLUMN_NAME_MAX_SIZE + SQL_MAX_AGG_FN_NAME_LEN + 2)
#define SQL_ALIAS_NAME_LEN  SQL_FQCN_SIZE
#endif 