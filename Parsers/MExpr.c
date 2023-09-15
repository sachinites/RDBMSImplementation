#include <stdbool.h>
#include <assert.h>
#include "SQLParserStruct.h"
#include "common.h"
#include "../stack/stack.h"
#include "MExpr.h"

static inline bool 
Math_is_operator (int token_code) {

    /* Supported Operators In MatheMatical Expression */

    switch (token_code) {

        case SQL_MATH_PLUS:
        case SQL_MATH_MINUS:
        case SQL_MATH_MUL:
        case SQL_MATH_DIV:
        case SQL_MATH_MAX:
        case SQL_MATH_MIN:
        case SQL_MATH_POW:
        case SQL_MATH_SIN:
        case SQL_MATH_SQR:
        case SQL_MATH_SQRT:
        case BRACK_START:
        return true;
    }

    return false;
}

/* Higher the returned value, higher the precedence. 
    Return Minimum value for '(*/
static int 
Math_operator_precedence (int token_code) {

    assert ( Math_is_operator (token_code));

    switch (token_code) {

        case SQL_MATH_PLUS:
        case SQL_MATH_MINUS:
            return 2;
        case SQL_MATH_MUL:
        case SQL_MATH_DIV:
            return 3;
        case SQL_MATH_MAX:
        case SQL_MATH_MIN:
        case SQL_MATH_POW:
            return 4;
        case SQL_MATH_SIN:
        case SQL_MATH_SQR:
        case SQL_MATH_SQRT:
            return 1;
        case BRACK_START:
            return 0;
    }
    printf ("token id - %d\n", token_code);
    assert(0);
    return 0;
} 

static bool 
mexpr_is_white_Space (int token_code) {

    return (token_code == 0 || token_code == EOL || token_code == WHITE_SPACE);
}

lex_data_t **
mexpr_convert_infix_to_postfix (lex_data_t *infix, int sizein, int *size_out) {

    int i;
    int out_index = 0;
    lex_data_t *lex_data;

    Stack_t *stack = get_new_stack();

    lex_data_t **lex_data_arr_out = 
        (lex_data_t**)calloc(MAX_MEXPR_LEN, sizeof(lex_data_t *));

    for (i = 0; i < sizein; i++) {

            lex_data = &infix[i];
            
            if (mexpr_is_white_Space (lex_data->token_code)) continue;
            if (lex_data->token_code == COMMA) continue;

            if (lex_data->token_code == BRACK_START)
            {
                    push(stack, (void *)lex_data);
            }
            else if (lex_data->token_code == BRACK_END)
            {
                    while (!isStackEmpty(stack) && 
                        (((lex_data_t *)stack->slot[stack->top])->token_code != BRACK_START)) {
                            lex_data_arr_out[out_index++] = (lex_data_t *)pop(stack);
                    }
                    pop(stack);
            }
             else if (!Math_is_operator(lex_data->token_code))
            {
                    lex_data_arr_out[out_index++] = lex_data;
            }
            else if (isStackEmpty (stack)) {
                push(stack, (void *)lex_data);
            }
            else
            {
                    while (!isStackEmpty(stack) &&
                        (Math_operator_precedence(lex_data->token_code) <= 
                          Math_operator_precedence(((lex_data_t *)stack->slot[stack->top])->token_code))) {
                        
                        lex_data_arr_out[out_index++] = (lex_data_t *)pop(stack);
                    }
                    push(stack, (void *)lex_data);
            }
    }

    while (!isStackEmpty(stack)) {
        lex_data_arr_out[out_index++] = (lex_data_t *)pop(stack);
    }

    *size_out = out_index;
    free_stack(stack);
    return lex_data_arr_out;
}

static mexpt_node_t*
mexpr_create_mexpt_node (
        int token_id,
        void *operand) {

    mexpt_node_t *mexpt_node;

    mexpt_node = (mexpt_node_t *)calloc (1, sizeof (mexpt_node_t));
    mexpt_node->lex_data.token_code = token_id;
    mexpt_node->lex_data.token_len = 0; // no use

    if (Math_is_operator (token_id)) {
            mexpt_node->lex_data.token_val = NULL;
    }
    else {
            assert(operand);
            mexpt_node->lex_data.token_val = operand;       
    }

    return mexpt_node;
}

mexpt_node_t *
mexpr_convert_postfix_to_expression_tree (
                                    lex_data_t **lex_data, int size) {

    int i;
    mexpt_node_t *mexpt_node;
    Stack_t *stack = get_new_stack();

    for (i = 0; lex_data[i]; ++i) {

        if (!Math_is_operator(lex_data[i]->token_code)) {
            mexpt_node = mexpr_create_mexpt_node (
                                    lex_data[i]->token_code, lex_data[i]->token_val);
            lex_data[i]->token_val = 0;
            push(stack, (void *)mexpt_node);

        } else {

            mexpt_node_t *right = pop(stack);
            mexpt_node_t *left = pop(stack);
            mexpt_node_t * opNode = mexpr_create_mexpt_node (
                                                        lex_data[i]->token_code, NULL);
            assert (!lex_data[i]->token_val);
            opNode->left = left;
            opNode->right = right;
            push(stack, opNode);
        }
    }

    mexpt_node_t *root = pop(stack);
    assert (isStackEmpty (stack));
    free_stack(stack);
    return root;
}

void 
mexpr_print_mexpt_node (mexpt_node_t *root) {

    if (Math_is_operator (root->lex_data.token_code)) {
        printf ("Mop (%d)", root->lex_data.token_code);
    }
    else {
        printf ("Operand (%d)", root->lex_data.token_code);
    }
}

/* Inorder traversal of expression tree print infix notation of 
    where clause */
void 
mexpr_debug_print_expression_tree (mexpt_node_t *root) {

    if (!root) return;
    mexpr_debug_print_expression_tree (root->left);
    mexpr_print_mexpt_node (root);
    mexpr_debug_print_expression_tree (root->right);
}

void 
mexpt_destroy(mexpt_node_t *root) {

    if (root != NULL) {

        mexpt_destroy(root->left);
        mexpt_destroy(root->right);

        if (Math_is_operator(root->lex_data.token_code)) {
            free(root);
        }
        else {
            free(root->lex_data.token_val);
            free(root);
        }
    }
}