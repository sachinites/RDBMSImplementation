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
    struct ast_node_ *child_list;
    struct ast_node_ *parent;
    struct ast_node_ *next;
} ast_node_t;

void 
ast_add_child (ast_node_t *parent, ast_node_t *child);

ast_node_t *
ast_find (ast_node_t *root, ast_node_t *tmplate);

void 
ast_destroy_tree (ast_node_t *root);

#define FOR_ALL_AST_CHILD(astnode_ptr, ptr) \
    for (ptr = astnode_ptr->child_list; ptr; ptr = ptr->next)

