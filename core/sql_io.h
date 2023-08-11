#ifndef __SQL_IO__
#define __SQL_IO__

typedef struct BPlusTree BPlusTree_t;
typedef struct qp_col_ qp_col_t;

void 
sql_print_hdr (qp_col_t *col_list, int n_cols );

void sql_emit_select_output(BPlusTree_t *schema_table,
                                              int n_col,
                                              qp_col_t *col_list_head,
                                              void *record_ptr) ;

#endif 