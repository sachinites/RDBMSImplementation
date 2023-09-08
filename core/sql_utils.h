#ifndef __SQL_UTILS__
#define __SQL_UTILS__

typedef struct ast_node_ ast_node_t;

#include <string.h>
#include "../Parsers/SQLParserStruct.h"
#include "rdbms_struct.h"
#include "../BPlusTreeLib/BPlusTree.h"

/* HashTable Setup */
#define HASH_PRIME_CONST    5381

static unsigned int 
hashfromkey(void *key) {

    unsigned char *str = (unsigned char *)key;
    unsigned int hash = HASH_PRIME_CONST;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static int 
equalkeys(void *k1, void *k2)
{
    char *ky1 = (char *)k1;
    char *ky2 = (char *)k2;
    int len1 = strlen(ky1);
    int len2 = strlen(ky2);
    if (len1 != len2) return len1 - len2;
    return (0 == memcmp(k1,k2, len1));
}


int 
rdbms_key_comp_fn (BPluskey_t *key_1, BPluskey_t *key_2, key_mdata_t *key_mdata, int size) ;

int
BPlusTree_key_format_fn_default (BPluskey_t *key, unsigned char *obuff, int buff_size) ;
int
BPlusTree_value_format_fn_default (void *value, unsigned char *obuff, int buff_size);

key_mdata_t *
sql_construct_table_key_mdata (ast_node_t *root, int *key_mdata_size);

void 
sql_compute_aggregate (sql_agg_fn_t agg_fn, 
                                        void *src, void *dst, 
                                        sql_dtype_t dtype, 
                                        int dype_size,
                                        int row_no);

                                    
void 
sql_compute_column_text_name (qp_col_t *col, unsigned char *column_name, int size) ;

void *
sql_get_column_value_from_joined_row (joined_row_t *joined_row, qp_col_t *col);

void 
parser_split_table_column_name ( unsigned char *composite_col_name, 
                                                        unsigned char *table_name_out,
                                                        unsigned char *col_name_out);


bool 
qp_col_is_equal (qp_col_t *col1, qp_col_t *col2);

qp_col_t *
qp_col_lookup_identical (qp_col_t **col_list, int size, qp_col_t *key_col) ;

#endif 