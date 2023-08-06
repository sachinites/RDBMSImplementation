
typedef struct schema_rec_ schema_rec_t;
typedef struct BPlusTree BPlusTree_t;
typedef struct _glthread glthread_t;

void 
sql_print_hdr (glthread_t *col_list_head );

void 
 sql_emit_select_output (BPlusTree_t *schema_table, 
                                        glthread_t *col_list_head, 
                                        void *record_ptr);