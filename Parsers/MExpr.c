#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>
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

static inline bool 
Math_is_unary_operator (int token_code) {

    /* Supported Operators In MatheMatical Expression */

    switch (token_code) {

        case SQL_MATH_SIN:
        case SQL_MATH_SQR:
        case SQL_MATH_SQRT:
        return true;
    }

    return false;
}

static inline bool 
Math_is_binary_operator (int token_code) {

    /* Supported Operators In MatheMatical Expression */

    switch (token_code) {

        case SQL_MATH_MAX:
        case SQL_MATH_MIN:
        case SQL_MATH_PLUS:
        case SQL_MATH_MINUS:
        case SQL_MATH_MUL:
        case SQL_MATH_DIV:
        case SQL_MATH_POW:
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
mexpr_is_white_space (int token_code) {

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

            if (mexpr_is_white_space (lex_data->token_code)) continue;

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

                    while (!isStackEmpty(stack)) {

                        lex_data = (lex_data_t *)StackGetTopElem(stack);

                        if (Math_is_unary_operator (lex_data->token_code)) {

                            lex_data_arr_out[out_index++] = (lex_data_t *)pop(stack);
                            continue;
                        }
                        break;
                    }
            }

            else if (lex_data->token_code == COMMA) {

                while (!isStackEmpty(stack) && 
                    (((lex_data_t *)stack->slot[stack->top])->token_code != BRACK_START)) {
                            lex_data_arr_out[out_index++] = (lex_data_t *)pop(stack);
                }
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
                int len,
                void *operand) {

    mexpt_node_t *mexpt_node;

    mexpt_node = (mexpt_node_t *)calloc (1, sizeof (mexpt_node_t));
    mexpt_node->token_code = token_id;

    if (Math_is_operator (token_id)) {
            return mexpt_node;
    }

    switch (token_id)
    {
    case SQL_IDENTIFIER:
    case SQL_IDENTIFIER_IDENTIFIER:
        strncpy(mexpt_node->variable_name, operand, len);
        mexpt_node->is_resolved = false;
        break;
    case SQL_INTEGER_VALUE:
        *(int *)mexpt_node->math_val = atoi(operand);
        mexpt_node->is_resolved = true;
        mexpt_node->dtype = SQL_INT;
        break;
    case SQL_FLOAT_VALUE:
        *(float *)mexpt_node->math_val = atof(operand);
        mexpt_node->is_resolved = true;
        mexpt_node->dtype = SQL_FLOAT;
        break;
    default:
        assert(0);
    }

    return mexpt_node;
}

mexpt_node_t *
mexpr_convert_postfix_to_expression_tree (
                                    lex_data_t **lex_data, int size) {

    int i;
    mexpt_node_t *mexpt_node;
    Stack_t *stack = get_new_stack();

    for (i = 0; i < size; i++) {

        if (!Math_is_operator(lex_data[i]->token_code)) {
        
            mexpt_node = mexpr_create_mexpt_node (
                                    lex_data[i]->token_code, lex_data[i]->token_len, lex_data[i]->token_val);
            push(stack, (void *)mexpt_node);
        } 

        else if (Math_is_binary_operator (lex_data[i]->token_code)){

            mexpt_node_t *right = pop(stack);
            mexpt_node_t *left = pop(stack);
            mexpt_node_t * opNode = mexpr_create_mexpt_node (
                                                        lex_data[i]->token_code, 0, NULL);
            opNode->left = left;
            opNode->right = right;
            push(stack, opNode);
        }

        else if (Math_is_unary_operator (lex_data[i]->token_code)){

            mexpt_node_t *left = pop(stack);
            mexpt_node_t * opNode = mexpr_create_mexpt_node (
                                                        lex_data[i]->token_code, 0, NULL);
            opNode->left = left;
            opNode->right = NULL;
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

    if (Math_is_operator (root->token_code)) {
        printf ("Mop (%d)  ", root->token_code);
    }
    else {
        printf ("Operand (%d)  ", root->token_code);
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
        free(root);
    }
}

res_t
mexpt_evaluate (mexpt_node_t *root) {

    res_t res;
    res.rc = false;
    res.value = NULL;

    if (!root) return res;

    res_t lrc = mexpt_evaluate (root->left);
    res_t rrc = mexpt_evaluate (root->right);

    /* If I am leaf */
    if (!root->left && !root->right) {

        assert (!Math_is_operator (root->token_code));

        if (!root->is_resolved) return res;

        /* Operand has been resolved*/
        res.dtype = root->dtype;

        switch (root->dtype) {

            case SQL_INT:
                res.value = root->math_val;
                break;
            case SQL_FLOAT:
                res.value = root->math_val;
                break;
            default:
                assert(0);
        }

        res.rc = true;
        return res;
    }

    /* If I am half node*/
    if (root->left && !root->right)
    {
        assert(Math_is_unary_operator(root->token_code));

        if (!lrc.rc) return res;

        switch (root->token_code)
        {
            case SQL_MATH_SIN:
            {
                double temp = sin(*(double *)lrc.value);
                *(double *)lrc.value = temp;
                res.dtype = SQL_FLOAT;
            }
            break;

            case SQL_MATH_SQRT:
            {
                double temp = sqrt (((*(double *)lrc.value)));
                *(double *)lrc.value = temp;
                res.dtype = SQL_FLOAT;
            }
            break;

            case SQL_MATH_SQR:
            {
                    switch (lrc.dtype)
                    {
                    case SQL_INT:
                    {
                        int temp = *(int *)lrc.value;
                        temp = temp * temp;
                        *(int *)lrc.value = temp;
                    }
                    break;
                    case SQL_FLOAT:
                    {
                        float temp = *(float *)lrc.value;
                        temp = temp * temp;
                        *(float *)lrc.value = temp;
                    }
                    break;
                    }
            }
            break;
        }
        res.value = lrc.value;
        res.rc = true;
        return res;
    }

    /* If I am Full node */
    if (!lrc.rc || !rrc.rc) return res;

    assert (Math_is_binary_operator (root->token_code));

    switch (root->token_code) {

        case SQL_MATH_MAX:
        {
            switch (lrc.dtype) {

                case SQL_INT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                res.value = *(int *)lrc.value < *(int *)rrc.value ? rrc.value : lrc.value;
                                res.dtype = SQL_INT;
                            }
                        break;
                        case SQL_FLOAT:
                            {
                                res.value = *(float *)lrc.value < *(float *)rrc.value ? rrc.value : lrc.value;
                                res.dtype = SQL_FLOAT;
                            }                        
                        break;
                    }
                break;
                case SQL_FLOAT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                res.value = *(float *)lrc.value < *(float *)rrc.value ? rrc.value : lrc.value;
                                res.dtype = SQL_FLOAT;
                            }                            
                        break;
                        case SQL_FLOAT:
                            {
                                res.value = *(float *)lrc.value < *(float *)rrc.value ? rrc.value : lrc.value;
                                res.dtype = SQL_FLOAT;
                            }                        
                        break;
                    }                    
                break;
            }
        }
        break;
        case SQL_MATH_MIN:
        {
            switch (lrc.dtype) {

                case SQL_INT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                res.value = *(int *)lrc.value < *(int *)rrc.value ? lrc.value : rrc.value;
                                res.dtype = SQL_INT;
                            }
                        break;
                        case SQL_FLOAT:
                            {
                                res.value = *(float *)lrc.value < *(float *)rrc.value ? lrc.value : rrc.value;
                                res.dtype = SQL_FLOAT;
                            }                        
                        break;
                    }
                break;
                case SQL_FLOAT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                res.value = *(float *)lrc.value < *(float *)rrc.value ? lrc.value : rrc.value;
                                res.dtype = SQL_FLOAT;
                            }                            
                        break;
                        case SQL_FLOAT:
                            {
                                res.value = *(float *)lrc.value < *(float *)rrc.value ? lrc.value : rrc.value;
                                res.dtype = SQL_FLOAT;
                            }                        
                        break;
                    }                    
                break;
            }
        }
        break;

        case SQL_MATH_PLUS:
        {
            switch (lrc.dtype) {

                case SQL_INT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                int temp = *(int *)lrc.value;
                                temp += *(int *)rrc.value;
                                *(int *)lrc.value = temp;
                                res.value = lrc.value;
                                res.dtype = SQL_INT;
                            }
                        break;
                        case SQL_FLOAT:
                            {
                                float temp = *(float *)lrc.value + *(float *)rrc.value ;
                                *(float *)lrc.value = temp;
                                 res.value = lrc.value;
                                res.dtype = SQL_FLOAT;
                            }                        
                        break;
                    }
                break;
                case SQL_FLOAT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                float temp = *(float *)lrc.value + *(float *)rrc.value ;
                                *(float *)lrc.value = temp;
                                 res.value = lrc.value;
                                 res.dtype = SQL_FLOAT;
                            }                            
                        break;
                        case SQL_FLOAT:
                            {
                                float temp = *(float *)lrc.value + *(float *)rrc.value ;
                                *(float *)lrc.value = temp;
                                 res.value = lrc.value;
                                res.dtype = SQL_FLOAT;
                            }                        
                        break;
                    }                    
                break;
            }            
        }
        break;


        case SQL_MATH_MINUS:
        {
            switch (lrc.dtype) {

                case SQL_INT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                *(int *)lrc.value = *(int *)rrc.value - *(int *)lrc.value;
                                res.value = lrc.value;
                                res.dtype = SQL_INT;
                            }
                        break;
                        case SQL_FLOAT:
                            {
                                 *(float *)lrc.value = *(float *)rrc.value - *(float *)lrc.value;
                                 res.value = lrc.value;
                                 res.dtype = SQL_FLOAT;
                            }                        
                        break;
                    }
                break;
                case SQL_FLOAT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                 *(float *)lrc.value = *(float *)rrc.value - *(float *)lrc.value;
                                 res.value = lrc.value;
                                 res.dtype = SQL_FLOAT;
                            }                            
                        break;
                        case SQL_FLOAT:
                            {
                                 *(float *)lrc.value = *(float *)rrc.value - *(float *)lrc.value;
                                 res.value = lrc.value;
                                 res.dtype = SQL_FLOAT;
                            }                        
                        break;
                    }                    
                break;
            }            
        }
        break;


        case SQL_MATH_MUL:
        {
            switch (lrc.dtype) {

                case SQL_INT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                int temp =  *(int *)lrc.value * *(int *)rrc.value;
                                *(int *)lrc.value = temp;
                                res.value = lrc.value;
                                res.dtype = SQL_INT;
                            }
                        break;
                        case SQL_FLOAT:
                            {
                                *(float *)lrc.value = *(float *)lrc.value * *(float *)rrc.value ;
                                res.value = lrc.value;
                                res.dtype = SQL_INT;
                            }                        
                        break;
                    }
                break;
                case SQL_FLOAT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                *(float *)lrc.value = *(float *)lrc.value * *(float *)rrc.value ;
                                res.value = lrc.value;
                                res.dtype = SQL_INT;
                            }                            
                        break;
                        case SQL_FLOAT:
                            {
                                *(float *)lrc.value = *(float *)lrc.value * *(float *)rrc.value ;
                                res.value = lrc.value;
                                res.dtype = SQL_INT;
                            }                        
                        break;
                    }                    
                break;
            }            
        }
        break;


        case SQL_MATH_DIV:
        {
            switch (lrc.dtype) {

                case SQL_INT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                *(int *)lrc.value = *(int *)rrc.value  / *(int *)lrc.value ;
                                res.value = lrc.value;
                                res.dtype = SQL_INT;
                            }
                        break;
                        case SQL_FLOAT:
                            {
                                *(float *)lrc.value = *(float *)rrc.value / *(float *)lrc.value ;
                                res.value = lrc.value;
                                res.dtype = SQL_INT;
                            }                        
                        break;
                    }
                break;
                case SQL_FLOAT:
                    switch (rrc.dtype) {
                        case SQL_INT:
                            {
                                *(float *)lrc.value = *(float *)rrc.value / *(float *)lrc.value ;
                                res.value = lrc.value;
                                res.dtype = SQL_INT;
                            }                            
                        break;
                        case SQL_FLOAT:
                            {
                                *(float *)lrc.value = *(float *)rrc.value / *(float *)lrc.value ;
                                res.value = lrc.value;
                                res.dtype = SQL_INT;
                            }                        
                        break;
                    }                    
                break;
            }            
        }
        break;


        case SQL_MATH_POW:
        {
            *(float *)lrc.value = pow (*(double *)rrc.value , *(double *)lrc.value);
            res.value = lrc.value;
            res.dtype = SQL_FLOAT;
        }
        break;

    }

    res.rc = true;
    return res;
}