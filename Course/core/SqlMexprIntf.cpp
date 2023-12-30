#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "sql_const.h"
#include "SqlMexprIntf.h"
#include "rdbms_struct.h"
#include "Catalog.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "../../SqlParser/ParserExport.h"
#include "../../../MathExpressionParser/MexprTree.h"
#include "../../../MathExpressionParser/Dtype.h"

parse_rc_t E (); 
extern lex_data_t **
mexpr_convert_infix_to_postfix (lex_data_t *infix, int sizein, int *size_out) ;

static void
postfix_array_free(lex_data_t **lex_data_array, int size) {
   
    for (int i = 0; i < size; i++) {

        if (lex_data_array[i])
        {
            if (lex_data_array[i]->token_val) free(lex_data_array[i]->token_val);
            free(lex_data_array[i]);
        }
    }

    free(lex_data_array);
}


// " <token> a + b"
static MexprTree *
Parser_Mexpr_build_math_expression_tree () {

    int i;
    MexprTree *tree = NULL;

    int stack_chkp = undo_stack.top + 1;
    parse_rc_t err = E();

    if (err == PARSE_ERR) {
        return NULL;
    }

    int size_out = 0;
    lex_data_t **postfix = mexpr_convert_infix_to_postfix (
                                            &undo_stack.data[stack_chkp],  undo_stack.top + 1 - stack_chkp, &size_out);

    tree = new MexprTree (postfix, size_out);
    postfix_array_free (postfix, size_out);
    return tree;
}


sql_exptree_t *
sql_create_exp_tree_compute () {

    sql_exptree_t *sql_exptree = ( sql_exptree_t *) calloc (1, sizeof (sql_exptree_t));
    sql_exptree->tree = Parser_Mexpr_build_math_expression_tree ();
    if (!sql_exptree->tree) {
        free(sql_exptree);
        return NULL;
    }
    
    if (!sql_exptree->tree->validate (sql_exptree->tree->root )) {

        printf ("Error : %s(%d) Expression Tree doesnt pass Validation test\n", 
                __FUNCTION__, __LINE__);
        sql_exptree->tree->destroy (sql_exptree->tree->root);
        free(sql_exptree);
        return NULL;
    }

    return sql_exptree;
}

void 
sql_destroy_exp_tree (sql_exptree_t *tree) {

    tree->tree->destroy(tree->tree->root);
    free(tree);
}

bool 
sql_resolve_exptree (BPlusTree_t *tcatalog,
                                  sql_exptree_t *sql_exptree,
                                  qep_struct_t *qep,
                                  joined_row_t **joined_row) {

   return true;
}