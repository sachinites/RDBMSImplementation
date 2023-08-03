#ifndef __SQL_UTILS__
#define __SQL_UTILS__

typedef struct ast_node_ ast_node_t;

#include "rdbms_struct.h"
#include "../BPlusTreeLib/BPlusTree.h"

int 
rdbms_key_comp_fn (BPluskey_t *key_1, BPluskey_t *key_2, key_mdata_t *key_mdata, int size) ;

int
BPlusTree_key_format_fn_default (BPluskey_t *key, unsigned char *obuff, int buff_size) ;
int
BPlusTree_value_format_fn_default (void *value, unsigned char *obuff, int buff_size);

key_mdata_t *
sql_construct_table_key_mdata (ast_node_t *root, int *key_mdata_size);

#endif 