#ifndef __SQLPARSER_STRUCT__
#define __SQLPARSER_STRUCT__

#define SQL_IDENTIFIER_LEN  64

typedef enum sql_entity_ {

    SQL_QUERY_TYPE = 1,
    SQL_AGG_FN,
    SQL_KEYWORD ,
    SQL_OPERATOR,
    SQL_IDENTIFIER,
    SQL_DTYPE,
    SQL_DTYPE_ATTR,
    SQL_UNKNOWN

} sql_entity_type_t;

typedef enum sql_query_type_{

    SQL_SELECT_Q = 11,
    SQL_UPDATE_Q ,
    SQL_CREATE_Q,
    SQL_DELETE_Q,
    SQL_INSERT_Q ,
    SQL_UNSUPPORTED_Q

} sql_query_type_t;

typedef enum sql_agg_fn_ {

    SQL_SUM = 21,
    SQL_MIN,
    SQL_MAX,
    SQL_COUNT,
    SQL_AVG,
    SQL_MAX_MAX

} sql_agg_fn_t;

typedef enum sql_keywords_ {

    SQL_FROM = 31,
    SQL_WHERE,
    SQL_GROUP_BY,
    SQL_HAVING,
    SQL_PRIMARY_KEY,
    SQL_NOT_NULL,
    SQL_KEYWORD_MAX,
    
} sql_keywords_t;

typedef enum sql_op_ {

    SQL_LESS_THAN = 41,
    SQL_LESS_THAN_EQ ,
    SQL_GREATER_THAN ,
    SQL_GREATER_THAN_EQ,
    SQL_EQ ,
    SQL_NOT_EQ,
    SQL_AND ,
    SQL_OR,
    SQL_IN,
    SQL_BETWEEN, 
    SQL_OP_MAX =  51

} sql_op_t;

typedef struct ident_ {

    unsigned char name [SQL_IDENTIFIER_LEN];
} str_identifier_t;

typedef enum sql_dtype_{

    SQL_STRING  = 61,
    SQL_INT ,
    SQL_FLOAT,
    SQL_IPV4_ADDR,
    SQL_DTYPE_MAX

} sql_dype_t;

typedef enum sql_dtype_attr_ {

    SQL_DTYPE_LEN = 71,
    SQL_DTYPE_PRIMARY_KEY,
    SQL_DTYPE_NOT_NULL

} sql_dtype_attr_t;

typedef enum sql_ident_type_ {

    SQL_TABLE_NAME = 81,
    SQL_COLUMN_NAME ,
    SQL_INTEGER_VALUE,
    SQL_STRING_VALUE ,
    SQL_FLOAT_VALUE,
    SQL_IDNT_TYPE_MAX

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