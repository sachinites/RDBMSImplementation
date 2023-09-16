#ifndef __MEXPR__
#define __MEXPR__

#include <stdint.h>
#include "ParserExport.h"
#include "../core/sql_const.h"

typedef struct mexpt_node_ {

    int token_code;
    /* Below fields are relevant only when this node is operand nodes*/
    bool is_resolved;   /* Have we obtained the math value of this operand*/
    double math_val;   /* Actual Math Value */
    uint8_t variable_name[SQL_COMPOSITE_COLUMN_NAME_SIZE];

    struct mexpt_node_ *left;
    struct mexpt_node_ *right;

} mexpt_node_t;

typedef struct res_{

    bool rc;
    double ovalue;
    
} res_t; 

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

res_t
mexpt_evaluate (mexpt_node_t *root);

bool 
double_is_integer (double d);

#endif 