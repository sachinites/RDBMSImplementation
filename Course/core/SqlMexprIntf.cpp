#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "sql_const.h"
#include "SqlMexprIntf.h"
#include "rdbms_struct.h"
#include "Catalog.h"
#include "sql_utils.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "../../SqlParser/ParserExport.h"
#include "../../../MathExpressionParser/Course/MexprTree.h"
#include "../../../MathExpressionParser/Course/Dtype.h"
#include "qep.h"
#include "sql_name.h"

parse_rc_t E (); 
extern lex_data_t **
mexpr_convert_infix_to_postfix (lex_data_t *infix, int sizein, int *size_out) ;

static void
postfix_array_free(lex_data_t **lex_data_array, int size) {
   
    for (int i = 0; i < size; i++) {

        if (lex_data_array[i])
        {
            if (lex_data_array[i]->token_val) free(lex_data_array[i]->token_val);
            free(lex_data_array[i]);
        }
    }

    free(lex_data_array);
}


// " <token> a + b"
static MexprTree *
Parser_Mexpr_build_math_expression_tree () {

    int i;
    MexprTree *tree = NULL;

    int stack_chkp = undo_stack.top + 1;
    parse_rc_t err = E();

    if (err == PARSE_ERR) {
        return NULL;
    }

    int size_out = 0;
    lex_data_t **postfix = mexpr_convert_infix_to_postfix (
                                            &undo_stack.data[stack_chkp],  undo_stack.top + 1 - stack_chkp, &size_out);

    tree = new MexprTree (postfix, size_out);
    postfix_array_free (postfix, size_out);
    return tree;
}


sql_exptree_t *
sql_create_exp_tree_compute () {

    sql_exptree_t *sql_exptree = ( sql_exptree_t *) calloc (1, sizeof (sql_exptree_t));
    sql_exptree->tree = Parser_Mexpr_build_math_expression_tree ();
    if (!sql_exptree->tree) {
        free(sql_exptree);
        return NULL;
    }
    
    if (!sql_exptree->tree->validate (sql_exptree->tree->root )) {

        printf ("Error : %s(%d) Expression Tree doesnt pass Validation test\n", 
                __FUNCTION__, __LINE__);
        sql_exptree->tree->destroy (sql_exptree->tree->root);
        free(sql_exptree);
        return NULL;
    }

    return sql_exptree;
}

void 
sql_destroy_exp_tree (sql_exptree_t *tree) {

    tree->tree->destroy(tree->tree->root);
    free(tree);
}

Dtype *
sql_evaluate_exp_tree (sql_exptree_t *sql_exptree) {

    return sql_exptree->tree->evaluate (sql_exptree->tree->root);
}


void 
sql_destroy_Dtype_value_holder (Dtype *dtype) {

    delete dtype;
}

static Dtype *
sql_column_value_resolution_fn (
                char *sql_column_name, 
                void *_data_src) {

    Dtype *dtype = NULL;

    exp_tree_data_src_t *data_src = (exp_tree_data_src_t *)_data_src;

    void *rec = (*data_src->joined_row)->rec_array[data_src->table_index];

    if (!rec) return NULL;

    void *val = (void *)((char *)rec + data_src->schema_rec->offset);
    
    dtype =  Dtype::factory (
                    sql_to_mexpr_dtype_converter(data_src->schema_rec->dtype));
    if (!dtype) return NULL;

    switch (data_src->schema_rec->dtype) {
        
        case SQL_INT:
        {
            Dtype_INT *dtype_int = dynamic_cast <Dtype_INT *> (dtype);
            dtype_int->dtype.int_val = *(int *)val;
            dtype->is_resolved = true;
        }
        break;
        case SQL_DOUBLE:
        {
            Dtype_DOUBLE *dtype_d = dynamic_cast <Dtype_DOUBLE *> (dtype);
            dtype_d->dtype.d_val = *(double *)val;
            dtype->is_resolved = true;
        }
        break;                
        case SQL_STRING:
        {
            Dtype_STRING *dtype_str = dynamic_cast <Dtype_STRING *> (dtype);
            dtype_str->dtype.str_val = std::string ((char *)val);
            dtype->is_resolved = true;
        }
        break;

        default:
            assert(0);
    }

    return dtype;
}

static int 
sql_get_table_index_from_table_column (BPlusTree_t *tcatalog,
                                                                qep_struct_t *qep, 
                                                                char *table_column_name) {

    int i;
    BPluskey_t bpkey;

    char table_name[SQL_TABLE_NAME_MAX_SIZE];
    char column_name[SQL_COLUMN_NAME_MAX_SIZE];

    sql_get_column_table_names (qep, table_column_name, table_name, column_name);

    bpkey.key = (void *) table_name;
    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;

    ctable_val_t *ctable_val = (ctable_val_t *)BPlusTree_Query_Key (tcatalog, &bpkey);

    for (i = 0; i < qep->join.table_cnt; i++) {
        if (qep->join.tables[i].ctable_val == ctable_val) return i;
    }

    return -1;
}

bool 
sql_resolve_exptree (BPlusTree_t *tcatalog,
                                  sql_exptree_t *sql_exptree,
                                  qep_struct_t *qep,
                                  joined_row_t **joined_row) {

    int table_index;
    BPluskey_t bpkey;                                        
    MexprNode *node;
    const char *opnd_name;
    ctable_val_t *ctable_val;
    schema_rec_t *schema_rec = NULL;
    exp_tree_data_src_t *data_src = NULL;
    char table_name_out[SQL_TABLE_NAME_MAX_SIZE];
    char column_name_out[SQL_COLUMN_NAME_MAX_SIZE];

    MexprTree_Iterator_Operands_Begin (sql_exptree->tree, node) {

        if (sql_opnd_node_is_resolved (node)) continue;

        opnd_name = sql_get_opnd_variable_name(node).c_str();
        table_index = sql_get_table_index_from_table_column (
                                     tcatalog, qep, (char *)opnd_name);
        if (table_index < 0) return false;
        /* Assuming that Join is not supported on select SQL query yet*/
        ctable_val = qep->join.tables[table_index].ctable_val;

        sql_get_column_table_names (qep, (char *)opnd_name, 
                                        table_name_out, column_name_out);

        bpkey.key = (void *)column_name_out;
        bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
        schema_rec =  (schema_rec_t *) BPlusTree_Query_Key (
                                ctable_val->schema_table, &bpkey);

        if (!schema_rec) {
            printf("Error : %s(%d) : Column %s could not be found in table %s\n", 
                __FUNCTION__, __LINE__, table_name_out, column_name_out);
            return false;
        }

        data_src = (exp_tree_data_src_t *)calloc(1, sizeof(exp_tree_data_src_t)); 
        qep->data_src_lst->push_back (data_src);
        data_src->table_index = table_index;
        data_src->schema_rec = schema_rec;
        data_src->joined_row = joined_row;        
        InstallDtypeOperandProperties (node, 
                                                    schema_rec->dtype, data_src, 
                                                    sql_column_value_resolution_fn);
    } MexprTree_Iterator_Operands_End;

     if (!sql_exptree->tree->validate(sql_exptree->tree->root)) {

        printf ("Error : Sql Exp Tree Validation failed post resolution\n");
        return false;
     }

   return true;
}

bool 
sql_is_single_operand_expression_tree (sql_exptree_t *sql_exptree) {

    return sql_exptree->tree->IsLoneVariableOperandNode();
}

void 
InstallDtypeOperandProperties (MexprNode *node, 
                                                    sql_dtype_t sql_dtype,
                                                    void *data_src, Dtype *(*compute_fn_ptr)(char*, void *))  {

    Dtype_VARIABLE *dtype_var = 
                dynamic_cast <Dtype_VARIABLE *> (node);

    dtype_var->ResolveOperand (
                sql_to_mexpr_dtype_converter (sql_dtype), 
                data_src,
                compute_fn_ptr );
}

bool 
sql_opnd_node_is_resolved (MexprNode *opnd_node) {

    Dtype *dtype = dynamic_cast <Dtype *> (opnd_node);
    return dtype->is_resolved;
}

std::string 
sql_get_opnd_variable_name (MexprNode *opnd_node) {

    Dtype_VARIABLE *dtype = dynamic_cast <Dtype_VARIABLE *> (opnd_node);
    return dtype->dtype.variable_name;
}

MexprNode *
sql_tree_get_root (sql_exptree_t *tree) {
    
    if (!tree->tree || !tree->tree->root) return NULL;
    return tree->tree->root;
}


mexprcpp_dtypes_t
sql_to_mexpr_dtype_converter (sql_dtype_t sql_dtype) {

    switch (sql_dtype) {

        case SQL_INT:
            return MATH_CPP_INT;
        case SQL_DOUBLE:
            return MATH_CPP_DOUBLE;
        case SQL_STRING:
            return MATH_CPP_STRING;
        default:
            return MATH_CPP_DTYPE_INVALID;
    }
}

sql_dtype_t
mexpr_to_sql_dtype_converter (mexprcpp_dtypes_t dtype) {

    switch (dtype) {

        case MATH_CPP_INT:
            return SQL_INT;
        case MATH_CPP_DOUBLE:
            return SQL_DOUBLE;
        case MATH_CPP_STRING:
            return SQL_STRING;
        default:
            return SQL_DTYPE_MAX;
    }
}

dtype_value_t 
DTYPE_GET_VALUE(Dtype *dtype)   {

    dtype_value_t  dtype_value;

    dtype_value.dtype = mexpr_to_sql_dtype_converter(dtype->did);

    switch (dtype_value.dtype) {

        case SQL_INT: 
           dtype_value.u.int_val =  (dynamic_cast <Dtype_INT *>(dtype))->dtype.int_val;
           break;

        case SQL_STRING:
            dtype_value.u.str_val = (dynamic_cast <Dtype_STRING *>(dtype))->dtype.str_val.c_str();
            break;

        case SQL_DOUBLE:
            dtype_value.u.d_val = (dynamic_cast <Dtype_DOUBLE *>(dtype))->dtype.d_val;
            break;

        default:
            assert(0);
    }
    return dtype_value;
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

int
sql_tree_expand_all_aliases (qep_struct_t *qep, sql_exptree_t *sql_tree) {

    int count = 0;
    qp_col_t *sqp_col;
    std::string opnd_name;
    MexprNode *opnd_node;
    bool all_alias_expanded = false;

    while (!all_alias_expanded) {
        
        all_alias_expanded = true;

        MexprTree_Iterator_Operands_Begin  (sql_tree->tree, opnd_node) {

            opnd_name = sql_get_opnd_variable_name (opnd_node);

            sqp_col = sql_get_qp_col_by_name (
                                             qep->select.sel_colmns,
                                             qep->select.n, 
                                             (char *)opnd_name.c_str(), true);

            if (!sqp_col ) continue;

            all_alias_expanded = false;

            sql_concatenate_expr_trees (sql_tree, 
                                                          opnd_node, 
                                                          sql_clone_expression_tree( sqp_col->sql_tree ));

            count++;
            break;
        } MexprTree_Iterator_Operands_End;
    }
    return count;
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
sql_tree_operand_names_to_fqcn (qep_struct_t *qep, sql_exptree_t *sql_tree) {

    char *old_name;
    MexprNode *node;
    Dtype_VARIABLE *dvar;
    char new_name [SQL_FQCN_SIZE];
    char table_name[SQL_TABLE_NAME_MAX_SIZE];
    char column_name[SQL_COLUMN_NAME_MAX_SIZE];

     MexprTree_Iterator_Operands_Begin (sql_tree->tree, node) {

        dvar = dynamic_cast<Dtype_VARIABLE *> (node); 
        old_name = (char *)dvar->dtype.variable_name.c_str();
        memset (new_name, 0, sizeof (new_name));
        memset (table_name, 0, sizeof (table_name));
        memset (column_name, 0, sizeof (column_name));
        sql_get_column_table_names (qep, old_name, table_name, column_name);
        snprintf (new_name, sizeof(new_name), "%s.%s", table_name, column_name);
        dvar->dtype.variable_name.assign(std::string(new_name));

     } MexprTree_Iterator_Operands_End;
}