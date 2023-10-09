#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <list>
#include <string>
#include "Catalog.h"
#include "sql_utils.h"
#include "SqlMexprIntf.h"
#include "../SqlParser/ParserExport.h"
#include "../../MathExpressionParser/Dtype.h"
#include "qep.h"

/* Imports from ExpressionParser*/
parse_rc_t S (); 
parse_rc_t Q (); 
parse_rc_t E (); 

extern lex_data_t **
mexpr_convert_infix_to_postfix (lex_data_t *infix, int sizein, int *size_out) ;

static MexprTree *
Parser_Mexpr_build_math_expression_tree () {

    int i;
    std::string *string_ptr;
    MexprTree *tree = NULL; 

    int stack_chkp = undo_stack.top + 1;

    parse_rc_t err = PARSER_CALL(E);

    if (err == PARSE_ERR) {
        return NULL;
    }

    int size_out = 0;
    lex_data_t **postfix = mexpr_convert_infix_to_postfix (
                                            &undo_stack.data[stack_chkp], undo_stack.top + 1 - stack_chkp, &size_out);
    
   tree = new MexprTree (postfix, size_out);

    /* Free the post fix array now */
   for (i = 0; i < size_out; i++) {

        if (postfix[i] -> token_code == MATH_CPP_STRING_LST) {

            std::list<std::string *> *str_lst_ptr = 
                reinterpret_cast <std::list<std::string *> *> (postfix[i]->token_val);

            for (std::list<std::string *>::iterator it = str_lst_ptr->begin(); 
                        it != str_lst_ptr->end(); ++it) {
                delete *it;
            }
            delete str_lst_ptr;
        }
        free(postfix[i]);
   }
   free(postfix);

    return tree; 
}


static MexprTree *
Parser_Mexpr_Condition_build_expression_tree () {

    int i;
    MexprTree *tree = NULL; 

    int stack_chkp = undo_stack.top + 1;

    parse_rc_t err = PARSER_CALL(S);

    do {

        if (err == PARSE_SUCCESS) break;
        err = PARSER_CALL(Q);
        if (err == PARSE_ERR) return NULL;

    } while (0);

    int size_out = 0;
    lex_data_t **postfix = mexpr_convert_infix_to_postfix (
                                            &undo_stack.data[stack_chkp], undo_stack.top + 1 - stack_chkp, &size_out);

    tree = new MexprTree(postfix, size_out);

    /* Free the post fix array now */
   for (i = 0; i < size_out; i++) {

        if (postfix[i] -> token_code == MATH_CPP_STRING_LST) {

            std::list<std::string *> *str_lst_ptr = 
                reinterpret_cast <std::list<std::string *> *> (postfix[i]->token_val);

            for (std::list<std::string *>::iterator it = str_lst_ptr->begin(); 
                        it != str_lst_ptr->end(); ++it) {
                delete *it;
            }
            delete str_lst_ptr;
        }
        free(postfix[i]);
   }
   free(postfix);

    return tree;
}


sql_exptree_t *
sql_create_exp_tree_compute ()  {

    sql_exptree_t *sql_exptree = (sql_exptree_t *) calloc (1, sizeof (sql_exptree_t ));
    sql_exptree->tree = Parser_Mexpr_build_math_expression_tree ();

    if (!sql_exptree->tree) {
        printf ("Error : %s(%d) Expression Parsing Failed\n",   __FUNCTION__, __LINE__);
        free (sql_exptree) ;
        return NULL;
    }

    if (!sql_exptree->tree->validate(sql_exptree->tree->root)) {
        printf ("Error : %s(%d) Expression Tree doesnt pass Validation test\n", 
                __FUNCTION__, __LINE__);
        sql_exptree->tree->destroy(sql_exptree->tree->root);
        free (sql_exptree) ;
         return NULL;
    }

    sql_exptree->tree->optimize(sql_exptree->tree->root);
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
    if (!sql_exptree->tree->validate(sql_exptree->tree->root)) {
        printf ("Error : %s(%d) Expression Tree doesnt pass Validation test\n", 
                __FUNCTION__, __LINE__);
        sql_exptree->tree->destroy(sql_exptree->tree->root);
        free (sql_exptree) ;
         return NULL;
    }
    sql_exptree->tree->optimize(sql_exptree->tree->root);
    return sql_exptree;
}


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

static Dtype *
sql_column_value_resolution_fn (void *_data_src) {

    Dtype *dtype = NULL;

    exp_tree_data_src_t *data_src = (exp_tree_data_src_t *)_data_src;

    void *rec = joined_row_search (data_src->table_index, data_src->joined_row);
    if (!rec) return NULL;

    void *val = (void *)((char *)rec + data_src->schema_rec->offset);

    switch (data_src->schema_rec->dtype) {
        
        case  SQL_STRING:
        {
            dtype =  Dtype::factory (MATH_CPP_STRING);
            Dtype_STRING *dtype_str = dynamic_cast <Dtype_STRING *> (dtype);
            dtype_str->dtype.str_val = std::string ((char *)val);
        }
        break;

        case SQL_INT:
        {
            dtype =  Dtype::factory (MATH_CPP_INT);
            Dtype_INT *dtype_int = dynamic_cast <Dtype_INT *> (dtype);
            dtype_int->dtype.int_val = *(int *)val;
        }
        break;

        case SQL_DOUBLE:
        {
            dtype =  Dtype::factory (MATH_CPP_DOUBLE);
            Dtype_DOUBLE *dtype_d = dynamic_cast <Dtype_DOUBLE *> (dtype);
            dtype_d->dtype.d_val = *(double *)val;
        }
        break;        

        case SQL_IPV4_ADDR:
            /* Not Supported by Mexpr Yet*/
            break;
        default:
            assert(0);
    }

    return dtype;
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
    MexprNode *node;
    ctable_val_t *ctable_val;
    schema_rec_t *schema_rec = NULL;
    exp_tree_data_src_t *data_src = NULL;
    Dtype_VARIABLE *opnd_var = NULL;
    char table_name_out [SQL_TABLE_NAME_MAX_SIZE];
    char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE];

   MexprTree_Iterator_Operands_Begin (sql_exptree->tree, node) {

        opnd_var = dynamic_cast <Dtype_VARIABLE *> (node);

        if (opnd_var->is_resolved) continue;;
        
        parser_split_table_column_name (
                (char *)sql_get_opnd_variable_name(node).c_str(), 
                table_name_out, lone_col_name);

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
            printf("Error : %s(%d) : Column %s could not be found in table %s\n", 
                __FUNCTION__, __LINE__,
                sql_get_opnd_variable_name(node).c_str(), ctable_val->table_name);
            return false;
        }
        data_src = (exp_tree_data_src_t *)calloc(1, sizeof(exp_tree_data_src_t));
        data_src->table_index = tindex;
        data_src->schema_rec = schema_rec;
        data_src->joined_row = joined_row;
        opnd_var->InstallOperandProperties (data_src, sql_column_value_resolution_fn);

   } MexprTree_Iterator_Operands_End;

   sql_exptree->tree->optimize (sql_exptree->tree->root);
   return true;
}

bool 
sql_resolve_exptree_against_table (sql_exptree_t *sql_exptree, 
                                                         ctable_val_t * ctable_val, 
                                                         int table_id, joined_row_t *joined_row) {

    BPluskey_t bpkey;
    MexprNode *node;
    schema_rec_t *schema_rec = NULL;
    exp_tree_data_src_t *data_src = NULL;
    Dtype_VARIABLE *opnd_var = NULL;
    char table_name_out [SQL_TABLE_NAME_MAX_SIZE];
    char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE];

   MexprTree_Iterator_Operands_Begin (sql_exptree->tree, node) {

        opnd_var = dynamic_cast <Dtype_VARIABLE *> (node);

         if (opnd_var->is_resolved) continue;;

        parser_split_table_column_name (
                (char *)sql_get_opnd_variable_name(node).c_str(), 
                table_name_out, lone_col_name);

        if (table_name_out[0] == '\0') {
            if (table_id != 0) continue;
            bpkey.key = lone_col_name;
            bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
            schema_rec =  (schema_rec_t *) BPlusTree_Query_Key (ctable_val->schema_table, &bpkey);
        }
        else {
            if (strncmp ( (const char *)table_name_out, 
                                 (const char *)ctable_val->table_name,
                                 SQL_TABLE_NAME_MAX_SIZE) ) {
                continue;
            }
            bpkey.key = lone_col_name;
            bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
            schema_rec = (schema_rec_t *)BPlusTree_Query_Key(ctable_val->schema_table, &bpkey);
        }

        if (!schema_rec) {
                printf("Info (%s) : Operand %s could not be resolved against table %s\n", __FUNCTION__,
                        opnd_var->variable_name.c_str(), ctable_val->table_name);
                continue;
        }
        data_src = (exp_tree_data_src_t *)calloc(1, sizeof(exp_tree_data_src_t));
        data_src->table_index = table_id,
        data_src->schema_rec = schema_rec;
        data_src->joined_row = joined_row;
        opnd_var->InstallOperandProperties (data_src, sql_column_value_resolution_fn);

   } MexprTree_Iterator_Operands_End;

   sql_exptree->tree->RemoveUnresolveOperands();
   sql_exptree->tree->optimize (sql_exptree->tree->root);
   return true;
}

bool 
sql_evaluate_conditional_exp_tree (sql_exptree_t *sql_exptree) {

    bool rc;
    Dtype *res;
    Dtype_BOOL *d_bool;

    if (!sql_exptree) return true;
    res = sql_exptree->tree->evaluate (sql_exptree->tree->root);
    d_bool = dynamic_cast <Dtype_BOOL *> (res);
    rc = d_bool->dtype.b_val;
    delete d_bool;
    return rc;
}

Dtype *
sql_evaluate_exp_tree (sql_exptree_t *sql_exptree) {

    return sql_exptree->tree->evaluate (sql_exptree->tree->root);
}

sql_exptree_t *
sql_create_exp_tree_for_one_operand (char *opnd_name) {

    Dtype_VARIABLE *dtype_var = new Dtype_VARIABLE(std::string (opnd_name));
    dtype_var->is_resolved = false;
    sql_exptree_t *sql_exptree = (sql_exptree_t *)calloc(1, sizeof(sql_exptree_t));
    sql_exptree->tree = new MexprTree();
    sql_exptree->tree->root = dtype_var;
    sql_exptree->tree->lst_head = sql_exptree->tree->root;
    return sql_exptree;
}

bool 
sql_opnd_node_is_resolved (MexprNode *opnd_node) {

    Dtype *dtype = dynamic_cast <Dtype *> (opnd_node);
    return dtype->is_resolved;
}


std::string 
sql_get_opnd_variable_name (MexprNode *opnd_node) {

    Dtype_VARIABLE *dtype = dynamic_cast <Dtype_VARIABLE *> (opnd_node);
    return dtype->variable_name;
}

sql_exptree_t *
sql_clone_expression_tree (sql_exptree_t *src_tree) {

    if (!src_tree) return NULL;
    sql_exptree_t * sql_exptree = (sql_exptree_t *) calloc (1, sizeof (sql_exptree_t));
    if (src_tree->tree == NULL) return sql_exptree;
    sql_exptree->tree = src_tree->tree->clone(src_tree->tree->root);
    return sql_exptree;
}

bool 
sql_concatenate_expr_trees (sql_exptree_t *parent_tree, 
                                                MexprNode *opnd_node,
                                                sql_exptree_t *child_tree) {

    bool rc = parent_tree->tree->concatenate (opnd_node, child_tree->tree);
    free (child_tree);
    return rc;
}

void 
sql_destroy_exp_tree (sql_exptree_t *tree) {

    tree->tree->destroy(tree->tree->root);
    free(tree);
}

MexprNode *
sql_tree_get_first_operand (sql_exptree_t *tree) {

    if (!tree->tree) return NULL;
    return tree->tree->lst_head;
}

MexprNode *
sql_tree_get_next_operand (MexprNode *node) {

    if (!node) return node;
    assert (dynamic_cast <Dtype_VARIABLE *> (node) ); 
    return node->lst_right;
}

bool 
sql_is_expression_tree_only_operand (sql_exptree_t *sql_exptree) {

    return sql_exptree->tree->IsLoneVariableOperandNode();
}

MexprNode *
sql_tree_get_root (sql_exptree_t *tree) {
    if (!tree->tree || !tree->tree->root) return NULL;
    return tree->tree->root;
}

void 
InstallDtypeOperandProperties (MexprNode *node, void *data_src, Dtype *(*compute_fn_ptr)(void *)) {

    Dtype_VARIABLE *dtype_node = dynamic_cast <Dtype_VARIABLE *> (node);
    dtype_node->InstallOperandProperties (data_src, compute_fn_ptr);
}

uint8_t  
sql_tree_remove_unresolve_operands(sql_exptree_t *sql_exptree) {

    if (!sql_exptree || sql_exptree->tree) return 0;
    return sql_exptree->tree->RemoveUnresolveOperands();
}

void 
sql_destroy_Dtype_value_holder (Dtype *dtype) {

    delete dtype;
}