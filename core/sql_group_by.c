#include <assert.h>
#include <list>
#include <memory.h>
#include "Catalog.h"
#include "qep.h"
#include "sql_group_by.h"
#include "SqlMexprIntf.h"
#include "../c-hashtable/hashtable.h"
#include "../c-hashtable/hashtable_itr.h"
#include "sql_utils.h"
#include "../../MathExpressionParser/Dtype.h"

bool
sql_query_initialize_groupby_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    bool rc;
    int i, j;
    int tindex;
    ctable_val_t *ctable_val;
    MexprNode *exptree_node;
    qp_col_t *gqp_col, *sqp_col;
    bool all_alias_resolved = false;
    char table_name_out [SQL_TABLE_NAME_MAX_SIZE];
    char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE];

    if (qep->groupby.n == 0)  return true;

    /* Three cases :
    1. The group by column gqp_col is an alias name
    2. The group by column gqp_col is a table column name
    3. The group by column is an expression */

    for (i = 0; i < qep->groupby.n; i++) {

        gqp_col = qep->groupby.col_list[i];

        if (sql_is_single_operand_expression_tree (gqp_col->sql_tree)) {
            
            sqp_col = sql_get_qp_col_by_name (
                                qep->select.sel_colmns,
                                qep->select.n, 
                                QP_COL_NAME(gqp_col), true);

            // 1. The group by column gqp_col is an alias name
            if (sqp_col) {

                if (sqp_col->agg_fn != SQL_AGG_FN_NONE) {

                    printf ("Error : Aggregate fn cannot be applied on column %s\n", sqp_col->alias_name);
                    return false;
                }

                sql_tree_expand_all_aliases (qep, gqp_col->sql_tree);

                rc =  (sql_resolve_exptree (tcatalog, 
                                                             gqp_col->sql_tree, 
                                                             qep,
                                                             qep->joined_row_tmplate));

                if (!rc) {

                    printf ("Error : Group by Column %s could not be resolved\n", sqp_col->alias_name);
                    return false;
                }

                sqp_col->link_to_groupby_col = gqp_col;
            }

            else {
                /* gqp_col could be pure table column name,  which may or may not exist in select list
                    2. The group by column gqp_col is a table column name
                */
                
                parser_split_table_column_name (
                        QP_COL_NAME (gqp_col),
                        table_name_out, lone_col_name);        
                
                tindex = -1;

                if (table_name_out[0] == '\0' ) {
                        
                    tindex = 0;
                    ctable_val = qep->join.tables[0].ctable_val;
                }
                else {

                    ctable_val =  sql_catalog_table_lookup_by_table_name (tcatalog, table_name_out);

                    if (!ctable_val) {
                        printf ("Error : Table name %s do not exist\n", table_name_out);
                        return false;
                    }

                    for (j = 0; j < qep->join.table_cnt; j++) {

                        if (ctable_val != qep->join.tables[j].ctable_val) continue;
                        tindex = j;
                        break;
                    }

                    if (tindex == -1) {

                        printf("Error : Table %s is not specified in Join list\n", table_name_out);
                        return false;
                    }
                }

                rc = sql_resolve_exptree_against_table (gqp_col->sql_tree, 
                                                                                ctable_val, 
                                                                                tindex,
                                                                                qep->joined_row_tmplate);                
                if (!rc) {

                    printf ("Error : Group by column %s could not be resolved against table %s\n",
                                    QP_COL_NAME(gqp_col), ctable_val->table_name);
                    return false;
                }

                sqp_col = sql_get_qp_col_by_name ( qep->select.sel_colmns, qep->select.n, 
                                                                            QP_COL_NAME(gqp_col), false);

                if (sqp_col) {

                    sqp_col->link_to_groupby_col = gqp_col;
                }

            }
        }

        // 3. The group by column is an expression */

        sql_tree_expand_all_aliases (qep, gqp_col->sql_tree);

        rc =  (sql_resolve_exptree (tcatalog, 
                                                    gqp_col->sql_tree, 
                                                    qep,
                                                    qep->joined_row_tmplate));                

        if (!rc) {

            printf ("Error : Could not resolve Unnamed %dth group by Expression Tree\n", i);
            return false;
        }
    }

    return true;
}

/* In HAVING condidion, variables with aggrgated fn applied on them do not contribute
    to the evaluation of having condition in phase 1. Reformed the expression tree by
    getting rid of such variables */
static bool
sql_query_initialize_having_clause_phase1 (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    qp_col_t *sqp_col;
    std::string opnd_name;
    MexprNode *opnd_node;

     SqlExprTree_Iterator_Operands_Begin (qep->having.gexptree_phase1, opnd_node) {

        sqp_col = sql_get_qp_col_by_name (
                                             qep->select.sel_colmns,
                                             qep->select.n, 
                                             (char *)opnd_name.c_str(), true);
       
        if (sqp_col) {

            if(sqp_col->agg_fn != SQL_AGG_FN_NONE) {
                sql_opnd_node_mark_unresolvable(opnd_node);
            }
        }
        else {
            
             sqp_col = sql_get_qp_col_by_name (
                                             qep->select.sel_colmns,
                                             qep->select.n, 
                                             (char *)opnd_name.c_str(), false);

             if (sqp_col) {

                 if (sqp_col->agg_fn != SQL_AGG_FN_NONE) {

                     sql_opnd_node_mark_unresolvable(opnd_node);
                 }
             }
        }
     }

     sql_tree_expand_all_aliases (qep, qep->having.gexptree_phase1);
     sql_resolve_exptree (tcatalog, qep->having.gexptree_phase1, qep, qep->joined_row_tmplate);
     sql_tree_remove_unresolve_operands (qep->having.gexptree_phase1);
    return true;
}


static bool
sql_query_initialize_having_clause_phase2 (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    return true;
}

bool
sql_query_initialize_having_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    if (qep->having.gexptree_phase1 == NULL) return true;

    if (qep->having.gexptree_phase2 == NULL) {
        qep->having.gexptree_phase2 = sql_clone_expression_tree (qep->having.gexptree_phase1);
    }

    qep->having.having_phase = 1;

    bool rc = sql_query_initialize_having_clause_phase1 (qep, tcatalog);
    if (!rc) return false;
    return sql_query_initialize_having_clause_phase2 (qep, tcatalog);
}


/* This fn setup the hashtable also if it is a first record we are grouping*/
void 
sql_group_by_clause_group_records (qep_struct_t *qep) {
    
    int i;
    char *ht_key;  
    Dtype *dtype;
    qp_col_t *qp_col;
    int ht_key_size = 0;
    unsigned int string_hash = 0;
    bool is_ht_exist = qep->groupby.ht ? true : false;

    /* Filter unwanted records */
    if (!sql_evaluate_conditional_exp_tree (qep->having.gexptree_phase1)) {

        printf ("Group by phase 1 : Filtered\n");
        return;
    };

    ht_key = (char *)calloc (is_ht_exist ? qep->groupby.ht_key_size :  SQL_MAX_HT_KEY_SIZE, 1);

    for (i = 0; i < qep->groupby.n; i++) {

        qp_col = qep->groupby.col_list[i];
        dtype = sql_evaluate_exp_tree (qp_col->sql_tree);

        if (dtype->did == MATH_CPP_STRING) {
            /* String string are of verying sizes, therefore, we will compute a
            fixed size 4 B hashcode of the strings to store as key in HT. Hashtable
            can store keys of fixed size only*/

            string_hash = hashfromkey ((void *)(dynamic_cast <Dtype_STRING*>(dtype))->dtype.str_val.c_str());
            memcpy ((void *) (ht_key + ht_key_size), &string_hash, sizeof (string_hash));
            ht_key_size +=  sizeof (string_hash);
            sql_destroy_Dtype_value_holder(dtype);
            assert (ht_key_size < SQL_MAX_HT_KEY_SIZE);
            continue;
        }

        ht_key_size +=  sql_dtype_serialize (dtype, (void *)(ht_key + ht_key_size));
        sql_destroy_Dtype_value_holder(dtype);
        assert (ht_key_size < SQL_MAX_HT_KEY_SIZE);
    }

    if (!is_ht_exist) {
        
        ht_key = (char *)realloc ( (void *)ht_key, ht_key_size);

        qep->groupby.ht = create_hashtable(
                            ht_key_size,
                            hashfromkey, equalkeys);
    
        qep->groupby.ht_key_size = ht_key_size;
    }
    else {
        assert (ht_key_size ==  qep->groupby.ht_key_size);
    }

    /* Create a new joined_row to be inserted into HT*/
    joined_row_t *joined_row = (joined_row_t *)calloc (1, sizeof (joined_row_t));
    memcpy (joined_row, qep->joined_row_tmplate, sizeof (joined_row_t));

    std::list<joined_row_t *> *record_lst = 
        reinterpret_cast  <std::list<joined_row_t *> *> (hashtable_search (qep->groupby.ht , ht_key));

    if (record_lst) {
        record_lst->push_back (joined_row);
    }
    else {
        std::list<joined_row_t *> *record_lst = new std::list<joined_row_t *>();
         record_lst->push_back (joined_row);
        assert((hashtable_insert (qep->groupby.ht, (void *)ht_key, (void *)record_lst)));
    }

    printf ("Group by Phase 1 : Added to HT\n");
}

void 
sql_process_group_by_grouped_records (qep_struct_t *qep) {


}


