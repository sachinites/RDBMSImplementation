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
#include "sql_group_by.h"
#include "../c-hashtable/hashtable.h"
#include "../c-hashtable/hashtable_itr.h"
#include "SqlMexprIntf.h"

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
qep_struct_record_table (qep_struct_t *qep_struct, char *table_name) {

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

    bool rc;
    int i, j, k;
    int tindex;
    ctable_val_t *ctable_val;
    MexprNode *exptree_node;
    qp_col_t *gqp_col, *sqp_col;
    bool all_alias_resolved = false;
    char table_name_out [SQL_TABLE_NAME_MAX_SIZE];
    char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE];

    if (qep->groupby.n == 0)  return true;

    /*
        Algorithm :

        For all qp_cols in group by list, 
        1. if the qp_col is a single operand tree
            1.1. if the operand node is a variable
                1.1.1 if the operand node variable name matches with the alias name of qp_col in select list
                    1.1.1.1 if select's qp_col has agg_fn applied, abort the query
                    1.1.1.2 concatenate the groupby qp_col exp tree with that of select's qp_col exp tree
                    1.1.1.3 resolve all the operands of the concatednated tree against all tables in join list
                    1.1.1.4 if any operand unresolved, abort the query
                    1.1.1.5 validate the exp tree 
                    1.1.1.6 optimize the exp tree 
                    1.1.1.7 link select;s qp_col->link_to_groupby_col = group by 's qp_col

                1.1.2 if the operand node variable name matches with the variable name of a single operand node in select list whose alias_provided_by_user = false
                    1.1.2.1  if select's qp_col has agg_fn applied, abort the query
                    1.1.2.2 resolve group by operand node against a table ( default or composite ), if not resolved abort the query
                    1.1.2.3 link select;s qp_col->link_to_groupby_col = group by 's qp_col

            1.2 if the operand ndoe is a constant value
                1.2.1 Nothing else to do

        2. if the qp_col is a multi-node tree
            2.1 Resolve all operands of the exp tree against all tables in join list
            Unresolved operands left after above step are alias names
            2.2 for each unresolved operand in exp tree
                2.2.1 search in select list a qp_col with same alias name
                2.2.2 concatenate group by exp tree with select's qp_col
            2.3 Resolve the exp tree again against all tables, if any unresolved opnd, abort the query
            2.4 validate the exp tree, abort if fails
            2.5 optimize the exp tree 
    */

    for (i = 0; i < qep->groupby.n; i++) {

        gqp_col = qep->groupby.col_list[i];

        // 1.1  if the operand node is a variable
        if (sql_is_single_operand_expression_tree (gqp_col->sql_tree)) {
            
            sqp_col = sql_get_qp_col_by_alias_name (
                                qep->select.sel_colmns,
                                qep->select.n, 
                                (char *)sql_get_opnd_variable_name (
                                       sql_tree_get_root (gqp_col->sql_tree)).c_str()  );

            //1.1.1 if the operand node variable name matches with the alias name of qp_col in select list
            if (sqp_col) {

                //1.1.1.1 if select's qp_col has agg_fn applied, abort the query
                if (sqp_col->agg_fn != SQL_AGG_FN_NONE) {

                    printf ("Error : Aggregate fn cannot be applied on column %s\n", sqp_col->alias_name);
                    return false;
                }

                //1.1.1.2 concatenate the groupby qp_col exp tree with that of select's qp_col exp tree
                assert (sql_concatenate_expr_trees ( gqp_col->sql_tree, 
                                                                sql_tree_get_root (gqp_col->sql_tree),
                                                                sql_clone_expression_tree (sqp_col->sql_tree)
                                                                ));

                // 1.1.1.3 resolve all the operands of the concatednated tree against all tables in join list
                rc =  (sql_resolve_exptree (tcatalog, 
                                                             gqp_col->sql_tree, 
                                                             qep,
                                                             qep->joined_row_tmplate));

                // 1.1.1.4 if any operand unresolved, abort the query
                if (!rc) {

                    printf ("Error : Group by Column %s could not be resolved\n", sqp_col->alias_name);
                    return false;
                }

                //1.1.1.5 validate the exp tree 
                rc = sql_tree_validate (gqp_col->sql_tree);

                if (!rc) {

                    printf ("Error : Exp Tree of Group by Column %s could not be validated\n", 
                        sqp_col->alias_name);
                    return false;
                }

                //1.1.1.6 optimize the exp tree 
                rc = sql_tree_optimize (gqp_col->sql_tree);

                if (!rc) {

                    printf ("Error : Exp Tree of Group by Column %s could not be Optimized\n",
                         sqp_col->alias_name);
                    return false;
                }

                // 1.1.1.7 link select;s qp_col->link_to_groupby_col = group by 's qp_col
                sqp_col->link_to_groupby_col = gqp_col;
            }

            else {
                
                for (j = 0; j < qep->select.n; j++ ) {

                    sqp_col = qep->select.sel_colmns[j];
                    
                    // 1.1.2 if the operand node variable name matches with the variable name of a single operand
                    // node in select list whose alias_provided_by_user = false

                    if (!sql_is_single_operand_expression_tree  (sqp_col->sql_tree)) continue;
                    //if (sqp_col->alias_provided_by_user ) continue;
                    if (strncmp  ( 
                            sql_get_opnd_variable_name (sql_tree_get_root (sqp_col->sql_tree)).c_str(), 
                            sql_get_opnd_variable_name (sql_tree_get_root (gqp_col->sql_tree)).c_str(),
                            SQL_ALIAS_NAME_LEN ) ) {
                        continue;
                    }

                    // 1.1.2.1  if select's qp_col has agg_fn applied, abort the query
                    if (sqp_col->agg_fn != SQL_AGG_FN_NONE) {

                        printf ("Error : Aggregate fn cannot be applied on column %s\n",
                                    sql_get_opnd_variable_name (sql_tree_get_root (sqp_col->sql_tree)).c_str());
                        return false;                        
                    }

                    // 1.1.2.2 resolve group by operand node against a table ( default or composite ), if not 
                    //  resolved abort the query
                    parser_split_table_column_name (
                        (char *)sql_get_opnd_variable_name (sql_tree_get_root (sqp_col->sql_tree)).c_str(),
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

                        for (k = 0; k < qep->join.table_cnt; k++) {

                            if (ctable_val != qep->join.tables[k].ctable_val) continue;
                            tindex = k;
                            break;
                        }

                        if (tindex == -1) {

                            printf("Error : Table %s is not specified in Join list\n", table_name_out);
                            return false;
                        }
                    }

                    rc = sql_resolve_exptree_against_table (sqp_col->sql_tree, 
                                                                            ctable_val, 
                                                                            tindex,
                                                                            qep->joined_row_tmplate);
                    
                    if (!rc) {

                        printf ("Error : Select column %s could not be resolved against table %s\n",
                                    sql_get_opnd_variable_name (sql_tree_get_root (sqp_col->sql_tree)).c_str(),
                                    ctable_val->table_name);
                        return false;
                    }

                    //1.1.2.3 link select;s qp_col->link_to_groupby_col = group by 's qp_col
                    sqp_col->link_to_groupby_col = gqp_col;
                    break;
                }

                if (j ==  qep->select.n) {
                    printf ("Error : group by column name %s is unrecognized\n", 
                         sql_get_opnd_variable_name (sql_tree_get_root (gqp_col->sql_tree)).c_str());
                    return false;
                }

            }

            return true;
        }

        // Implement 2

        //2.1 
        rc =  (sql_resolve_exptree (tcatalog, 
                                                    gqp_col->sql_tree, 
                                                    qep,
                                                    qep->joined_row_tmplate));

        //2.2

        while (!all_alias_resolved) {

            all_alias_resolved = true;

            SqlExprTree_Iterator_Operands_Begin(gqp_col->sql_tree, exptree_node) {

                if (sql_opnd_node_is_resolved(exptree_node)) continue;

                // 2.2.1 search in select list a qp_col with same alias name
                sqp_col = sql_get_qp_col_by_alias_name(
                    qep->select.sel_colmns,
                    qep->select.n,
                    (char *)sql_get_opnd_variable_name(exptree_node).c_str());

                if (!sqp_col) {

                    printf("Error : Alias name %s could not be resolved\n",
                           (char *)sql_get_opnd_variable_name(exptree_node).c_str());
                    return false;
                }

                all_alias_resolved = false;
                
                //2.2.2 concatenate group by exp tree with select's qp_col
                rc = (sql_concatenate_expr_trees(gqp_col->sql_tree,
                                                 exptree_node,
                                                 sql_clone_expression_tree(sqp_col->sql_tree)));

                if (!rc) {

                    printf("Error : Failed to concatenate exp tree of %s select column against group by clause\n",
                           sqp_col->alias_name);
                    return false;
                }

                if (!all_alias_resolved ) break;
            }
        }

        //2.3 Resolve the exp tree again against all tables, if any unresolved opnd, abort the query
        rc =  (sql_resolve_exptree (tcatalog, 
                                                             gqp_col->sql_tree, 
                                                             qep,
                                                             qep->joined_row_tmplate));                

        if (!rc) {

            printf ("Error : Could not resolve Unnamed %dth group by Expression Tree\n", i);
            return false;
        }

        // 2.4 validate the exp tree, abort if fails
        rc = sql_tree_validate (gqp_col->sql_tree);

        if (!rc) {

            printf ("Error : Could not validate Unnamed %dth group by Expression Tree\n", i); 
            return false;
        }

        //2.5 optimize the exp tree 
        rc = sql_tree_optimize (gqp_col->sql_tree);

        if (!rc) {

            printf ("Error : Could not optimize Unnamed %dth group by Expression Tree\n", i); 
            return false;
        }
    }

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

    int opnd_len;
    qp_col_t *qp_col;
    MexprNode *opnd_node;
    sql_exptree_t *clone_tree;

    bool alias_resolved = false;
    bool all_alias_resolved = false;

    while (!all_alias_resolved) {

        all_alias_resolved = true;
        alias_resolved = false;

        SqlExprTree_Iterator_Operands_Begin (qep->where.gexptree, opnd_node) {

            if (sql_opnd_node_is_resolved (opnd_node)) continue;

            /* All Unresolved nodes are opernads of type 'variable'*/
            opnd_len = sql_get_opnd_variable_name(opnd_node).length();

            for (i = 0; i < qep->select.n; i++) {

                qp_col = qep->select.sel_colmns[i];
                if (!qp_col->alias_provided_by_user) continue;

                if ((strlen(qp_col->alias_name) != opnd_len) ||
                    (strncmp(qp_col->alias_name,
                             sql_get_opnd_variable_name(opnd_node).c_str(),
                             SQL_ALIAS_NAME_LEN)))
                    continue;

                all_alias_resolved = false;
                clone_tree = sql_clone_expression_tree(qp_col->sql_tree);
                assert(clone_tree);

                if (!sql_concatenate_expr_trees(qep->where.gexptree,
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
        }
    }


    for (i = 0; i < qep->join.table_cnt; i++) {

        qep->where.exptree_per_table[i] = sql_clone_expression_tree(qep->where.gexptree);

        if (!qep->where.exptree_per_table[i]) {

            printf("Error : Failed to create Exp Tree Clones of Where Clause\n");
            return false;
        }

        if (!sql_resolve_exptree_against_table(
                        qep->where.exptree_per_table[i],
                        qep->join.tables[i].ctable_val, i, 
                        qep->joined_row_tmplate)) {

            printf("Error : Failed to resolve per table Where Expression Tree\n");
            return false;
        }
    }


    if (!sql_resolve_exptree(&TableCatalogDef,
                             qep->where.gexptree,
                             qep, qep->joined_row_tmplate)) {

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
            /* Will allocate at the time of computation in select query*/
            qp_col->computed_value = NULL;

            if (qep->join.table_cnt > 1) {
                memset (opnd_name, 0, sizeof (opnd_name));
                snprintf (opnd_name, sizeof(opnd_name), "%s.%s", 
                    ctable_val->table_name, (char *)lnode->data);
                qp_col->sql_tree = sql_create_exp_tree_for_one_operand (opnd_name);
                strncpy (qp_col->alias_name, opnd_name, sizeof (qp_col->alias_name));
            }
            else {
                qp_col->sql_tree = sql_create_exp_tree_for_one_operand ((char *)lnode->data);
                strncpy (qp_col->alias_name, (char *)lnode->data, sizeof (qp_col->alias_name));
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
                if (sql_is_single_operand_expression_tree (qp_col->sql_tree)
                        && qp_col->alias_name[0] == '\0') {

                     strncpy(qp_col->alias_name,  
                        sql_get_opnd_variable_name(sql_tree_get_root (qp_col->sql_tree)).c_str(), 
                        sizeof (qp_col->alias_name));
                }
            }
    }
    /* Now Resolve Expressression Trees for all select columns*/
    for (i = 0; i < qep->select.n; i++) {

        if (!sql_resolve_exptree (&TableCatalogDef, 
                                                qep->select.sel_colmns[i]->sql_tree,
                                                qep, qep->joined_row_tmplate)) {
            
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

    qep->joined_row_tmplate = (joined_row_t *)calloc (1, sizeof (joined_row_t));

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
    joined_row_t *joined_row_tmplate = qep->joined_row_tmplate;
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

            hashtable_destroy(qep->groupby.ht, 1);
            qep->groupby.ht = NULL;
        }
    }

    if (qep->having.gexptree) {
        sql_destroy_exp_tree (qep->having.gexptree);
        qep->having.gexptree = NULL;
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

    if (qep->joined_row_tmplate) {
        free (qep->joined_row_tmplate->rec_array);
        free(qep->joined_row_tmplate->schema_table_array);
        free(qep->joined_row_tmplate->table_id_array);
        free(qep->joined_row_tmplate);
    }
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

    if (table_id < 0) return ;

    do
    {
        rec = BPlusTree_get_next_record(
                    titer->table_iter_data[table_id].ctable_val->rdbms_table,
                    &titer->table_iter_data[table_id].bpnode,
                    &titer->table_iter_data[table_id].index);

        if (!rec) break;

        qep_struct->joined_row_tmplate->rec_array[table_id] = rec;

        rc = sql_evaluate_conditional_exp_tree (
                    qep_struct->where.exptree_per_table[table_id]);

    } while (!rc && titer->table_iter_data[table_id].bpnode);

    /* If record is found*/
    if (rec) {
            return;
    }
    else {
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
                            &titer->table_iter_data[table_id].index);

            if (!rec) break;

            qep_struct->joined_row_tmplate->rec_array[table_id] = rec;

            rc = sql_evaluate_conditional_exp_tree (
                    qep_struct->where.exptree_per_table[table_id]);

        } while (!(rc) && titer->table_iter_data[table_id].bpnode);

        qep_struct->joined_row_tmplate->rec_array[table_id] = rec;
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
    Dtype *computed_value;
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

            sql_setup_group_by_hashtable (qep);
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
                    qp_col->aggregator = sql_get_aggregator (qp_col);
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

            if (row_no == 1) {
                sql_print_hdr  (qep->select.sel_colmns, qep->select.n);
            }

            sql_emit_select_output (qep->select.n, qep->select.sel_colmns);

            if (qep->limit == row_no) {
                break;
            }
        }
    }  /* While ends */

    /* Case 2 :  No Group by Clause,  Aggregated Columns */
    if (!qep->groupby.n && is_aggregation) {

        sql_print_hdr(qep->select.sel_colmns, qep->select.n);
        sql_emit_select_output(qep->select.n, qep->select.sel_colmns);
        printf ("(1 rows)\n");
    }
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
