#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "sql_utils.h"
#include "sql_where.h"
#include "qplanner.h"
#include "Catalog.h"

static bool 
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




        case SQL_FLOAT:

            switch (mop) {

                case SQL_LESS_THAN:
                {
                    float lval_float = *(float *)lval;
                    int rval_float = *(int *)rval;
                    if (lval_float < rval_float) return true;
                    return false;
                }
                break;


                case SQL_LESS_THAN_EQ:
                {
                    float lval_float = *(float *)lval;
                    int rval_float = *(int *)rval;
                    if (lval_float <= rval_float) return true;
                    return false;
                }
                break;


                case SQL_GREATER_THAN:
                {
                    float lval_float = *(float *)lval;
                    int rval_float = *(int *)rval;
                    if (lval_float > rval_float) return true;
                    return false;
                }
                break;                


                case SQL_GREATER_THAN_EQ:
                {
                    float lval_float = *(float *)lval;
                    int rval_float = *(int *)rval;
                    if (lval_float >= rval_float) return true;
                    return false;
                }
                break;


                case SQL_EQ:
                {
                    float lval_float = *(float *)lval;
                    int rval_float = *(int *)rval;
                    if (lval_float == rval_float) return true;
                    return false;
                }
                break;


                case SQL_NOT_EQ:
                {
                    float lval_float = *(float *)lval;
                    int rval_float = *(int *)rval;
                    if (lval_float != rval_float) return true;
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
                    if (lval_size != rval_size) return false;
                    int rc = strncmp (lval, rval, lval_size);
                    if (rc == 0 ) return true;
                    return false;
                }
                break;


                case SQL_NOT_EQ:
                {
                    if (lval_size != rval_size) return true;
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
sql_where_compute (where_cond_t *wc, joined_row_t *joined_row) {

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
    assert (!lval);

    /* Compute rval*/
    switch (wc->right_op.wh_opd_type) {

        case WH_COL:
            rcol = &wc->right_op.u.col;
            rval =  sql_get_column_value_from_joined_row (joined_row, rcol);
            r_value_size = rcol->schema_rec->dtype_size;
            assert (lcol->schema_rec->dtype == rcol->schema_rec->dtype);
            return sql_where_compare (lval, l_value_size, rval, r_value_size, lcol->schema_rec->dtype, wc->op);

        case WH_VAUE:
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
