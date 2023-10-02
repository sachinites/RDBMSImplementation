#ifndef __SQL_MEXPR_INTF__
#define __SQL_MEXPR_INTF__

#include <stdbool.h>

typedef struct mexpt_tree_ mexpt_tree_t;
typedef struct catalog_table_value ctable_val_t;
typedef struct BPlusTree BPlusTree_t ;
typedef struct joined_row_ joined_row_t;
typedef struct qep_struct_ qep_struct_t;
typedef struct qp_col_ qp_col_t;

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
                                  qep_struct_t *qep,
                                  joined_row_t *joined_row) ;


bool 
sql_resolve_exptree_against_table (sql_exptree_t *sql_exptree, 
                                                         ctable_val_t * ctable_val, 
                                                         int table_id, joined_row_t *joined_row);
                                                         
bool 
sql_evaluate_conditional_exp_tree (sql_exptree_t *sql_exptree);

mexpr_var_t 
sql_evaluate_exp_tree (sql_exptree_t *sql_exptree);

sql_exptree_t *
sql_create_exp_tree_for_one_operand (unsigned char *opnd_name) ;

bool 
sql_is_expression_tree_only_operand (sql_exptree_t *sql_exptree);


#endif 