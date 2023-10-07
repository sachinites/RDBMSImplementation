#ifndef __CATALOG__
#define __CATALOG__

#include <stdbool.h>
#include "sql_const.h"
#include "rdbms_struct.h"
#include "../SqlParser/SqlParserStruct.h"

typedef struct BPlusTree BPlusTree_t ;
typedef struct BPluskey BPluskey_t;
typedef struct BPlusTreeNode BPlusTreeNode ;
typedef struct sql_create_data_ sql_create_data_t ;


typedef struct schema_rec_ {

    char column_name [SQL_COLUMN_NAME_MAX_SIZE];
    sql_dtype_t dtype;
    int dtype_size;
    bool is_primary_key;
    bool is_non_null;
    int offset;

} schema_rec_t;

typedef enum schema_scope {

    PUBLIC

} schema_scope;

typedef enum catalog_entry_type_ {

    TABLE

} catalog_entry_type_t;

#pragma pack (push,1)
typedef struct catalog_table_key {

    schema_scope scope;
     char entity_name [SQL_MAX_ENTITY_NAME_LEN];
    catalog_entry_type_t type;
    char owner[32];

} catalog_table_key_t;
#pragma pack(pop)


typedef struct catalog_table_value {

    char table_name [SQL_TABLE_NAME_MAX_SIZE];
    BPlusTree_t  *rdbms_table;
    BPlusTree_t  *schema_table;
    glthread_t col_list_head;
    
} ctable_val_t;

bool 
Catalog_insert_new_table (BPlusTree_t *catalog, sql_create_data_t *cdata) ;

int
Catalog_create_schema_table_records ( sql_create_data_t *cdata,
                                                        BPluskey_t ***bkeys,
                                                        schema_rec_t ***crecords) ;

void 
sql_show_table_catalog (BPlusTree_t *TableCatalog) ;

ctable_val_t *
sql_catalog_table_lookup_by_table_name (BPlusTree_t *TableCatalog, char *entity_name);

#endif 
