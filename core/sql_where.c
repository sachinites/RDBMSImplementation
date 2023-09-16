#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include "sql_utils.h"
#include "sql_where.h"
#include "qplanner.h"
#include "sql_groupby.h"
#include "Catalog.h"
#include "../stack/stack.h"
#include "../Parsers/common.h"

bool 
sql_where_compare (void *lval , int lval_size, 
                                   void *rval, int rval_size,
                                    sql_dtype_t dtype,
                                    sql_op_t mop) {

    switch (dtype) {

        case SQL_INT:
            
            switch (mop) {

                case SQL_LESS_THAN:
                {
                    int lval_int = *(int *)lval;
                    int rval_int = *(int *)rval;
                    if (lval_int < rval_int) return true;
                    return false;
                }
                break;


                case SQL_LESS_THAN_EQ:
                {
                    int lval_int = *(int *)lval;
                    int rval_int = *(int *)rval;
                    if (lval_int <= rval_int) return true;
                    return false;
                }
                break;


                case SQL_GREATER_THAN:
                {
                    int lval_int = *(int *)lval;
                    int rval_int = *(int *)rval;
                    if (lval_int > rval_int) return true;
                    return false;
                }
                break;                


                case SQL_GREATER_THAN_EQ:
                {
                    int lval_int = *(int *)lval;
                    int rval_int = *(int *)rval;
                    if (lval_int >= rval_int) return true;
                    return false;
                }
                break;


                case SQL_EQ:
                {
                    int lval_int = *(int *)lval;
                    int rval_int = *(int *)rval;
                    if (lval_int == rval_int) return true;
                    return false;
                }
                break;


                case SQL_NOT_EQ:
                {
                    int lval_int = *(int *)lval;
                    int rval_int = *(int *)rval;
                    if (lval_int != rval_int) return true;
                    return false;
                }
                break;


                default:
                    assert(0);

            }   // switch (mop) ends
        break;




        case SQL_DOUBLE:

            switch (mop) {

                case SQL_LESS_THAN:
                {
                    double lval_double = *(double *)lval;
                    double rval_double = *(double *)rval;
                    if (lval_double < rval_double) return true;
                    return false;
                }
                break;


                case SQL_LESS_THAN_EQ:
                {
                    double lval_double = *(double *)lval;
                    double rval_double = *(double *)rval;
                    if (lval_double <= rval_double) return true;
                    return false;
                }
                break;


                case SQL_GREATER_THAN:
                {
                    double lval_double = *(double *)lval;
                    double rval_double = *(double *)rval;
                    if (lval_double > rval_double) return true;
                    return false;
                }
                break;                


                case SQL_GREATER_THAN_EQ:
                {
                    double lval_double = *(double *)lval;
                    double rval_double = *(double *)rval;
                    if (lval_double >= rval_double) return true;
                    return false;
                }
                break;


                case SQL_EQ:
                {
                    double lval_double = *(double *)lval;
                    double rval_double = *(double *)rval;
                    if (lval_double == rval_double) return true;
                    return false;
                }
                break;


                case SQL_NOT_EQ:
                {
                    double lval_double = *(double *)lval;
                    double rval_double = *(double *)rval;
                    if (lval_double != rval_double) return true;
                    return false;
                }
                break;


                default:
                    assert(0);

            }   // switch (mop) ends
        break;



        case SQL_STRING:

            switch (mop) {

                case SQL_EQ:
                {
                   // if (lval_size != rval_size) return false;
                    int rc = strncmp (lval, rval, lval_size);
                    if (rc == 0 ) return true;
                    return false;
                }
                break;


                case SQL_NOT_EQ:
                {
                  //  if (lval_size != rval_size) return true;
                    int rc = strncmp (lval, rval, lval_size);
                    if (rc == 0 ) return false;
                    return true;
                }
                break;


                default:
                    assert(0);

            }   // switch (mop) ends
        break;



        case SQL_IPV4_ADDR:

            switch (mop) {

                case SQL_LESS_THAN:
                {
                    uint32_t lval_ipv4 = *(uint32_t *)lval;
                    uint32_t rval_ipv4 = *(uint32_t *)rval;
                    if (lval_ipv4 < rval_ipv4) return true;
                    return false;
                }
                break;


                case SQL_LESS_THAN_EQ:
                {
                    uint32_t lval_ipv4 = *(uint32_t *)lval;
                    uint32_t rval_ipv4 = *(uint32_t *)rval;
                    if (lval_ipv4 <= rval_ipv4) return true;
                    return false;
                }
                break;


                case SQL_GREATER_THAN:
                {
                    uint32_t lval_ipv4 = *(uint32_t *)lval;
                    uint32_t rval_ipv4 = *(uint32_t *)rval;
                    if (lval_ipv4 > rval_ipv4) return true;
                    return false;
                }
                break;                


                case SQL_GREATER_THAN_EQ:
                {
                    uint32_t lval_ipv4 = *(uint32_t *)lval;
                    uint32_t rval_ipv4 = *(uint32_t *)rval;
                    if (lval_ipv4 >= rval_ipv4) return true;
                    return false;
                }
                break;


                case SQL_EQ:
                {
                    uint32_t lval_ipv4 = *(uint32_t *)lval;
                    uint32_t rval_ipv4 = *(uint32_t *)rval;
                    if (lval_ipv4 == rval_ipv4) return true;
                    return false;
                }
                break;


                case SQL_NOT_EQ:
                {
                    uint32_t lval_ipv4 = *(uint32_t *)lval;
                    uint32_t rval_ipv4 = *(uint32_t *)rval;
                    if (lval_ipv4 != rval_ipv4) return true;
                    return false;
                }
                break;


                default:
                    assert(0);

            }   // switch (mop) ends

        break;


        default:
            assert(0);
    }

    assert(0);
    return false;
}

bool 
sql_where_compute (qep_struct_t *qep_struct, where_cond_t *wc, joined_row_t *joined_row) {

    void *lval;
    void *rval;
    int l_value_size;
    int r_value_size;
    qp_col_t *lcol;
    qp_col_t *rcol;

    /* Compute lval*/
    lcol = &wc->col;
    l_value_size = lcol->schema_rec->dtype_size;
    lval = sql_get_column_value_from_joined_row (joined_row, lcol);
    if (!lval) return true;

    /* Compute rval*/
    switch (wc->right_op.w_opd) {

        case WH_COL:
            rcol = &wc->right_op.u.col;
            rval =  sql_get_column_value_from_joined_row (joined_row, rcol);
            if (!rval) return true;
            r_value_size = rcol->schema_rec->dtype_size;
            assert (lcol->schema_rec->dtype == rcol->schema_rec->dtype);
            return sql_where_compare (lval, l_value_size, rval, r_value_size, lcol->schema_rec->dtype, wc->op);

        case WH_VALUE:
            rval = wc->right_op.u.value.val;
            r_value_size = wc->right_op.u.value.size;
            assert (lcol->schema_rec->dtype == wc->right_op.u.value.dtype);
            return sql_where_compare (lval, l_value_size, rval, r_value_size, lcol->schema_rec->dtype, wc->op);

        default:
            assert(0);
    }
    
    assert(0);
    return false;
}

bool 
sql_evaluate_where_expression_tree (qep_struct_t *qep_struct, expt_node_t *root, joined_row_t *joined_row)  {

    switch (root->expt_node_type) {

        case LOG_OP:
            switch (root->u.lop) {
                case SQL_AND:
                {
                    bool rcl = sql_evaluate_where_expression_tree (qep_struct, root->left, joined_row);
                    if (rcl == false) return false;
                    return rcl && sql_evaluate_where_expression_tree (qep_struct, root->right, joined_row);
                }
                case SQL_OR:
                {
                    bool rcl = sql_evaluate_where_expression_tree (qep_struct, root->left, joined_row);
                    if (rcl == true) return true;
                    return sql_evaluate_where_expression_tree (qep_struct, root->right, joined_row);               
                }
                case SQL_NOT:
                    return ! ( sql_evaluate_where_expression_tree (qep_struct,
                                root->right ? root->right : root->left, joined_row));
            }
        break;
        case WHERE_COND:
            if (qep_struct->stage_id != QP_NODE_GROUP_BY) {
                return sql_where_compute (qep_struct, root->u.wc, joined_row);
            }
            else {
                return sql_having_compute (qep_struct, root->u.wc, joined_row);
            }
        default: ;
            assert(0);
            return false;
    }
    return false;
}

void 
sql_debug_print_where_cond (where_cond_t *wc) {

    switch (wc->right_op.w_opd)
    {
    case WH_COL:
            printf("Where Cond  : [%s   op=%d   %s]\n", wc->col.schema_rec->column_name, wc->op, wc->right_op.u.col.schema_rec->column_name);
            break;
    case WH_VALUE:
            assert(sql_valid_dtype(wc->right_op.u.value.dtype));
            switch (wc->right_op.u.value.dtype)
            {
            case SQL_STRING:
                    printf("Where Cond  : [%s   op=%d   %s]\n", wc->col.schema_rec->column_name, wc->op, (char *)wc->right_op.u.value.val);
                    break;
            case SQL_INT:
                    printf("Where Cond  : [%s   op=%d   %d]\n", wc->col.schema_rec->column_name, wc->op, *(int *)wc->right_op.u.value.val);
                    break;
            case SQL_DOUBLE:
                    printf("Where Cond  : [%s   op=%d   %f]\n", wc->col.schema_rec->column_name, wc->op,
                           *(double *)wc->right_op.u.value.val);
                    break;
            case SQL_IPV4_ADDR:
            {
                    unsigned char ip_addr_str[16] = {0};
                    inet_ntop(AF_INET, (void *)wc->right_op.u.value.val, ip_addr_str, 16);
                    printf("Where Cond  : [%s   op=%d   %s]\n", wc->col.schema_rec->column_name, wc->op, ip_addr_str);
            }
            break;
            default:
                    assert(0);
            }
            break;
    default:
            assert(0);
    }
}

static void 
sql_debug_print_where_literal (where_literal_t *wlit ) {

        switch (wlit->where_token_type) {
            case WHERE_LITERAL_OPERATOR_TOKEN_CODE:
                printf ("Logical Operator, token code = %d\n", wlit->u.token_id);
            break;
            case WHERE_LITERAL_WHERE_COND:
                sql_debug_print_where_cond (&wlit->u.wc);
            break;
            default:
                assert(0);
        }
}


void 
sql_debug_print_where_literals (where_literal_t *arr) {

    where_literal_t *wlit = &arr[0];

    while (!(wlit->where_token_type == WHERE_LITERAL_OPERATOR_TOKEN_CODE &&
                    wlit->u.token_id == EOL)) {

        sql_debug_print_where_literal (wlit);
        wlit++;
    }
}


void 
sql_debug_print_where_literals2 (where_literal_t **arr, int size) {

    int i;

    for (i = 0; i < size; i++) {
        sql_debug_print_where_literal (arr[i]);
    }

}

static int 
operator_precedence (sql_op_t sql_op) {

    switch (sql_op) {

        case SQL_OR:
            return 1;
        case SQL_AND:
            return 2;
        case SQL_NOT:
            return 3;
        case BRACK_START:
            return 0;
    }
    assert(0);
    return 0;
} 

static bool 
is_where_literal_match (where_literal_t *wlit, int token_code) {

    if (wlit->where_token_type == WHERE_LITERAL_WHERE_COND) return false;
    return wlit->u.token_id == token_code;
}

/* Generated by chatGPT */
where_literal_t **
sql_where_clause_infix_to_postfix (where_literal_t *wlit_arr_in, int *size_out)
{
    int out_index = 0;
    where_literal_t *wlit;

    Stack_t *stack = get_new_stack();

    where_literal_t **wlit_arr_out = 
        (where_literal_t **)calloc(SQL_MAX_WHERE_LITERAL_ARRAY_SIZE, sizeof(where_literal_t *));

    wlit = &wlit_arr_in[0];

    while (!(wlit->where_token_type == WHERE_LITERAL_OPERATOR_TOKEN_CODE &&
                    wlit->u.token_id == EOL)) {

               if (wlit->where_token_type == WHERE_LITERAL_WHERE_COND)
            {
                    wlit_arr_out[out_index++] = wlit;
            }
            else if (wlit->where_token_type == WHERE_LITERAL_OPERATOR_TOKEN_CODE &&
                     wlit->u.token_id == BRACK_START)
            {
                    push(stack, (void *)wlit);
            }
            else if (wlit->where_token_type == WHERE_LITERAL_OPERATOR_TOKEN_CODE &&
                     wlit->u.token_id == BRACK_END)
            {
                    while (!isStackEmpty(stack) &&
                           (!is_where_literal_match((where_literal_t *)stack->slot[stack->top], BRACK_START)))
                        wlit_arr_out[out_index++] = (where_literal_t *)pop(stack);
                    pop(stack);
                    
            }
            else
            {
                    while (!isStackEmpty(stack) &&
                                  (operator_precedence(wlit->u.token_id) <= 
                                   operator_precedence(((where_literal_t *)stack->slot[stack->top])->u.token_id)))
                        wlit_arr_out[out_index++] = (where_literal_t *)pop(stack);
                    push(stack, (void *)wlit);
            }
            wlit++;
    }

    while (!isStackEmpty(stack)) {
        wlit_arr_out[out_index++] = (where_literal_t *)pop(stack);
    }

    *size_out = out_index;
    free_stack(stack);
    return wlit_arr_out;
}

static expt_node_t*
sql_create_expt_node_from_where_literal (
        qep_struct_t *qep_struct,
        where_token_type_t where_token_type,
        int token_id,
        where_cond_t *wc) {

    expt_node_t *expt_node;

    expt_node = (expt_node_t *)calloc (1, sizeof (expt_node_t));

    switch (where_token_type) {

        case WHERE_LITERAL_OPERATOR_TOKEN_CODE:
            
            expt_node->expt_node_type = LOG_OP;
            expt_node->u.lop = token_id;
            return expt_node;
        case WHERE_LITERAL_WHERE_COND:
            assert(wc);
            expt_node->expt_node_type = WHERE_COND;
            expt_node->u.wc = (where_cond_t *)calloc (1, sizeof (where_cond_t));
            memcpy (expt_node->u.wc, wc, sizeof (*wc));

            expt_node->u.wc->col.grpby_col_to_select_col_linkage = 
                qp_col_lookup_identical  (qep_struct->select.sel_colmns, qep_struct->select.n, 
                &expt_node->u.wc->col);
            if (expt_node->u.wc->right_op.w_opd == WH_COL) {
                expt_node->u.wc->right_op.u.col.grpby_col_to_select_col_linkage = 
                    qp_col_lookup_identical  (qep_struct->select.sel_colmns, qep_struct->select.n, 
                    &expt_node->u.wc->right_op.u.col);
            }
            return expt_node;
    }
    return NULL;
}

expt_node_t *
sql_where_convert_postfix_to_expression_tree (qep_struct_t *qep_struct,
                                                                where_literal_t **wlit, int size) {

    int i;
    expt_node_t *expt_node;
    Stack_t *stack = get_new_stack();

    for (i = 0; wlit[i]; ++i) {

        if (wlit[i]->where_token_type == WHERE_LITERAL_WHERE_COND) {
            expt_node = sql_create_expt_node_from_where_literal (
                                    qep_struct,
                                    wlit[i]->where_token_type, 0, &wlit[i]->u.wc);
            push(stack, (void *)expt_node);
        } else {

            expt_node_t *right = pop(stack);
            expt_node_t *left = pop(stack);
            expt_node_t * opNode = sql_create_expt_node_from_where_literal (qep_struct,
                                                        wlit[i]->where_token_type, wlit[i]->u.token_id, NULL);
            opNode->left = left;
            opNode->right = right;
            push(stack, opNode);
        }
    }

    expt_node_t *root = pop(stack);
    assert (isStackEmpty (stack));
    free_stack(stack);
    return root;
}

void 
sql_debug_print_expt_node (expt_node_t *root) {

    printf ("Expt node  type : %s\n", root->expt_node_type == LOG_OP ? "LOG_OP" : "WHERE_COND");
    switch (root->expt_node_type) {
        case LOG_OP:
            printf ("\top(%d)\n", root->u.lop);
            break;
        case WHERE_COND:
            printf ("\t");
            sql_debug_print_where_cond (root->u.wc);
            break;
        default:
            assert(0);
    }
}

/* Inorder traversal of expression tree print infix notation of 
    where clause */
void 
sql_debug_print_expression_tree (expt_node_t *root) {

    if (!root) return;
    sql_debug_print_expression_tree (root->left);
    sql_debug_print_expt_node (root);
    sql_debug_print_expression_tree (root->right);
}

void 
expt_destroy(expt_node_t *root) {

    if (root != NULL) {

        expt_destroy(root->left);
        expt_destroy(root->right);

        if (root->expt_node_type == LOG_OP) {
            free(root);
        }
        else {
            free(root->u.wc);
            free(root);
        }
    }
}

void 
sql_where_literals_array_free (where_literal_t *where_literal_arr) {

    where_literal_t *wlit;

    wlit = &where_literal_arr[0];

    while (!(wlit->where_token_type == WHERE_LITERAL_OPERATOR_TOKEN_CODE &&
                    wlit->u.token_id == EOL)) {
        
        if (wlit->where_token_type == WHERE_LITERAL_WHERE_COND) {

            switch (wlit->u.wc.right_op.w_opd) {
                
                case WH_VALUE:
                    free (wlit->u.wc.right_op.u.value.val);
                    break;
                default: ;
            }
        }
        wlit++;
    }

    free (where_literal_arr);
}
