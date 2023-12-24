#ifndef __QPLANNER__
#define __QPLANNER__

typedef struct catalog_table_value ctable_val_t ;
typedef struct qp_col_  qp_col_t;

#include "sql_const.h"

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

} qep_struct_t;

#endif 