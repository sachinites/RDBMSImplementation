#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <assert.h>
#include "Ast.h"

typedef struct where_literal_ where_literal_t;
extern void sql_where_literals_array_free (where_literal_t *where_literal_arr) ;

void 
ast_add_child (ast_node_t *parent, ast_node_t *child) {

    ast_node_t *cur;

    if (!parent->child_list) {
        parent->child_list = child;
        child->parent = parent;
        return;
    }

    FOR_ALL_AST_CHILD(parent, cur) {

        if (cur->next) continue;
        break;

    } FOR_ALL_AST_CHILD_END;

    cur->next = child;
    child->parent = parent;
}

static bool 
ast_match (ast_node_t *root, ast_node_t *tmplate) {

    if (root->entity_type != tmplate->entity_type) return false;
    if (memcmp (&root->u, &tmplate->u, sizeof(int)) == 0) return true;
    return false;
}

ast_node_t *
ast_find (ast_node_t *root, ast_node_t *tmplate) {

    ast_node_t *cur;
    ast_node_t *res;

    if (!root) return NULL;

    if (ast_match (root, tmplate)) return root;

    FOR_ALL_AST_CHILD(root, cur) {

       res =  ast_find (cur, tmplate);
       if (res) return res;
    }  FOR_ALL_AST_CHILD_END;

    return NULL;
}

ast_node_t *
ast_find_identifer (ast_node_t *root, ast_node_t *tmplate) {

    ast_node_t *cur;
    ast_node_t *res;
    
    assert (tmplate->entity_type == SQL_IDENTIFIER);

    if (!root) return NULL;
    
    if (ast_match (root, tmplate) && 
            strncmp (root->u.identifier.identifier.name, 
                           tmplate->u.identifier.identifier.name, 
                           sizeof (tmplate->u.identifier.identifier.name)) == 0) return root;

    FOR_ALL_AST_CHILD(root, cur) {

       res =  ast_find_identifer (cur, tmplate);
       if (res) return res;

    }  FOR_ALL_AST_CHILD_END;

    return NULL;
}

ast_node_t *
ast_find_up (ast_node_t *curr_node, ast_node_t *tmplate) {

    if (ast_match (curr_node, tmplate)) return curr_node;

    while (curr_node = curr_node->parent) {
        if (ast_match (curr_node, tmplate)) return curr_node;
    }
    return NULL;
}

static void 
ast_node_free (ast_node_t *node) {

    if (node->entity_type == SQL_IDENTIFIER &&
            node->u.identifier.ident_type == SQL_PTR) {

        void *array_ptr = NULL;
        memcpy (&array_ptr, (void *)node->u.identifier.identifier.name, sizeof (void *));
        sql_where_literals_array_free (array_ptr);
    }
    if (node->data) free(node->data);
    free (node);
}

void 
ast_destroy_tree_from_root (ast_node_t *root) {

     ast_node_t *cur;
    if (!root) return;

    FOR_ALL_AST_CHILD(root, cur) {

        ast_destroy_tree_from_root(cur);

    } FOR_ALL_AST_CHILD_END;

    ast_node_free (root);
}

void 
ast_destroy_tree (ast_node_t *any_ast_node) {

    while ((any_ast_node->parent &&
                    (any_ast_node =  any_ast_node->parent)));

    ast_destroy_tree_from_root  (any_ast_node);
}

static void 
ast_node_print (ast_node_t *root, int depth) {

    printf ("%p depth:%d sql_entity_type = %d\n", 
        root, depth,
        root->entity_type);

    switch (root->entity_type) {

        case SQL_QUERY_TYPE:
            printf ("Query type : %d\n", root->u.q_type);
            break;
        case SQL_AGG_FN:
            switch (root->u.agg_fn) {
                case SQL_SUM:
                    printf ("Agg fn : sum\n");
                    break;
                case SQL_MIN:
                    printf ("Agg fn : min\n");
                    break;                
                case SQL_MAX:
                    printf ("Agg fn : max\n");
                    break;                
                case SQL_COUNT:
                    printf ("Agg fn : count\n");
                    break;                
                case SQL_AVG:
                    printf ("Agg fn : avg\n");
                    break;                
                case SQL_AGG_FN_NONE:
                    printf ("Agg fn : None\n");
                    break;
                break;
                default: ;
            }
            break;
        case SQL_KEYWORD:
            printf ("Key word : %d\n", root->u.kw);
            break;
        case SQL_OPERATOR:
            printf ("Operator = %d\n", root->u.op);
            break;
        case SQL_IDENTIFIER:
            switch (root->u.identifier.ident_type) {
                case SQL_TABLE_NAME:
                case SQL_TABLE_COLMN_NAME:
                    printf ("Table name = %s\n", root->u.identifier.identifier.name);
                    break;
                case SQL_COLUMN_NAME:
                    printf ("Colmn name = %s\n",  root->u.identifier.identifier.name);
                    break;
                case SQL_INTEGER_VALUE:
                    printf ("Integer Value = %d\n", *(int *)(root->u.identifier.identifier.name));
                    break;
                case SQL_FLOAT_VALUE:
                    printf ("Float Value =  ?\n");
                    break;
                default: ;
            }
        break;
        case SQL_DTYPE:
            switch (root->u.dtype) {
                case SQL_STRING:
                    printf ("Dtype : String\n");
                    break;
                case SQL_INT:
                    printf ("Dtype : int\n");
                    break;
                case SQL_FLOAT:
                printf ("Dtype : float\n");
                break;
                default: ;
            }
        break;
        case SQL_DTYPE_ATTR:
            switch (root->u.dtype_attr) {
                case SQL_DTYPE_LEN:
                    printf ("dtype attr : dtype len\n");
                    break;
                case SQL_DTYPE_PRIMARY_KEY:
                    printf ("dtype attr : primary key\n");
                    break;
                case SQL_DTYPE_NOT_NULL:
                    printf ("dtype attr : not null\n");
                    break;
            }
        break;
    }
}


void 
ast_print (ast_node_t *root, int depth) {
     
    ast_node_t *cur;
    if (!root) return;

    FOR_ALL_AST_CHILD(root, cur) {

        ast_print(cur, depth + 1);
        
    } FOR_ALL_AST_CHILD_END;

    ast_node_print (root, depth);
}

ast_node_t *
ast_node_get_root (ast_node_t *ast_node)  {

    while (ast_node->parent) {
        ast_node = ast_node->parent;
    }

    return ast_node;
}