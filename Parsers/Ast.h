#ifndef __AST__
#define __AST__

#include "SQLParserStruct.h"

typedef struct ast_node_ {

    sql_entity_type_t entity_type;
    union {
        sql_query_type_t q_type;
        sql_agg_fn_t agg_fn;
        sql_keywords_t kw;
        sql_op_t op;
        sql_dtype_t dtype;
        sql_dtype_attr_t dtype_attr;
        struct {
            sql_identifier_type_t ident_type;
            str_identifier_t identifier;
        } identifier;
    }u;
    void *data;         /* any other data you want to store*/
    struct ast_node_ *child_list;
    struct ast_node_ *parent;
    struct ast_node_ *next;
} ast_node_t;

void 
ast_add_child (ast_node_t *parent, ast_node_t *child);

ast_node_t *
ast_find (ast_node_t *root, ast_node_t *tmplate);

ast_node_t *
ast_find_identifer (ast_node_t *root, ast_node_t *tmplate);

ast_node_t *
ast_find_up (ast_node_t *curr_node, ast_node_t *tmplate);;

void 
ast_destroy_tree_from_root (ast_node_t *root);

void 
ast_destroy_tree (ast_node_t *any_ast_node);

void 
ast_print (ast_node_t *root, int depth) ;

ast_node_t *
ast_node_get_root (ast_node_t *ast_node) ;

#define FOR_ALL_AST_CHILD(astnode_ptr, ptr)     \
    {                                                                                \
    if (astnode_ptr)    {                                                   \
    ast_node_t *_temp;                                                  \
    for (ptr = astnode_ptr->child_list; ptr; ptr = _temp){  \
    _temp = ptr->next;

 #define FOR_ALL_AST_CHILD_END }}}
 
#endif 