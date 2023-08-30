#ifndef __SQL_UTILS__
#define __SQL_UTILS__

typedef struct ast_node_ ast_node_t;

#include "../Parsers/SQLParserStruct.h"
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

void 
sql_compute_aggregate (sql_agg_fn_t agg_fn, 
                                        void *src, void *dst, 
                                        sql_dtype_t dtype, 
                                        int dype_size,
                                        int row_no);

void 
sql_compute_column_text_name (qp_col_t *col, unsigned char *column_name, int size) ;

void *
sql_get_column_value_from_joined_row (joined_row_t *joined_row, qp_col_t *col, int table_cnt);

void 
parser_split_table_column_name ( unsigned char *composite_col_name, 
                                                        unsigned char *table_name_out,
                                                        unsigned char *col_name_out);

#endif 