#ifndef __SQLPARSER_STRUCT__
#define __SQLPARSER_STRUCT__

#include "../../MathExpressionParser/MexprEnums.h"
#include "../core/sql_const.h"

typedef enum sql_entity_ {

    SQL_IDENTIFIER = MATH_IDENTIFIER,
    SQL_QUERY_TYPE = MEXPR_LIB_MAX_CODE_USED,
    SQL_AGG_FN,
    SQL_KEYWORD ,
    SQL_OPERATOR,
    SQL_DTYPE,
    
} sql_entity_type_t;

typedef enum sql_query_type_{

    SQL_SELECT_Q = MEXPR_LIB_MAX_CODE_USED + 11,
    SQL_UPDATE_Q ,
    SQL_CREATE_Q,
    SQL_DELETE_Q,
    SQL_INSERT_Q ,
    SQL_UNSUPPORTED_Q

} sql_query_type_t;

typedef enum sql_agg_fn_ {

    SQL_SUM = MEXPR_LIB_MAX_CODE_USED + 21,
    SQL_MIN,
    SQL_MAX,
    SQL_COUNT,
    SQL_AVG,
    SQL_AGG_FN_NONE,
 
} sql_agg_fn_t;

typedef enum sql_keywords_ {

    SQL_FROM = MEXPR_LIB_MAX_CODE_USED + 31,
    SQL_WHERE,
    SQL_GROUP_BY,
    SQL_HAVING,
    SQL_ORDER_BY,
    SQL_LIMIT,
    SQL_PRIMARY_KEY,
    SQL_NOT_NULL,
    SQL_SELECT,
    SQL_AS,
    SQL_KEYWORD_MAX,
    
} sql_keywords_t;

typedef enum sql_op_ {

    SQL_LESS_THAN = MATH_LESS_THAN,
    SQL_LESS_THAN_EQ  = MATH_LESS_THAN_EQ,
    SQL_GREATER_THAN = MATH_GREATER_THAN,
    SQL_EQ  = MATH_EQ,
    SQL_NOT_EQ = MATH_NOT_EQ,
    SQL_AND  = MATH_AND,
    SQL_OR = MATH_OR,
    SQL_GREATER_THAN_EQ = MEXPR_LIB_MAX_CODE_USED + 41,
    SQL_NOT,
    SQL_IN,
    SQL_BETWEEN, 
    SQL_OP_MAX

} sql_op_t;

typedef struct ident_ {

    unsigned char name [SQL_COMPOSITE_COLUMN_NAME_SIZE];
} str_identifier_t;

typedef enum sql_dtype_{

    SQL_DTYPE_FIRST = MEXPR_LIB_MAX_CODE_USED + 51,
    SQL_STRING,
    SQL_INT,
    SQL_DOUBLE,
    SQL_IPV4_ADDR,
    SQL_DTYPE_MAX

} sql_dtype_t;

typedef enum sql_dtype_attr_ {

    SQL_DTYPE_LEN = MEXPR_LIB_MAX_CODE_USED + 61,
    SQL_DTYPE_PRIMARY_KEY,
    SQL_DTYPE_NOT_NULL

} sql_dtype_attr_t;

typedef enum sql_ident_type_ {

    SQL_INTEGER_VALUE = MATH_INTEGER_VALUE,
    SQL_STRING_VALUE  = MATH_STRING_VALUE,
    SQL_DOUBLE_VALUE = MATH_DOUBLE_VALUE,
    SQL_IDENTIFIER_IDENTIFIER = MATH_IDENTIFIER_IDENTIFIER,
    SQL_IPV4_ADDR_VALUE = MEXPR_LIB_MAX_CODE_USED + 71,
    SQL_IDNT_TYPE_MAX

} sql_identifier_type_t;


/* Math Functions */
typedef enum math_fns_ {

    SQL_MATH_MAX = MATH_MAX,      // max(a,b)
    SQL_MATH_MIN = MATH_MIN,                // min(a,b)
    SQL_MATH_PLUS = MATH_PLUS,              //  a + b
    SQL_MATH_MINUS = MATH_MINUS,          //  a - b
    SQL_MATH_MUL = MATH_MUL,               //  a * b
    SQL_MATH_DIV = MATH_DIV,                 // a / b
    SQL_MATH_SQRT = MATH_SQRT,              // sqrt (a)
    SQL_MATH_SQR = MATH_SQR,                // sqr(a)
    SQL_MATH_SIN = MATH_SIN,                 // sin(a)
    SQL_MATH_POW = MATH_POW,               // pow(a,b) : a power b
    /* Reserved enums*/
    SQL_MATH_FNS_MAX = MEXPR_LIB_MAX_CODE_USED + 81

} math_fns_t;

typedef enum sql_order_ {

    SQL_ORDERBY_ASC = MEXPR_LIB_MAX_CODE_USED + 91,
    SQL_ORDERBY_DSC

} sql_order_t;


typedef enum sql_misc_ {

    SQL_COMMA = MATH_COMMA,
    SQL_BRACKET_START = MATH_BRACKET_START,
    SQL_BRACKET_END = MATH_BRACKET_END,
    SQL_QUOTATION_MARK = MEXPR_LIB_MAX_CODE_USED + 101,

} sql_misc_t;

typedef enum sql_short_commands_ {

    SHOW_DB_TABLES = MEXPR_LIB_MAX_CODE_USED + 110
    
} sql_short_commands_t;

static inline int
sql_valid_dtype (int dtype) {

    switch (dtype) {
        case SQL_STRING:
        case SQL_INT:
        case SQL_DOUBLE:
        case SQL_IPV4_ADDR:
        return 1;
    }
    return 0;
}

static inline const char *
sql_dtype_str (sql_dtype_t dtype) {

    switch (dtype) {
        case SQL_STRING:
            return "SQL_STRING";
        case SQL_INT:
            return "SQL_INT";
        case SQL_DOUBLE:
            return "SQL_DOUBLE";
        case SQL_IPV4_ADDR:
            return "SQL_IPV4_ADDR";
    }
    return 0;    
}

static inline int
sql_dtype_size (sql_dtype_t dtype) {

    switch (dtype)
    {
    case SQL_STRING:
        return 1;
    case SQL_INT:
        return 4;
    case SQL_DOUBLE:
        return sizeof(double);
    case SQL_IPV4_ADDR:
        return 4;
    }
    return 0;
}

static inline const char *
sql_agg_fn_tostring (sql_agg_fn_t agg_fn) {

    switch (agg_fn) {
        case SQL_SUM:
            return "sum";
        case SQL_MIN:
            return "min";
        case SQL_MAX:
            return "max";
        case SQL_COUNT:
            return "count";
        case SQL_AVG:
            return "avg";
    }
    return "";
}

#endif 
