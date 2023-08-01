#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include "Ast.h"

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
    }

    cur->next = child;
    child->parent = parent;
}

static bool 
ast_match (ast_node_t *root, ast_node_t *tmplate) {

    if (root->entity_type != tmplate->entity_type) return false;
    if (memcmp (&root->u, &tmplate->u, sizeof (root->u)) == 0) return true;
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
    }

    return NULL;
}

void 
ast_destroy_tree (ast_node_t *root) {

     ast_node_t *cur;
    if (!root) return;

    FOR_ALL_AST_CHILD(root, cur) {

        ast_destroy_tree(cur);
    }

    free(root);
}
