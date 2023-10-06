#ifndef __SQL_MEXPR_INTF__
#define __SQL_MEXPR_INTF__

#include <stdbool.h>

class MexprTree;
class Dtype;
typedef struct catalog_table_value ctable_val_t;
typedef struct BPlusTree BPlusTree_t ;
typedef struct joined_row_ joined_row_t;
typedef struct qep_struct_ qep_struct_t;
typedef struct qp_col_ qp_col_t;

#include "../../MathExpressionParser/MexprTree.h"

typedef struct sql_exptree_ {

    MexprTree *tree;

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

Dtype *
sql_evaluate_exp_tree (sql_exptree_t *sql_exptree);

sql_exptree_t *
sql_create_exp_tree_for_one_operand (unsigned char *opnd_name) ;

#define SqlExprTree_Iterator_Operands_Begin(tree_ptr, dtype_var_ptr)          \
    { MexprNode*_next_node = NULL;  dtype_var_ptr = NULL;                      \
        MexprNode* node_ptr = NULL;                                                                 \
    for (node_ptr = tree_ptr->tree->lst_head;  node_ptr; node_ptr = _next_node){       \
        _next_node = node_ptr->lst_right;                                                           \
         dtype_var_ptr = dynamic_cast <Dtype_VARIABLE *>(node_ptr);


#define SqlExprTree_Iterator_Operands_End }}


#endif 
