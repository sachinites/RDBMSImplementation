#ifndef __SQL_MEXPR_INTF__
#define __SQL_MEXPR_INTF__

#include <stdbool.h>

typedef struct mexpt_tree_ mexpt_tree_t;
typedef struct catalog_table_value ctable_val_t;
typedef struct BPlusTree BPlusTree_t ;
typedef struct joined_row_ joined_row_t;

#include "../../MathExpressionParser/MexprEnums.h"

typedef struct sql_exptree_ {

    mexpt_tree_t *tree;

} sql_exptree_t;

sql_exptree_t *
sql_create_exp_tree_compute () ;

sql_exptree_t *
sql_create_exp_tree_conditional () ;

bool 
sql_resolve_exptree (BPlusTree_t *tcatalog,
                                  sql_exptree_t *sql_exptree,
                                  ctable_val_t **table_arr, int n,
                                  joined_row_t *joined_row) ;


bool 
sql_resolve_exptree_against_table (sql_exptree_t *sql_exptree, 
                                                         ctable_val_t * ctable_val, 
                                                         int table_id, joined_row_t *joined_row);
                                                         
bool 
sql_evaluate_conditional_exp_tree (sql_exptree_t *sql_exptree);

#endif 