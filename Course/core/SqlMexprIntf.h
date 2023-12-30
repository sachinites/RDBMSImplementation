#ifndef __SQL_MEXPR_INTF__
#define __SQL_MEXPR_INTF__
#include <stdbool.h>

typedef struct BPlusTree BPlusTree_t ;
typedef struct joined_row_ joined_row_t;
typedef struct qep_struct_ qep_struct_t;

class MexprTree;

typedef struct sql_exptree_ {

    MexprTree *tree;

} sql_exptree_t;


sql_exptree_t *
sql_create_exp_tree_compute ();

void 
sql_destroy_exp_tree (sql_exptree_t *tree);

bool 
sql_resolve_exptree (BPlusTree_t *tcatalog,
                                  sql_exptree_t *sql_exptree,
                                  qep_struct_t *qep,
                                  joined_row_t **joined_row) ;

#endif 