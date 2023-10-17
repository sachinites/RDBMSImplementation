#ifndef __QPLANNER__
#define __QPLANNER__

#include "../Tree/libtree.h"
#include "../SqlParser/SqlParserStruct.h"
#include "sql_const.h"
#include "rdbms_struct.h"

typedef struct catalog_table_value ctable_val_t ;
typedef struct schema_rec_ schema_rec_t ;
typedef struct BPlusTree BPlusTree_t;
typedef  struct hashtable hashtable_t;
typedef struct stack Stack_t;
typedef struct sql_exptree_ sql_exptree_t;

typedef enum qp_stage_id {

    QP_NODE_SEQ_SCAN,
    QP_NODE_WHERE,
    QP_NODE_JOIN,
    QP_NODE_JOIN_PREDICATE,
    QP_NODE_GROUP_BY,
    QP_NODE_HAVING,
    QP_NODE_SELECT,
    QP_NODE_DISTINCT,
    QP_NODE_ORDER_BY,
    QP_NODE_LIMIT,
    QP_NODE_OUTPUT,
    QP_NODE_MAX

} qp_stage_id_t;

typedef struct table_iter_data_ {

    BPlusTreeNode *bpnode;
    int index;
    ctable_val_t *ctable_val;
} table_iter_data_t;

typedef struct table_iterators_ {

    int table_cnt;
    table_iter_data_t table_iter_data[0];

} table_iterators_t;

typedef struct qep_struct_ {

    qp_stage_id_t stage_id;
    
    struct {

        int table_cnt;

        struct {

            ctable_val_t *ctable_val;
            char alias_name[SQL_ALIAS_NAME_LEN];

        } tables [SQL_MAX_TABLES_IN_JOIN_LIST];

    } join;

    struct {
            
            sql_exptree_t *gexptree;
            sql_exptree_t *exptree_per_table[SQL_MAX_TABLES_IN_JOIN_LIST];

    } where;

    struct {

        int n;
        qp_col_t *col_list[SQL_MAX_COLS_IN_GROUPBY_LIST];
        hashtable_t *ht;
        int ht_key_size;
        
    } groupby;

    struct {

         sql_exptree_t *gexptree_phase1;
         sql_exptree_t *gexptree_phase2;
         int8_t having_phase; // either 1 or 2

    } having;

    struct {

        int n;
        qp_col_t *sel_colmns[SQL_MAX_COLS_IN_SELECT_LIST];

    } select;

    struct {

        bool distinct;
        qp_col_t *col;

    } distinct;

    struct {

        bool orderby;
        qp_col_t *col;
        /* AVL tree to store records ordered by 'order by'* field. Default is FCFS*/
        avltree_t avl_order_by_root;
    } orderby;

    uint32_t limit;  /* 0 means no limit*/

    /* other variables*/
    bool is_join_started;
    bool is_join_finished;
    table_iterators_t *titer;

    joined_row_t *joined_row_tmplate;
} qep_struct_t;


void
sql_execute_qep (qep_struct_t *qep);

void 
qep_deinit (qep_struct_t *qep);

bool 
qep_struct_record_table (qep_struct_t *qep,  char *table_name);

void 
sql_process_select_query (qep_struct_t *qep) ;

#endif 
