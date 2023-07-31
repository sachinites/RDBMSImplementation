#ifndef __SQLPARSER_STRUCT__
#define __SQLPARSER_STRUCT__

#define SQL_IDENTIFIER_LEN  64

typedef enum sql_entity_ {

    SQL_QUERY_TYPE = 1,
    SQL_AGG_FN = 2,
    SQL_KEYWORD = 3,
    SQL_OPERATOR = 4,
    SQL_IDENTIFIER = 5,
    SQL_UNKNOWN = 6
} sql_entity_type_t;

typedef enum sql_query_type_{

    SQL_SELECT_Q = 7,
    SQL_UPDATE_Q = 8,
    SQL_CREATE_Q = 9,
    SQL_DELETE_Q = 10,
    SQL_UNSUPPORTED_Q = 11
} sql_query_type_t;

typedef enum sql_agg_fn_ {

    SQL_SUM = 12,
    SQL_MIN = 13,
    SQL_MAX = 14,
    SQL_COUNT = 15,
    SQL_AVG = 16,
    SQL_MAX_MAX = 17
} sql_agg_fn_t;

typedef enum sql_keywords_ {

    SQL_FROM = 18,
    SQL_WHERE = 19,
    SQL_GROUP_BY = 20,
    SQL_HAVING = 21,
    SQL_COLUMNS_DUMMY = 22,
    SQL_PRIMARY_KEY = 23,
    SQL_NOT_NULL = 24,
    SQL_KEYWORD_MAX = 25,
    
} sql_keywords_t;

typedef enum sql_op_ {

    SQL_LESS_THAN = 26,
    SQL_LESS_THAN_EQ = 27,
    SQL_GREATER_THAN = 28,
    SQL_GREATER_THAN_EQ = 29,
    SQL_EQ = 30,
    SQL_NOT_EQ = 31,
    SQL_AND = 32,
    SQL_OR = 33,
    SQL_IN = 34,
    SQL_BETWEEN = 35, 
    SQL_OP_MAX = 36
} sql_op_t;

typedef struct ident_ {

    unsigned char name [SQL_IDENTIFIER_LEN];
} str_identifier_t;

typedef enum sql_dtype_{

    SQL_STRING  = 37,
    SQL_INT = 38,
    SQL_FLOAT = 39,
    SQL_DTYPE_MAX = 40
} sql_dype_t;

typedef enum sql_ident_type_ {

    SQL_TABLE_NAME = 41,
    SQL_COLUMN_NAME = 42,
    SQL_CONDITION_VALUE = 43,
    SQL_IDNT_TYPE_MAX = 44
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

#endif 