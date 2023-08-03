#ifndef __SQLPARSER_STRUCT__
#define __SQLPARSER_STRUCT__

#define SQL_IDENTIFIER_LEN  64

typedef enum sql_entity_ {

    SQL_QUERY_TYPE = 1,
    SQL_AGG_FN = 2,
    SQL_KEYWORD = 3,
    SQL_OPERATOR = 4,
    SQL_IDENTIFIER = 5,
    SQL_DTYPE = 6,
    SQL_DTYPE_ATTR = 7,
    SQL_UNKNOWN = 8

} sql_entity_type_t;

typedef enum sql_query_type_{

    SQL_SELECT_Q = 9,
    SQL_UPDATE_Q = 10,
    SQL_CREATE_Q = 11,
    SQL_DELETE_Q = 12,
    SQL_UNSUPPORTED_Q = 13

} sql_query_type_t;

typedef enum sql_agg_fn_ {

    SQL_SUM = 14,
    SQL_MIN = 15,
    SQL_MAX = 16,
    SQL_COUNT = 17,
    SQL_AVG = 18,
    SQL_MAX_MAX = 19

} sql_agg_fn_t;

typedef enum sql_keywords_ {

    SQL_FROM = 20,
    SQL_WHERE = 21,
    SQL_GROUP_BY = 22,
    SQL_HAVING = 23,
    SQL_PRIMARY_KEY = 24,
    SQL_NOT_NULL = 25,
    SQL_KEYWORD_MAX = 26,
    
} sql_keywords_t;

typedef enum sql_op_ {

    SQL_LESS_THAN = 27,
    SQL_LESS_THAN_EQ = 28,
    SQL_GREATER_THAN = 29,
    SQL_GREATER_THAN_EQ = 30,
    SQL_EQ = 31,
    SQL_NOT_EQ = 32,
    SQL_AND = 33,
    SQL_OR = 34,
    SQL_IN = 35,
    SQL_BETWEEN = 36, 
    SQL_OP_MAX = 37

} sql_op_t;

typedef struct ident_ {

    unsigned char name [SQL_IDENTIFIER_LEN];
} str_identifier_t;

typedef enum sql_dtype_{

    SQL_STRING  = 38,
    SQL_INT = 39,
    SQL_FLOAT = 40,
    SQL_IPV4_ADDR = 41,
    SQL_DTYPE_MAX = 42

} sql_dype_t;

typedef enum sql_dtype_attr_ {

    SQL_DTYPE_LEN = 43,
    SQL_DTYPE_PRIMARY_KEY = 44,
    SQL_DTYPE_NOT_NULL = 45

} sql_dtype_attr_t;

typedef enum sql_ident_type_ {

    SQL_TABLE_NAME = 46,
    SQL_COLUMN_NAME = 47,
    SQL_INTEGER_VALUE = 48,
    SQL_FLOAT_VALUE = 49,
    SQL_IDNT_TYPE_MAX = 50

} sql_identifier_type_t;

static inline int
sql_valid_dtype (int dtype) {

    switch (dtype) {
        case SQL_STRING:
        case SQL_INT:
        case SQL_FLOAT:
        return 1;
    }
    return 0;
}

static inline int
sql_dtype_size (sql_dype_t dtype) {

    switch (dtype)
    {
    case SQL_STRING:
        return 1;
    case SQL_INT:
        return 4;
    case SQL_FLOAT:
        return sizeof(float);
    }
    return 0;
}

#endif 