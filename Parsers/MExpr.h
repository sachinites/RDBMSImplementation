#ifndef __MEXPR__
#define __MEXPR__

#include "ParserExport.h"

typedef struct mexpt_node_ {

    lex_data_t lex_data;
    struct mexpt_node_ *left;
    struct mexpt_node_ *right;

} mexpt_node_t;

lex_data_t **
mexpr_convert_infix_to_postfix (lex_data_t *infix, int sizein, int*size_out);

mexpt_node_t *
mexpr_convert_postfix_to_expression_tree (
                                    lex_data_t **lex_data, int size) ;

void 
mexpr_print_mexpt_node (mexpt_node_t *root);

void 
mexpr_debug_print_expression_tree (mexpt_node_t *root) ;

void 
mexpt_destroy(mexpt_node_t *root);

#endif 