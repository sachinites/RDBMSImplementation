#include "sql_api.h"
#include "../core/sql_utils.h"
#include "../../MathExpressionParser/Dtype.h"
#include "../../MathExpressionParser/MExprcppEnums.h"

void sql_record_reader (void *arg, std::vector<Dtype *> *vec) {

    int i;
    
    int column_width = 80 / vec->size();  

    for (i = 0; i < vec->size(); i++) {

        Dtype *dtype = vec->at(i);

        switch (dtype->did) {

            case MATH_CPP_STRING:
                printf("%-*s|", column_width, (dynamic_cast <Dtype_STRING*>(dtype))->dtype.str_val.c_str());
                break;

            case MATH_CPP_INT:
                printf("%-*d|", column_width, (dynamic_cast <Dtype_INT *>(dtype))->dtype.int_val);
                break;

            case MATH_CPP_DOUBLE:
                if (mexpr_double_is_integer ( (dynamic_cast <Dtype_DOUBLE *>(dtype))->dtype.d_val)) {
                    printf("%-*d|", column_width, (int)(dynamic_cast <Dtype_DOUBLE *>(dtype))->dtype.d_val);
                }
                else {
                    printf("%-*f|", column_width, (dynamic_cast <Dtype_DOUBLE *>(dtype))->dtype.d_val);
                }
                break;

            case MATH_CPP_BOOL:
                assert(0);
                break;
                
            case MATH_CPP_IPV4: {
                printf("%-*s|", column_width, (dynamic_cast <Dtype_IPv4_addr*>(dtype))->dtype.ip_addr_str.c_str());
                break;
            }

            case MATH_CPP_INTERVAL: 
            {
                Dtype_STRING *dtype_string = dtype->toString();
                std::string str = dtype_string->dtype.str_val;
                printf("%-*s|", column_width, str.c_str());
                delete dtype_string;
                break;
            }
            break;
            default:
                assert(0);
        }

    }
}

int 
main (int argc, char **argv) {

    std::string sel_query = "select * from quad";
    sql_select_exec (sel_query, sql_record_reader, NULL);
    return 0;
}