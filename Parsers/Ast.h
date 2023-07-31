#include "SQLParserStruct.h"

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
