#ifndef __SQL_MEXPR_INTF__
#define __SQL_MEXPR_INTF__

#include <stdbool.h>
#include <stdint.h>

class MexprTree;
class Dtype;
class MexprNode;
typedef struct catalog_table_value ctable_val_t;
typedef struct BPlusTree BPlusTree_t ;
typedef struct joined_row_ joined_row_t;
typedef struct qep_struct_ qep_struct_t;
typedef struct qp_col_ qp_col_t;

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
sql_create_exp_tree_for_one_operand (std::string opnd_name) ;

bool 
sql_is_expression_tree_only_operand (sql_exptree_t *sql_exptree);

bool 
sql_opnd_node_is_resolved (MexprNode *opnd_node);

uint8_t 
 sql_tree_remove_unresolve_operands(sql_exptree_t *sql_exptree);

std::string 
sql_get_opnd_variable_name (MexprNode *opnd_node);

sql_exptree_t *
sql_clone_expression_tree  (sql_exptree_t *src_tree);

bool 
sql_concatenate_expr_trees (sql_exptree_t *parent_tree, 
                                                MexprNode *opnd_node,
                                                sql_exptree_t *child_tree);

void 
sql_destrpy_exp_tree (sql_exptree_t *tree);

MexprNode *
sql_tree_get_first_operand (sql_exptree_t *tree);

MexprNode *
sql_tree_get_next_operand (MexprNode *node);

#define SqlExprTree_Iterator_Operands_Begin(tree_ptr, node_ptr) \
    for (node_ptr = sql_tree_get_first_operand(tree_ptr) ;  \
            node_ptr;   \
            node_ptr = sql_tree_get_next_operand (sql_tree_get_next_operand (node_ptr)))

MexprNode *
sql_tree_get_root (sql_exptree_t *tree) ;

void 
InstallDtypeOperandProperties (MexprNode *node, void *data_src, Dtype *(*compute_fn_ptr)(void *)) ;

#endif 
