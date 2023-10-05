#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <memory.h>
#include <arpa/inet.h>
#include "../stack/stack.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "qep.h"
#include "Catalog.h"
#include "../gluethread/glthread.h"
#include "sql_io.h"
#include "sql_utils.h"
#include "sql_groupby.h"
#include "../c-hashtable/hashtable.h"
#include "../c-hashtable/hashtable_itr.h"
#include "sql_mexpr_intf.h"
#include "../../MathExpressionParser/MExpr.h"

extern BPlusTree_t TableCatalogDef;

static void 
table_iterators_init (qep_struct_t *qep,
                                table_iterators_t **_titer) {

    int i;

    (*_titer) = (table_iterators_t *)calloc (1, 
                        sizeof (table_iterators_t) + 
                        (sizeof (table_iter_data_t) * qep->join.table_cnt));
    
    table_iterators_t *titer = (*_titer);

    titer->table_cnt = qep->join.table_cnt;
    
    for (i = 0 ; i < titer->table_cnt ; i++) {
        titer->table_iter_data[i].bpnode = NULL;
        titer->table_iter_data[i].index = 0;
        titer->table_iter_data[i].ctable_val  = qep->join.tables[i].ctable_val;
    }
}

bool 
qep_struct_record_table (qep_struct_t *qep_struct, unsigned char *table_name) {

    ctable_val_t *ctable_val;

    ctable_val = sql_catalog_table_lookup_by_table_name  (&TableCatalogDef, table_name);
    if (!ctable_val) return false;

    qep_struct->join.tables[qep_struct->join.table_cnt++].ctable_val = ctable_val ;
    return true;
}

/* Query execution Plans Initialization fn */

static bool
sql_query_initialize_orderby_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    return true;
}

static bool
sql_query_initialize_having_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    qep->having.having_phase = 1;

    if (qep->having.gexptree) {


    }
    return true;
}

static bool
sql_query_initialize_groupby_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    if (qep->groupby.n == 0)  return true;
}

static bool
sql_query_initialize_where_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    int i;

    if (!qep->where.gexptree) return true;

    /* Initializing Where Clause
        Initializing Where clause in 3 steps :
        1. Check for operands in the where.gexptree, and if the operand is an Alias of columns specified in 
            select clause. Replace these operand nodes with select expression trees for that operand.
        2. Clone and Resolve where.gexptree to per-table expression trees for each table in join table list
        3. Resolve where.gexptree against all join tables put together
    */

   int opnd_len;
    qp_col_t *qp_col;
    mexpt_tree_t *clone_tree;
    mexpt_node_t *opnd_node;
    bool alias_resolved = false;
    bool all_alias_resolved = false;

    while (!all_alias_resolved) {

        all_alias_resolved = true;
        alias_resolved = false;

        mexpt_iterate_operands_begin (qep->where.gexptree->tree, opnd_node) {

            if (opnd_node->u.opd_node.is_resolved) continue;

            /* All Unresolved nodes are opernads of type 'variable'*/
            opnd_len = strlen(opnd_node->u.opd_node.opd_value.variable_name);

            for (i = 0; i < qep->select.n; i++) {

                qp_col = qep->select.sel_colmns[i];
                if (!qp_col->alias_provided_by_user)
                    continue;

                if ((strlen(qp_col->alias_name) != opnd_len) ||
                    (strncmp(qp_col->alias_name,
                             opnd_node->u.opd_node.opd_value.variable_name,
                             SQL_ALIAS_NAME_LEN)))
                    continue;

                all_alias_resolved = false;
                clone_tree = mexpt_clone(qp_col->sql_tree->tree);
                assert(clone_tree);

                if (!mexpt_concatenate_mexpt_trees(qep->where.gexptree->tree,
                                                   opnd_node,
                                                   clone_tree)) {

                    printf("Error : %s(%d) Failed to resolve Where clause Alias name %s\n",
                           __FUNCTION__, __LINE__, qp_col->alias_name);
                    return false;
                }
                alias_resolved = true;
                break;
            }

            if (!all_alias_resolved || alias_resolved) break;

        } mexpt_iterate_operands_end (qep->where.gexptree->tree, opnd_node)
    }


    for (i = 0; i < qep->join.table_cnt; i++) {

        qep->where.exptree_per_table[i] = (sql_exptree_t *)calloc(1, sizeof(sql_exptree_t));
        qep->where.exptree_per_table[i]->tree = mexpt_clone(qep->where.gexptree->tree);

        if (!qep->where.exptree_per_table[i]->tree) {

            printf("Error : Failed to create Exp Tree Clones of Where Clause\n");
            return false;
        }

        if (!sql_resolve_exptree_against_table(
                        qep->where.exptree_per_table[i],
                        qep->join.tables[i].ctable_val, i, 
                        &qep->joined_row_tmplate)) {

            printf("Error : Failed to resolve per table Where Expression Tree\n");
            return false;
        }
    }


    if (!sql_resolve_exptree(&TableCatalogDef,
                             qep->where.gexptree,
                             qep, &qep->joined_row_tmplate)) {

        printf("Error : Failed to resolve Global Where Expression Tree\n");
        return false;
    }

    return true;
}

static bool 
qep_resolve_select_asterisk (qep_struct_t *qep) {

    int i;
    glthread_t *curr;
    qp_col_t *qp_col;    
    list_node_t *lnode;
    glthread_t *col_list_head;
    ctable_val_t *ctable_val;
    unsigned char opnd_name[SQL_COMPOSITE_COLUMN_NAME_SIZE];

    if (qep->select.n ) return true;

    for (i = 0; i < qep->join.table_cnt; i++) {

        ctable_val = qep->join.tables[i].ctable_val;
        col_list_head = &ctable_val->col_list_head;

        ITERATE_GLTHREAD_BEGIN (col_list_head, curr) {

            lnode = glue_to_list_node (curr);
            qp_col = (qp_col_t *)calloc (1, sizeof (qp_col_t));
            qp_col->agg_fn = SQL_AGG_FN_NONE;
            qp_col->alias_name[0] = '\0';
            qp_col->computed_value.dtype = MEXPR_DTYPE_INVALID;
            if (qep->join.table_cnt > 1) {
                memset (opnd_name, 0, sizeof (opnd_name));
                snprintf (opnd_name, sizeof(opnd_name), "%s.%s", 
                    ctable_val->table_name, (unsigned char *)lnode->data);
                qp_col->sql_tree = sql_create_exp_tree_for_one_operand (opnd_name);
                strncpy (qp_col->alias_name, opnd_name, sizeof (qp_col->alias_name));
            }
            else {
                qp_col->sql_tree = sql_create_exp_tree_for_one_operand ((unsigned char *)lnode->data);
                strncpy (qp_col->alias_name, (unsigned char *)lnode->data, sizeof (qp_col->alias_name));
            }
            qep->select.sel_colmns[qep->select.n++] = qp_col;

        } ITERATE_GLTHREAD_END (col_list_head, curr);
    }

    return true;
 }

static bool
sql_query_initialize_select_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    int i;
    qp_col_t *qp_col;

    if (qep->select.n == 0) {

        /* Expand * (asterisk) here */
        if (!qep_resolve_select_asterisk (qep)) {

            printf ("Error : Failed to resolve asterisk\n");
            return false;
        }
    }
    else {
        /* Fix up the alias name. For those qp_cols whose alias were not
            specified by the user, if those are single operand node, then take
            variable name as alias name*/
            for (i = 0; i < qep->select.n; i++) {

                qp_col = qep->select.sel_colmns[i];
                if (sql_is_expression_tree_only_operand (qp_col->sql_tree)
                        && qp_col->alias_name[0] == '\0') {
                    strncpy(qp_col->alias_name, 
                            qp_col->sql_tree->tree->root->u.opd_node.opd_value.variable_name,
                            sizeof (qp_col->alias_name));
                }
            }
    }
    /* Now Resolve Expressression Trees for all select columns*/
    for (i = 0; i < qep->select.n; i++) {

        if (!sql_resolve_exptree (&TableCatalogDef, 
                                                qep->select.sel_colmns[i]->sql_tree,
                                                qep, &qep->joined_row_tmplate)) {
            
            printf ("Error : Failed to resolve Global Where Expression Tree "
                    "against %d th columns\n", i);
            return false;
        }
    }    

    return true;
}

static bool
sql_query_init_execution_plan (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    int i;
    bool rc;
    qp_col_t *qp_col;

   qep->stage_id = QP_NODE_SEQ_SCAN;

    /* Before we initialize anything, expand the select * first */
    rc = qep_resolve_select_asterisk (qep);
    if (!rc) return;
   
   /* The order in which we initializes the below clauses matters. Select
        clause must be initialized in the end because rest of the clauses clones
        expression trees of  select clause. */
    rc = sql_query_initialize_where_clause  (qep, tcatalog);
    if (!rc) return rc;

    rc = sql_query_initialize_groupby_clause (qep, tcatalog);
    if (!rc) return rc;

    rc = sql_query_initialize_having_clause (qep, tcatalog);
    if (!rc) return rc;

    rc = sql_query_initialize_orderby_clause (qep, tcatalog) ;
    if (!rc) return rc;

    rc = sql_query_initialize_select_clause (qep, tcatalog) ;
    if (!rc) return rc;

    /* initialize other variables*/
    qep->limit = 0;
    qep->is_join_started = false;
    qep->is_join_finished = false;

    /* initialize Iterators*/
    table_iterators_init (qep, &qep->titer);

    /* Initialize Joined Row*/
    joined_row_t *joined_row_tmplate = &qep->joined_row_tmplate;
    joined_row_tmplate->size = qep->join.table_cnt;
    joined_row_tmplate->rec_array = (void **) calloc (qep->join.table_cnt, sizeof (void *));
    joined_row_tmplate->schema_table_array = (BPlusTree_t **)
        calloc (qep->join.table_cnt, sizeof (BPlusTree_t *));
    joined_row_tmplate->table_id_array = (int *)calloc (qep->join.table_cnt, sizeof (int));

    for (i = 0; i < qep->join.table_cnt; i++) {
        joined_row_tmplate->schema_table_array[i] = 
            qep->join.tables[i].ctable_val->schema_table;
        joined_row_tmplate->table_id_array[i] = i;
    }    

    return true;
}

void 
qep_deinit (qep_struct_t *qep) {

    int i;
    qp_col_t *qp_col;

    if (qep->where.gexptree) {
        mexpt_destroy (qep->where.gexptree->tree->root, true);
        free (qep->where.gexptree->tree);
        free(qep->where.gexptree);
        qep->where.gexptree = NULL;
    }

    if (qep->groupby.n) {

        for (i = 0; i < qep->groupby.n; i++) {

            qp_col = qep->groupby.col_list[i];

            if (qp_col->sql_tree) {
                mexpt_destroy(qp_col->sql_tree->tree->root, true);
                free(qp_col->sql_tree->tree);
                free(qp_col->sql_tree);
                qp_col->sql_tree = NULL;
            }
            free(qp_col);
        }

        if (qep->groupby.ht) {

            hashtable_destroy(qep->groupby.ht, 1);
            qep->groupby.ht = NULL;
        }
    }

    if (qep->having.gexptree) {
        mexpt_destroy (qep->having.gexptree->tree->root, true);
        free (qep->having.gexptree->tree);
        free(qep->having.gexptree);
        qep->having.gexptree = NULL;
    }

    if (qep->select.n) {

        for (i = 0; i < qep->select.n; i++) {

            qp_col = qep->select.sel_colmns[i];
            if (qp_col->sql_tree) {
                mexpt_destroy(qp_col->sql_tree->tree->root, true);
                free(qp_col->sql_tree->tree);
                free(qp_col->sql_tree);
                qp_col->sql_tree = NULL;
            }
            free(qp_col);            
        }
    }

    free (qep->titer);
    qep->titer = NULL;

    free (qep->joined_row_tmplate.rec_array);
    free(qep->joined_row_tmplate.schema_table_array);
    free(qep->joined_row_tmplate.table_id_array);
}


static void
table_iterators_first (qep_struct_t *qep_struct, 
                                 table_iterators_t *titer, 
                                 int table_id){

        bool rc;
        void *rec = NULL;

        if (table_id < 0) return;

        do
        {
            rec = BPlusTree_get_next_record(
                        titer->table_iter_data[table_id].ctable_val->rdbms_table,
                        &titer->table_iter_data[table_id].bpnode,
                        &titer->table_iter_data[table_id].index);

            /* No need to scan further*/
            if (!rec) {
                qep_struct->is_join_finished = true;
                return;
            }

            qep_struct->joined_row_tmplate.rec_array[table_id] = rec;

            rc = sql_evaluate_conditional_exp_tree (
                    qep_struct->where.exptree_per_table[table_id]);

        } while (!rc && titer->table_iter_data[table_id].bpnode);

        table_iterators_first (qep_struct, titer, table_id -1);
}

static void
table_iterators_next (qep_struct_t *qep_struct, 
                                  table_iterators_t *titer, 
                                  int table_id) {

    bool rc;
    void *rec = NULL;

    if (table_id < 0) return ;

    do
    {
        rec = BPlusTree_get_next_record(
                    titer->table_iter_data[table_id].ctable_val->rdbms_table,
                    &titer->table_iter_data[table_id].bpnode,
                    &titer->table_iter_data[table_id].index);

        if (!rec) break;

        qep_struct->joined_row_tmplate.rec_array[table_id] = rec;

        rc = sql_evaluate_conditional_exp_tree (
                    qep_struct->where.exptree_per_table[table_id]);

    } while (!rc && titer->table_iter_data[table_id].bpnode);

    /* If record is found*/
    if (rec) {
            return;
    }
    else {
        qep_struct->joined_row_tmplate.rec_array[table_id] = NULL;
        table_iterators_next(qep_struct, titer, table_id - 1);

        /* If We could not find qualified record in the top level table, abort the iteration */
        if (table_id == 0) {
            qep_struct->is_join_finished = true;
            return;
        }

        /* If secondary table finds that parent table could not find any qualified records, abort the iteration*/
        if (!qep_struct->joined_row_tmplate.rec_array[table_id - 1]) return;

        do {
            rec = BPlusTree_get_next_record(
                            titer->table_iter_data[table_id].ctable_val->rdbms_table,
                            &titer->table_iter_data[table_id].bpnode,
                            &titer->table_iter_data[table_id].index);

            if (!rec) break;

            qep_struct->joined_row_tmplate.rec_array[table_id] = rec;

            rc = sql_evaluate_conditional_exp_tree (
                    qep_struct->where.exptree_per_table[table_id]);

        } while (!(rc) && titer->table_iter_data[table_id].bpnode);

        qep_struct->joined_row_tmplate.rec_array[table_id] = rec;
    }
}

static bool
qep_execute_join (qep_struct_t *qep_struct) {

   if (!qep_struct->is_join_started) {

        table_iterators_first (qep_struct, qep_struct->titer, qep_struct->join.table_cnt -1);

        qep_struct->is_join_started = true;

        /* We could not get Ist Qualified record from each joined tables*/
        if (qep_struct->is_join_finished) return false;
        return true;
   }

    table_iterators_next (qep_struct, qep_struct->titer, qep_struct->join.table_cnt -1);
    return !qep_struct->is_join_finished;
}


static bool
qep_execute_join_predicate (qep_struct_t *qep_struct, joined_row_t *joined_row) {

   if (!qep_struct->where.gexptree) return true;
   return sql_evaluate_conditional_exp_tree (qep_struct->where.gexptree);
}


void 
sql_execute_qep (qep_struct_t *qep) {

    int i;
    int row_no = 0;
    qp_col_t *qp_col;
    bool is_aggregation = false;

    while (qep_execute_join (qep)) {

        /* Optimization : If only one table is involved, no need to evaluate join-predicate*/
        if (qep->join.table_cnt > 1 &&
                !qep_execute_join_predicate(qep, &qep->joined_row_tmplate)) {
            continue;
        }

        row_no++;

         /* Time for Grouping */

        /* Check if the query has group by clause */
        if (qep->groupby.n) {

        }

        for (i = 0; i < qep->select.n; i++) {

            qp_col = qep->select.sel_colmns[i];

            if (qp_col->agg_fn = SQL_AGG_FN_NONE) {
                
                qp_col->computed_value = sql_evaluate_exp_tree (qp_col->sql_tree);
            }
            else {
                /* Process Agg fn*/
            }
        }

        /* Output  */
        /* Case 1 :  No Group by Clause, Non-Aggregated Columns */
        if (!qep->groupby.n && !is_aggregation) {

            if (row_no == 1) {
                sql_print_hdr  (qep->select.sel_colmns, qep->select.n);
            }

            sql_emit_select_output (qep->select.n, qep->select.sel_colmns);

            if (qep->limit == row_no) {
                break;
            }
        }        

    }  /* While ends */
}

void 
sql_process_select_query (qep_struct_t *qep) {

    int i;

    if (!sql_query_init_execution_plan (qep, &TableCatalogDef)) {

        printf ("Error : Failed to initialize Query Execution Plan\n");
        return;
    }

    sql_execute_qep (qep);
}