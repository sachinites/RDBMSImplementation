#ifndef __SQL_MEXPR_INTF__
#define __SQL_MEXPR_INTF__

class MexprTree;

typedef struct sql_exptree_ {

    MexprTree *tree;

} sql_exptree_t;


sql_exptree_t *
sql_create_exptree_compute ();

#endif 