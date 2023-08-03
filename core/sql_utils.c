
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include "sql_utils.h"
#include "sql_const.h"
#include "../Parsers/Ast.h"

int 
rdbms_key_comp_fn (BPluskey_t *key_1, BPluskey_t *key_2, key_mdata_t *key_mdata, int size) {

    int i , rc;
    int dsize;
    int offset = 0;

    sql_dype_t dtype;

    if (!key_1 || !key_1->key || !key_1->key_size) return 1;
    if (!key_2 || !key_2->key || !key_2->key_size) return -1;

    char *key1 = (char *)key_1->key;
    char *key2 = (char *)key_2->key;

    if (!key_mdata || !size) {
        /* Implement a general memcmp function */
        assert (key_1->key_size == key_2->key_size);
        rc = memcmp (key1, key2, key_1->key_size);
        if (rc > 0) return -1;
        if (rc < 0) return 1;
        return 0; 
    }

    for (i = 0; i < size ; i++) {

        dtype = key_mdata->dtype;
        dsize = key_mdata->size;

        switch (dtype) {

            case SQL_STRING:
                rc = strncmp (key1 + offset, key2 + offset, dsize);
                if (rc < 0) return 1;
                if (rc > 0) return -1;
                offset += dsize;
                break;
            case  SQL_INT:
                {
                    int *n1 = (int *)(key1 + offset);
                    int *n2 = (int *)(key2 + offset);
                    if (*n1 < *n2) return 1;
                    if (*n1 > *n2) return -1;
                    offset += dsize;
                }
                break;
            case SQL_IPV4_ADDR:
                {
                    uint32_t *n1 = (uint32_t *)(key1 + offset);
                    uint32_t *n2 = (uint32_t *)(key2 + offset);
                    if (*n1 < *n2) return 1;
                    if (*n1 > *n2) return -1;
                    offset += dsize;
                }
                break;
            case SQL_FLOAT:
            break;
        }
    }
    return 0;
}

int
BPlusTree_key_format_fn_default (BPluskey_t *key, unsigned char *obuff, int buff_size) {

	assert (key->key_size <= buff_size);
	memset (obuff, 0, buff_size);
	memcpy (obuff, key->key, key->key_size);
	return  key->key_size;
}

int
BPlusTree_value_format_fn_default (void *value, unsigned char *obuff, int buff_size) {

	memset (obuff, 0, buff_size);
	strncpy ( (char *)obuff, (char *)value, buff_size);
	return 0;
}

key_mdata_t *
sql_construct_table_key_mdata (ast_node_t *root, int *key_mdata_size) {

    int i;
    int attr_len;
    bool is_key;
    bool is_non_null;
    ast_node_t *ast_node;
    ast_node_t *attr_node;
    ast_node_t *col_node;
    ast_node_t astnode_tmplate;
    ast_node_t *col_dtype_node;
    ast_node_t *attr_node_len;
    ast_node_t *identifier_node;
    ast_node_t *attr_node_pr_key;
    ast_node_t *attr_node_non_null;

    key_mdata_t *key_mdata = (key_mdata_t *)calloc
         (SQL_MAX_PRIMARY_KEYS_SUPPORTED, sizeof (key_mdata_t));
        
    switch (root->entity_type) {

        case SQL_QUERY_TYPE:

            switch (root->u.q_type) {

                case SQL_CREATE_Q:
                    memset (&astnode_tmplate, 0, sizeof (ast_node_t ));
                    astnode_tmplate.entity_type = SQL_IDENTIFIER;
                    astnode_tmplate.u.identifier.ident_type = SQL_TABLE_NAME;
                    ast_node = ast_find (root, &astnode_tmplate);
                    assert (ast_node);
                    i = 0;

                    FOR_ALL_AST_CHILD(ast_node, col_node) {
                        col_dtype_node = col_node->child_list;
                        assert (col_dtype_node->entity_type == SQL_DTYPE);

                        is_key = false;
                        is_non_null = false;
                        attr_len = 1;
                         FOR_ALL_AST_CHILD(col_dtype_node, attr_node) {
                            
                            assert (attr_node->entity_type == SQL_DTYPE_ATTR);
                            switch (attr_node->u.dtype_attr) {
                                case SQL_DTYPE_LEN:
                                    identifier_node = attr_node->child_list;
                                    assert (identifier_node->entity_type == SQL_IDENTIFIER);
                                    assert (identifier_node->u.identifier.ident_type ==  SQL_INTEGER_VALUE);
                                    memcpy (&attr_len, identifier_node->u.identifier.identifier.name, sizeof (int));
                                    break;
                                case SQL_DTYPE_PRIMARY_KEY :
                                    is_key = true;
                                    break;
                                case SQL_DTYPE_NOT_NULL:
                                    is_non_null = true;
                                    break;
                            }
                         }
                         if (is_key) {
                            key_mdata[i].dtype = col_dtype_node->u.dtype;
                            key_mdata[i].size = attr_len;
                            i++;
                         }
                    }
                break;
                default:
                    assert(0);
            }
        break;
        default:
            assert(0);
    }

    if (!i) {
        free (key_mdata);
        key_mdata = NULL;
    }

    *key_mdata_size = i;
    return key_mdata;
}