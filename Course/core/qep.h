#ifndef __QPLANNER__
#define __QPLANNER__

#include <stdbool.h>
#include <list>
#include <vector>
#include "sql_const.h"

typedef struct catalog_table_value ctable_val_t ;
typedef struct qp_col_  qp_col_t;
typedef struct BPlusTreeNode BPlusTreeNode;
typedef struct joined_row_  joined_row_t;
typedef struct table_iterators_ table_iterators_t;
typedef struct exp_tree_data_src_ exp_tree_data_src_t;

typedef struct qep_struct_
{
    struct
    {
        int table_cnt;
        struct
        {
            ctable_val_t *ctable_val;
            char table_name[SQL_TABLE_NAME_MAX_SIZE];

        } tables[SQL_MAX_TABLES_IN_JOIN_LIST];

    } join;

    struct
    {
        int n;
        qp_col_t *sel_colmns[SQL_MAX_COLS_IN_SELECT_LIST];

    } select;

    bool is_join_started;
    bool is_join_finished;
    table_iterators_t *titer;

    joined_row_t *joined_row_tmplate;

    std::list<exp_tree_data_src_t *> *data_src_lst;

} qep_struct_t;

void
sql_execute_qep (qep_struct_t *qep) ;

void 
qep_deinit (qep_struct_t *qep);

#endif 