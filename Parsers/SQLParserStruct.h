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
    SQL_KEYWORD_MAX = 23
} sql_keywords_t;

typedef enum sql_op_ {

    SQL_LESS_THAN = 24,
    SQL_LESS_THAN_EQ = 25,
    SQL_GREATER_THAN = 26,
    SQL_GREATER_THAN_EQ = 27,
    SQL_EQ = 28,
    SQL_NOT_EQ = 29,
    SQL_AND = 30,
    SQL_OR = 31,
    SQL_IN = 32,
    SQL_BETWEEN = 33, 
    SQL_OP_MAX = 34
} sql_op_t;

typedef struct ident_ {

    unsigned char name [SQL_IDENTIFIER_LEN];
} str_identifier_t;

typedef enum sql_dtype_{

    SQL_STRING  = 35,
    SQL_INT = 36,
    SQL_FLOAT = 37,
    SQL_DTYPE_MAX = 38
} sql_dype_t;

typedef enum sql_ident_type_ {

    SQL_TABLE_NAME = 39,
    SQL_COLUMN_NAME = 40,
    SQL_CONDITION_VALUE = 41,
    SQL_IDNT_TYPE_MAX = 42
} sql_identifier_type_t;

typedef struct ast_node_ {

    sql_entity_type_t entity_type;
    union {
        sql_query_type_t q_type;
        sql_agg_fn_t agg_fn;
        sql_keywords_t kw;
        sql_op_t op;
        struct {
            sql_identifier_type_t ident_type;
            str_identifier_t identifer;
        } identifier;
    }u;
    struct ast_node_ *left;
    struct ast_node_ *right;
    struct ast_node_ *parent;
} ast_node_t;

#endif 