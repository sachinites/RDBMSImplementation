#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <memory.h>
#include <arpa/inet.h>
#include <list>
#include <vector>
#include "../stack/stack.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "qep.h"
#include "Catalog.h"
#include "../gluethread/glthread.h"
#include "sql_io.h"
#include "sql_utils.h"
#include "sql_group_by.h"
#include "sql_update.h"
#include "sql_order_by.h"
#include "SqlMexprIntf.h"
#include "../c-hashtable/hashtable.h"
#include "../c-hashtable/hashtable_itr.h"

extern BPlusTree_t TableCatalogDef;
extern void 
sql_process_delete_query (qep_struct_t *qep);

void 
table_iterators_init (qep_struct_t *qep,
                                table_iterators_t **_titer) {

    int i;

    if (*_titer == NULL) {

        (*_titer) = (table_iterators_t *)calloc (1, 
                        sizeof (table_iterators_t) + 
                        (sizeof (table_iter_data_t) * qep->join.table_cnt));
    }
    
    table_iterators_t *titer = (*_titer);

    titer->table_cnt = qep->join.table_cnt;
    
    for (i = 0 ; i < titer->table_cnt ; i++) {
        titer->table_iter_data[i].bpnode = NULL;
        titer->table_iter_data[i].index = 0;
        titer->table_iter_data[i].ctable_val  = qep->join.tables[i].ctable_val;
    }
}

bool 
qep_struct_record_table (qep_struct_t *qep_struct, char *table_name) {

    ctable_val_t *ctable_val;

    ctable_val = sql_catalog_table_lookup_by_table_name  (&TableCatalogDef, table_name);
    if (!ctable_val) return false;

    qep_struct->join.tables[qep_struct->join.table_cnt].ctable_val = ctable_val ;
    strncpy(qep_struct->join.tables[qep_struct->join.table_cnt].table_name , 
                    table_name, SQL_TABLE_NAME_MAX_SIZE );
    return true;
}

/* Query execution Plans Initialization fn */

static bool
sql_query_initialize_orderby_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    int i;
    qp_col_t *sqp_col;
    char table_name_out [SQL_TABLE_NAME_MAX_SIZE];
    char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE];

    if (qep->orderby.column_name[0] == '\0') return true;

    /* Has User specified order by column name as Alias name ?*/
    sqp_col = sql_get_qp_col_by_name (qep->select.sel_colmns,
                                                                qep->select.n,
                                                                qep->orderby.column_name,
                                                                true);

    if (!sqp_col) {

        parser_split_table_column_name (
                        qep->join.table_alias,
                        tcatalog,
                        qep->orderby.column_name,
                        table_name_out, lone_col_name);

        snprintf (qep->orderby.column_name, 
                    sizeof (qep->orderby.column_name),
                    "%s.%s", 
                    table_name_out[0] == '\0' ? \
                    qep->join.tables[0].table_name : table_name_out,
                    lone_col_name);

        sqp_col = sql_get_qp_col_by_name (qep->select.sel_colmns,
                                                                    qep->select.n,
                                                                    qep->orderby.column_name,
                                                                    false); 
    }

    if (!sqp_col) {

        printf ("Error : Order by columns is not recognized\n");
        return false;
    }

    qep->orderby.orderby_col_select_index = -1;

    for (i = 0; i < qep->select.n; i++) {
        
        if (qep->select.sel_colmns[i] != sqp_col) continue;
        qep->orderby.orderby_col_select_index = i;
        break;
    }

    assert (qep->orderby.orderby_col_select_index >= 0);

    return true;
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

    sql_tree_expand_all_aliases (qep, qep->where.gexptree);

    for (i = 0; i < qep->join.table_cnt; i++) {

        qep->where.exptree_per_table[i] = sql_clone_expression_tree(qep->where.gexptree);

        if (!qep->where.exptree_per_table[i]) {

            printf("Error : Failed to create Exp Tree Clones of Where Clause\n");
            return false;
        }

        if (!sql_resolve_exptree_against_table(
                        qep->join.table_alias,
                        tcatalog,
                        qep->where.exptree_per_table[i],
                        qep->join.tables[i].ctable_val, i, 
                        &qep->joined_row_tmplate,
                        qep->data_src_lst)) {

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
    ctable_val_t *ctable_val;
    glthread_t *col_list_head;
    char opnd_name[SQL_COMPOSITE_COLUMN_NAME_SIZE];

    if (qep->select.n ) return true;

    for (i = 0; i < qep->join.table_cnt; i++) {

        ctable_val = qep->join.tables[i].ctable_val;
        col_list_head = &ctable_val->col_list_head;

        ITERATE_GLTHREAD_BEGIN (col_list_head, curr) {

            lnode = glue_to_list_node (curr);
            qp_col = (qp_col_t *)calloc (1, sizeof (qp_col_t));
            qp_col->agg_fn = SQL_AGG_FN_NONE;
            qp_col->alias_name[0] = '\0';
            qp_col->alias_provided_by_user = false;
            /* Will allocate at the time of computation in select query*/
            qp_col->computed_value = NULL;
            memset(opnd_name, 0, sizeof(opnd_name));
            snprintf(opnd_name, sizeof(opnd_name), "%s.%s",
                     ctable_val->table_name, (char *)lnode->data);
            qp_col->sql_tree = sql_create_exp_tree_for_one_operand(opnd_name);
            strncpy(qp_col->alias_name, opnd_name, sizeof(qp_col->alias_name));
            qep->select.sel_colmns[qep->select.n++] = qp_col;

        } ITERATE_GLTHREAD_END (col_list_head, curr);
    }

    return true;
 }

static bool
sql_query_initialize_select_clause (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    int i;
    qp_col_t *qp_col;
    char table_name_out [SQL_TABLE_NAME_MAX_SIZE];
    char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE];

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
                if (sql_is_single_operand_expression_tree (qp_col->sql_tree)
                        && qp_col->alias_name[0] == '\0') {

                    parser_split_table_column_name (
                            qep->join.table_alias,
                            tcatalog,
                            QP_COL_NAME(qp_col),
                            table_name_out, lone_col_name);

                    snprintf (qp_col->alias_name,  sizeof (qp_col->alias_name),
                        "%s.%s", 
                        table_name_out[0] == '\0' ? \
                        qep->join.tables[0].table_name : table_name_out,
                        lone_col_name);
                }
            }
    }

    /* Now Resolve Expressression Trees for all select columns*/
    for (i = 0; i < qep->select.n; i++) {

        sql_tree_expand_all_aliases (qep, qep->select.sel_colmns[i]->sql_tree);

        if (!sql_resolve_exptree (&TableCatalogDef, 
                                                qep->select.sel_colmns[i]->sql_tree,
                                                qep, &qep->joined_row_tmplate)) {
            
            printf ("Error : Failed to resolve Expression Tree for select column %s\n", 
                        qep->select.sel_colmns[i]->alias_name);
            return false;
        }
    }    

    return true;
}

static void 
qep_create_alias_to_table_name_mapping (qep_struct_t *qep) {

    int i;

    qep->join.table_alias = new std::unordered_map<std::string, std::string>();

    for (i = 0; i < qep->join.table_cnt; i++) {

        if (qep->join.tables[i].alias_name[0] != '\0') {
            qep->join.table_alias->insert (
                {  std::string (qep->join.tables[i].alias_name),   
                    std::string (qep->join.tables[i].table_name)} );
        }
    }
}

static bool
sql_query_init_execution_plan (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    int i;
    bool rc;
    qp_col_t *qp_col;

    /* initialize the containers first as remaining initialization code is dependent on 
        these two allocations */
    qep->joined_row_tmplate = (joined_row_t *)calloc (1, sizeof (joined_row_t));
    qep->data_src_lst = new std::list<exp_tree_data_src_t *>();

    qep_create_alias_to_table_name_mapping (qep);

    /* Before we initialize anything, expand the select * first */
    rc = qep_resolve_select_asterisk (qep);
    if (!rc) return rc;
   
   /* The order in which we initializes the below clauses matters. Select
        clause must be initialized in the end because rest of the clauses clones
        expression trees of  select clause. */
    rc = sql_query_initialize_where_clause  (qep, tcatalog);
    if (!rc) return rc;

    rc = sql_query_initialize_groupby_clause (qep, tcatalog);
    if (!rc) return rc;

    rc = sql_query_initialize_having_clause (qep, tcatalog);
    if (!rc) return rc;

    rc = sql_query_initialize_select_clause (qep, tcatalog) ;
    if (!rc) return rc;

    rc = sql_query_initialize_orderby_clause (qep, tcatalog) ;
    if (!rc) return rc;
    
    rc = sql_query_initialize_update_query (qep, tcatalog);
    if (!rc) return rc;

    /* initialize other variables*/
    qep->is_join_started = false;
    qep->is_join_finished = false;

    /* initialize Iterators*/
    table_iterators_init (qep, &qep->titer);

    /* Initialize Joined Row*/
    joined_row_t *joined_row_tmplate = qep->joined_row_tmplate;
    joined_row_tmplate->size = qep->join.table_cnt;
    joined_row_tmplate->key_array = (BPluskey_t **) calloc (qep->join.table_cnt, sizeof (BPluskey_t *));
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

static void
table_iterators_first (qep_struct_t *qep_struct, 
                                 table_iterators_t *titer, 
                                 int table_id){

        bool rc;
        void *rec = NULL;
        BPluskey_t *bp_key;

        if (table_id < 0) return;

        do
        {
            rec = BPlusTree_get_next_record(
                        titer->table_iter_data[table_id].ctable_val->rdbms_table,
                        &titer->table_iter_data[table_id].bpnode,
                        &titer->table_iter_data[table_id].index,
                        &bp_key);

            /* No need to scan further*/
            if (!rec) {
                qep_struct->is_join_finished = true;
                return;
            }

            qep_struct->joined_row_tmplate->key_array[table_id] = bp_key;
            qep_struct->joined_row_tmplate->rec_array[table_id] = rec;

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
    BPluskey_t *bp_key;

    if (table_id < 0) return ;

    do
    {
        rec = BPlusTree_get_next_record(
                    titer->table_iter_data[table_id].ctable_val->rdbms_table,
                    &titer->table_iter_data[table_id].bpnode,
                    &titer->table_iter_data[table_id].index,
                    &bp_key);

        if (!rec) break;

        qep_struct->joined_row_tmplate->key_array[table_id] = bp_key;
        qep_struct->joined_row_tmplate->rec_array[table_id] = rec;

        rc = sql_evaluate_conditional_exp_tree (
                    qep_struct->where.exptree_per_table[table_id]);

    } while (!rc && titer->table_iter_data[table_id].bpnode);

    /* If record is found*/
    if (rec) {
            return;
    }
    else {
        qep_struct->joined_row_tmplate->key_array[table_id] = NULL;
        qep_struct->joined_row_tmplate->rec_array[table_id] = NULL;
        table_iterators_next(qep_struct, titer, table_id - 1);

        /* If We could not find qualified record in the top level table, abort the iteration */
        if (table_id == 0) {
            qep_struct->is_join_finished = true;
            return;
        }

        /* If secondary table finds that parent table could not find any qualified records, abort the iteration*/
        if (!qep_struct->joined_row_tmplate->rec_array[table_id - 1]) return;

        do {
            rec = BPlusTree_get_next_record(
                            titer->table_iter_data[table_id].ctable_val->rdbms_table,
                            &titer->table_iter_data[table_id].bpnode,
                            &titer->table_iter_data[table_id].index,
                            &bp_key);

            if (!rec) break;

            qep_struct->joined_row_tmplate->key_array[table_id] = bp_key;
            qep_struct->joined_row_tmplate->rec_array[table_id] = rec;

            rc = sql_evaluate_conditional_exp_tree (
                    qep_struct->where.exptree_per_table[table_id]);

        } while (!(rc) && titer->table_iter_data[table_id].bpnode);

        qep_struct->joined_row_tmplate->key_array[table_id] = bp_key;
        qep_struct->joined_row_tmplate->rec_array[table_id] = rec;
    }
}

bool
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
sql_process_select_query (qep_struct_t *qep) {

    int i;
    int row_no = 0;
    qp_col_t *qp_col;
    Dtype *computed_value;
    int  qualified_row_no = 0;
    bool is_aggregation = false;

    while (qep_execute_join (qep)) {

        /* Optimization : If only one table is involved, no need to evaluate join-predicate*/
        if (qep->join.table_cnt > 1 &&
                !qep_execute_join_predicate(qep, qep->joined_row_tmplate)) {
            continue;
        }

        row_no++;

         /* Time for Grouping */

        /* Check if the query has group by clause */
        if (qep->groupby.n) {

            sql_group_by_clause_group_records_phase1 (qep);
            continue;
        }

        for (i = 0; i < qep->select.n; i++) {

            qp_col = qep->select.sel_colmns[i];

            if (qp_col->agg_fn == SQL_AGG_FN_NONE) {
                
                /* Flush the old result*/
                if (qp_col->computed_value ) {
                     sql_destroy_Dtype_value_holder (qp_col->computed_value);
                     qp_col->computed_value = NULL;
                }
                qp_col->computed_value = sql_evaluate_exp_tree (qp_col->sql_tree);
            }
            else {
                /* Process Agg fn*/

                is_aggregation = true;

                if (!qp_col->aggregator) {
                    qp_col->computed_value =  sql_evaluate_exp_tree (qp_col->sql_tree);
                    
                    qp_col->aggregator = sql_get_aggregator (
                            qp_col->agg_fn,
                            sql_dtype_get_type (qp_col->computed_value));

                    assert (qp_col->aggregator);
                    sql_column_value_aggregate  (qp_col,  qp_col->computed_value);
                    sql_destroy_Dtype_value_holder (qp_col->computed_value);
                    qp_col->computed_value = NULL;
                }
                else {
                    computed_value = sql_evaluate_exp_tree (qp_col->sql_tree);
                    sql_column_value_aggregate  (qp_col, computed_value);
                    sql_destroy_Dtype_value_holder (computed_value);
                }
                
            }
        }

        /* Output  */
        /* Case 1 :  No Group by Clause, Non-Aggregated Columns */
        if (!qep->groupby.n && !is_aggregation) {

            if (qep_collect_dtypes_for_sorting(qep)) continue;

            if (row_no == 1) {
                sql_print_hdr  (qep, qep->select.sel_colmns, qep->select.n);
            }

            sql_emit_select_output (qep, qep->select.n, qep->select.sel_colmns);

            if (qep->limit == row_no) {
                break;
            }
        }
    }  /* While ends */


    /* Handling group by Phase 2*/
    if (qep->groupby.n) {

        sql_group_by_clause_process_grouped_records_phase2 (qep); 
        return;
    }

    /* Case 2 :  No Group by Clause,  Aggregated Columns */
    if (!qep->groupby.n && is_aggregation) {

        sql_print_hdr(qep, qep->select.sel_colmns, qep->select.n);
        sql_emit_select_output(qep, qep->select.n, qep->select.sel_colmns);
        printf ("(1 rows)\n");
        return;
    }

    /* Order by*/
    qep_orderby_sort (qep);
    qep->orderby.iterator_index = 0;
    while (qep_order_by_reassign_select_columns (qep)) {

        if ( qep->orderby.iterator_index == 1) {
            sql_print_hdr(qep, qep->select.sel_colmns, qep->select.n);
        }
        sql_emit_select_output(qep, qep->select.n, qep->select.sel_colmns);
        sql_select_flush_computed_values (qep);
        if (qep->limit == qep->orderby.iterator_index) {
            break;
        }
    }
    printf ("(%d rows)\n", row_no);
}

void 
sql_execute_qep (qep_struct_t *qep) {

    if (!sql_query_init_execution_plan (qep, &TableCatalogDef)) {

        printf ("Error : Failed to initialize Query Execution Plan\n");
        return;
    }

    switch (qep->query_type) {

        case SQL_SELECT_Q:
            sql_process_select_query (qep);
            break;
        case SQL_DELETE_Q:
            sql_process_delete_query (qep);
            break;
        case SQL_UPDATE_Q:
            sql_process_update_query (qep);
            break;
        default:
            printf ("Error : Could not identify Query type\n");
            break;
    }
}

void 
qep_deinit (qep_struct_t *qep) {

    int i;
    qp_col_t *qp_col;
    struct hashtable_itr *itr;
    joined_row_t *joined_row;
    exp_tree_data_src_t *data_src;
    std::list<joined_row_t *> *record_lst;
    ht_group_by_record_t *ht_group_by_record;

    if (qep->where.gexptree) {
        
        sql_destroy_exp_tree (qep->where.gexptree);
        qep->where.gexptree = NULL;
    }

    if (qep->groupby.n) {

        for (i = 0; i < qep->groupby.n; i++) {

            qp_col = qep->groupby.col_list[i];

            if (qp_col->sql_tree) {
                sql_destroy_exp_tree (qp_col->sql_tree );
                qp_col->sql_tree = NULL;
                if (qp_col->computed_value) {
                    sql_destroy_Dtype_value_holder (qp_col->computed_value);
                    qp_col->computed_value = NULL;
                }
                else if (qp_col->aggregator){
                    sql_destroy_aggregator (qp_col);
                }
            }
            free(qp_col);
        }

        if (qep->groupby.ht) {

            itr = hashtable_iterator(qep->groupby.ht);

            do {

                ht_group_by_record =  (ht_group_by_record_t *)hashtable_iterator_value (itr);
                record_lst = ht_group_by_record->record_lst;

                while (!record_lst->empty()) {

                    joined_row = record_lst->front();
                    record_lst->pop_front();
                    free(joined_row->key_array);
                    free(joined_row->rec_array);
                    free(joined_row); 
                }
                delete  record_lst;
                ht_group_by_record->record_lst = NULL;

            } while (hashtable_iterator_advance(itr));

            free(itr);
            hashtable_destroy(qep->groupby.ht, 1);
            qep->groupby.ht = NULL;
        }
    }

    if (qep->having.gexptree_phase1) {
        sql_destroy_exp_tree (qep->having.gexptree_phase1);
        qep->having.gexptree_phase1 = NULL;
    }

    if (qep->having.gexptree_phase2) {
        sql_destroy_exp_tree (qep->having.gexptree_phase2);
        qep->having.gexptree_phase2 = NULL;
    }

    if (qep->select.n) {

        for (i = 0; i < qep->select.n; i++) {

            qp_col = qep->select.sel_colmns[i];
            if (qp_col->sql_tree) {
                 sql_destroy_exp_tree (qp_col->sql_tree);
                qp_col->sql_tree = NULL;
            }

            if (qp_col->computed_value) {
                sql_destroy_Dtype_value_holder(qp_col->computed_value);
                qp_col->computed_value = NULL;
            }
            else if (qp_col->aggregator){
                sql_destroy_aggregator (qp_col);
            }
            free(qp_col);            
        }
    }

    free (qep->titer);
    qep->titer = NULL;

    /* Free update query resources*/
    if (qep->update.n) {

        for (i = 0; i < qep->update.n; i++) {
            sql_destroy_exp_tree (qep->update.upd_colmns[i].value_exptree);
            qep->update.upd_colmns[i].value_exptree = NULL;
        }
        qep->update.n = 0;
    }

    if (qep->joined_row_tmplate) {
        free (qep->joined_row_tmplate->key_array);
        free (qep->joined_row_tmplate->rec_array);
        free(qep->joined_row_tmplate->schema_table_array);
        free(qep->joined_row_tmplate->table_id_array);
        free(qep->joined_row_tmplate);
    }

    /* Free Data Srcs*/
    if (qep->data_src_lst) {

        while (!qep->data_src_lst->empty()) {

            data_src = qep->data_src_lst->front();
            qep->data_src_lst->pop_front();
            free(data_src);
        }
        delete qep->data_src_lst;
        qep->data_src_lst = NULL;
    }

    /* Free Alias Hashmap*/
    if (qep->join.table_alias) {

        qep->join.table_alias->clear();
        delete qep->join.table_alias;
        qep->join.table_alias = NULL;
    }

    /* Free order by Vector */
    for (i =0; i < qep->orderby.pVector.size(); i++) {

        std::vector < Dtype *> *cVector = qep->orderby.pVector.at(i);
        if (!cVector) continue;
        for (int j = 0; j < cVector->size(); j++) {
            sql_destroy_Dtype_value_holder (cVector->at(j));
        }
        delete cVector;
        qep->orderby.pVector[i] = NULL;
    }

     if (qep->orderby.pVector.size()) {
        qep->orderby.pVector.clear();
     }

}
