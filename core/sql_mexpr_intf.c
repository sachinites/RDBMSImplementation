
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sql_mexpr_intf.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "Catalog.h"
#include "../../MathExpressionParser/MexprEnums.h"
#include "../SqlParser/SqlParserStruct.h"
#include "../../MathExpressionParser/ParserMexpr.h"
#include "sql_utils.h"
#include "qep.h"

typedef struct exp_tree_data_src_ {

    int table_index;
    schema_rec_t *schema_rec;
    joined_row_t *joined_row;

} exp_tree_data_src_t;

static void *
joined_row_search ( int table_id, joined_row_t *joined_row) {

    if (joined_row->size == 1) {

        if (joined_row->table_id_array[0] == table_id) {
            return  joined_row->rec_array[0];
        }

        return NULL;
    }

   return joined_row->rec_array[table_id];
}

static mexpr_var_t
sql_column_value_resolution_fn (void *_data_src) {

    mexpr_var_t res;
    res.dtype = MEXPR_DTYPE_INVALID;

    exp_tree_data_src_t *data_src = (exp_tree_data_src_t *)_data_src;

    void *rec = joined_row_search (data_src->table_index, data_src->joined_row);
    if (!rec) return res;

    void *val = (void *)((char *)rec + data_src->schema_rec->offset);

    switch (data_src->schema_rec->dtype) {
        
        case  SQL_STRING:
            res.dtype = MEXPR_DTYPE_STRING;
            res.u.str_val = (unsigned char *)val;
            break;
        case SQL_INT:
            res.dtype = MEXPR_DTYPE_INT;
            res.u.int_val = *(int *)val;
            break;
        case SQL_DOUBLE:
            res.dtype = MEXPR_DTYPE_DOUBLE;
            res.u.d_val = *(double *)val;
            break;        
        case SQL_IPV4_ADDR:
            /* Not Supported by Mexpr Yet*/
            break;
        default:
            assert(0);
    }

    return res;
}

/* Return true if all operands are resolved successfully,
    return false if atleast one operand is unresolved*/
bool 
sql_resolve_exptree (BPlusTree_t *tcatalog,
                                  sql_exptree_t *sql_exptree,
                                  qep_struct_t *qep,
                                  joined_row_t *joined_row) {
    /* Algorithm : 
    1. Iterate over the operands opnd of the exptree
    2.  If opnd name is one word 'col_name', then resolve it against table_arr[0] only. Index = 0.
    3. if opnd_name is table_name.col_name then split 'table_name' and 'col_name'. 
            3.1 lookup table_name in tcatalog and get ctable_val_t *
            3.2 Now resolve 'col_name' against this found  'ctable_val_t *' only. Index = matching index in ctable_val_t ** array.
    4. lookup the schema_rec_t in  ctable_val_t -> schema_table using col_name as a key
    5. Prepare exp_tree_data_src_t object
            exp_tree_data_src_t *data_src = malloc () ..  
            data_src->table_index = Index;
            data_src->schema_rec = schema_rec; 
            data_src->joined_row = joined_row

    6. mexpt_tree_install_operand_properties (opnd_tree_node, <to be removed>, joined_row_t *, sql_column_value_resolution_fn);
    7. Any Column which could not be resolved should be removed from Exp Tree using API 
    mexpt_remove_unresolved_operands ( ) followed by mexpt_optimize( )
    */
   
    int tindex = 0, i;
    BPluskey_t bpkey;
    mexpt_node_t *node;
    ctable_val_t *ctable_val;
    schema_rec_t *schema_rec = NULL;
    exp_tree_data_src_t *data_src = NULL;
    unsigned char table_name_out [SQL_TABLE_NAME_MAX_SIZE];
    unsigned char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE];

   mexpt_iterate_operands_begin (sql_exptree->tree, node) {

        if (node->u.opd_node.is_resolved) continue;
        parser_split_table_column_name (
                node->u.opd_node.opd_value.variable_name, table_name_out, lone_col_name);
        if (table_name_out[0] == '\0') {
            tindex = 0;
            ctable_val = qep->join.tables[0].ctable_val;
            bpkey.key = lone_col_name;
            bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
            schema_rec =  (schema_rec_t *) BPlusTree_Query_Key (ctable_val->schema_table, &bpkey);
        }
        else {
            ctable_val =  sql_catalog_table_lookup_by_table_name (tcatalog, table_name_out);
            if (!ctable_val) {
                printf ("Error : %s(%d) Table %s does not exit\n", __FUNCTION__, __LINE__, table_name_out);
                return false;
            }
            for (i = 0; i < qep->join.table_cnt; i++) {
                if (ctable_val != qep->join.tables[i].ctable_val) continue;
                tindex = i;
                bpkey.key = lone_col_name;
                bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
                schema_rec =  (schema_rec_t *) BPlusTree_Query_Key (ctable_val->schema_table, &bpkey);
                break;
            }
        }
        if (!schema_rec) {
            printf("Error : %s(%d) : Column %s could not be found in table %s\n", __FUNCTION__, __LINE__,
                       node->u.opd_node.opd_value.variable_name, ctable_val->table_name);
            return false;
        }
        data_src = (exp_tree_data_src_t *)calloc(1, sizeof(exp_tree_data_src_t));
        data_src->table_index = tindex;
        data_src->schema_rec = schema_rec;
        data_src->joined_row = joined_row;
        mexpt_tree_install_operand_properties(node, data_src, sql_column_value_resolution_fn);
   } mexpt_iterate_operands_end(sql_exptree->tree, node);

   mexpt_optimize (sql_exptree->tree->root);
   return true;
}

bool 
sql_resolve_exptree_against_table (sql_exptree_t *sql_exptree, 
                                                         ctable_val_t * ctable_val, 
                                                         int table_id, joined_row_t *joined_row) {

    BPluskey_t bpkey;
    mexpt_node_t *node;
    schema_rec_t *schema_rec = NULL;
    exp_tree_data_src_t *data_src = NULL;
    unsigned char table_name_out [SQL_TABLE_NAME_MAX_SIZE];
    unsigned char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE];

   mexpt_iterate_operands_begin (sql_exptree->tree, node) {

        if (node->u.opd_node.is_resolved) continue;
        parser_split_table_column_name (
                node->u.opd_node.opd_value.variable_name, table_name_out, lone_col_name);

        if (table_name_out[0] == '\0') {
            if (table_id != 0) continue;
            bpkey.key = lone_col_name;
            bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
            schema_rec =  (schema_rec_t *) BPlusTree_Query_Key (ctable_val->schema_table, &bpkey);
        }
        else {
            if (strncmp (table_name_out, ctable_val->table_name, SQL_TABLE_NAME_MAX_SIZE) ) {
                continue;
            }
            bpkey.key = lone_col_name;
            bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
            schema_rec = (schema_rec_t *)BPlusTree_Query_Key(ctable_val->schema_table, &bpkey);
        }

        if (!schema_rec) {
                printf("Info (%s) : Operand %s could not be resolved against table %s\n", __FUNCTION__,
                       node->u.opd_node.opd_value.variable_name, ctable_val->table_name);
                continue;
        }
        data_src = (exp_tree_data_src_t *)calloc(1, sizeof(exp_tree_data_src_t));
        data_src->table_index = table_id,
        data_src->schema_rec = schema_rec;
        data_src->joined_row = joined_row;
        mexpt_tree_install_operand_properties(node, data_src, sql_column_value_resolution_fn);

   } mexpt_iterate_operands_end (sql_exptree->tree, node);

   mexpt_remove_unresolved_operands (sql_exptree->tree, true);
   mexpt_optimize (sql_exptree->tree->root);
   return true;
}

sql_exptree_t *
sql_create_exp_tree_compute ()  {

    sql_exptree_t *sql_exptree = (sql_exptree_t *) calloc (1, sizeof (sql_exptree_t ));
    sql_exptree->tree = Parser_Mexpr_build_math_expression_tree ();
    if (!sql_exptree->tree) {
        free (sql_exptree) ;
        return NULL;
    }
    if (!mexpr_validate_expression_tree (sql_exptree->tree)) {
        mexpt_destroy (sql_exptree->tree->root, false);
        free(sql_exptree->tree);
        free (sql_exptree) ;
         return NULL;
    }
    mexpt_optimize (sql_exptree->tree->root);
    return sql_exptree;
}

sql_exptree_t *
sql_create_exp_tree_for_one_operand (unsigned char *opnd_name) {

    sql_exptree_t *sql_exptree = (sql_exptree_t *) calloc (1, sizeof (sql_exptree_t ));
    sql_exptree->tree = (mexpt_tree_t *) calloc (1, sizeof (mexpt_tree_t));
    sql_exptree->tree->root = mexpr_create_mexpt_node (
            SQL_IDENTIFIER_IDENTIFIER,
            SQL_COMPOSITE_COLUMN_NAME_SIZE,
            opnd_name);
    sql_exptree->tree->opd_list_head.lst_right = sql_exptree->tree->root;
    sql_exptree->tree->root->lst_left = &sql_exptree->tree->opd_list_head;
    return sql_exptree;
}

sql_exptree_t *
sql_create_exp_tree_conditional () {

    sql_exptree_t *sql_exptree = (sql_exptree_t *) calloc (1, sizeof (sql_exptree_t ));
    sql_exptree->tree = Parser_Mexpr_Condition_build_expression_tree ();
    if (!sql_exptree->tree) {
        free (sql_exptree) ;
        return NULL;
    }
    if (!mexpr_validate_expression_tree (sql_exptree->tree)) {
        mexpt_destroy (sql_exptree->tree->root, false);
        free(sql_exptree->tree);
        free (sql_exptree) ;
        return NULL;
    }
    mexpt_optimize (sql_exptree->tree->root);
    return sql_exptree;
}

bool 
sql_evaluate_conditional_exp_tree (sql_exptree_t *sql_exptree) {

    mexpr_var_t res;
    if (!sql_exptree) return true;
    res = mexpt_evaluate (sql_exptree->tree->root);
    assert (res.dtype == MEXPR_DTYPE_BOOL);
    return res.u.b_val;
}

mexpr_var_t 
sql_evaluate_exp_tree (sql_exptree_t *sql_exptree) {

    mexpr_var_t res;
    res = mexpt_evaluate (sql_exptree->tree->root);
    return res;
}

mexpt_node_t *
sql_create_operand_node (unsigned char *opnd_name) {

    mexpt_node_t *node = mexpr_create_mexpt_node (
                    SQL_IDENTIFIER_IDENTIFIER,
                    MEXPR_TREE_OPERAND_LEN_MAX,
                    opnd_name);
    return node;
}

bool 
sql_is_expression_tree_only_operand (sql_exptree_t *sql_exptree) {

    mexpt_node_t *node = sql_exptree->tree->root;

    bool rc =  (node->token_code == SQL_IDENTIFIER ||
                      node->token_code == SQL_IDENTIFIER_IDENTIFIER);

    if (rc) {
        assert (node->left == NULL && node->right == NULL);
    }
    return rc;
}

