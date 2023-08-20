#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <memory.h>
#include "../BPlusTreeLib/BPlusTree.h"
#include "qplanner.h"
#include "Catalog.h"
#include "../Parsers/Ast.h"
#include "../gluethread/glthread.h"
#include "sql_io.h"
#include "sql_utils.h"
#include "sql_where.h"
#include "../c-hashtable/hashtable.h"
#include "../c-hashtable/hashtable_itr.h"


#define NOT_SUPPORT_YET assert(0)
extern BPlusTree_t TableCatalogDef;

static void *
 qep_enforce_where (BPlusTree_t  *schema_table, void *record, expt_node_t *expt_root) {

    if (!record) return NULL;
    if (!expt_root) return record;
    
     joined_row_t  joined_row;

     memset (&joined_row, 0, sizeof (joined_row));
     joined_row.schema_table[0] = schema_table;
     joined_row.rec[0] = record;

    bool rc = sql_evaluate_where_expression_tree (expt_root, &joined_row);
    
    if (rc) return record;
    return NULL;
 }


static void 
table_iterators_init (table_iterators_t *titer, 
                                ctable_val_t *ctable_val1,
                                ctable_val_t *ctable_val2,
                                ctable_val_t *ctable_val3) {

    titer->ctable_val[0] = ctable_val1;
    titer->ctable_val[1] = ctable_val2;
    titer->ctable_val[2] = ctable_val3;
}

static void 
qep_init_where_clause (qep_struct_t *qep_struct, ast_node_t *root) {

    int size_out = 0;
    ast_node_t ast_tmplate;

    ast_tmplate.entity_type = SQL_WHERE_CLAUSE;
    ast_tmplate.u.kw = SQL_PTR;

    ast_node_t *where_clause_node = ast_find (root, &ast_tmplate);

    if (!where_clause_node) return;

    where_literal_t *where_literals_arr = NULL;

    memcpy ((void *)&where_literals_arr,
                    where_clause_node->u.identifier.identifier.name,
                    sizeof (void *));
    assert (where_literals_arr);

    where_literal_t **where_literals_postfix = 
        sql_where_clause_infix_to_postfix (where_literals_arr, &size_out);

    if (!where_literals_postfix) return;
#if 0
    printf ("postfix : \n");
    sql_debug_print_where_literals2 (where_literals_postfix, size_out);
#endif
    qep_struct->expt_root = 
        sql_where_convert_postfix_to_expression_tree (where_literals_postfix , size_out);
#if 0
    printf ("Expression Tree :\n");
    sql_debug_print_expression_tree (qep_struct->expt_root);
#endif
    free (where_literals_postfix);
}

static void
table_iterators_first (qep_struct_t *qep_struct, table_iterators_t *titer, void **rec1, void **rec2, void **rec3) {
    titer->bpnode[0] = NULL;
    titer->index[0] = 0;
    titer->bpnode[1] = NULL;
    titer->index[1] = 0;
    titer->bpnode[2] = NULL;
    titer->index[2] = 0;

    do {
        *rec1 = BPlusTree_get_next_record (titer->ctable_val[0]->rdbms_table, &titer->bpnode[0], &titer->index[0]);
        *rec1 =  qep_enforce_where (qep_struct->ctable_val1->schema_table, *rec1, qep_struct->expt_root);
    } while (!(*rec1) && titer->bpnode[0]);

    if (!titer->bpnode[0]) {
        *rec1 = NULL;
        *rec2 = NULL;
        *rec3 = NULL;
        return;
    }

    if (!titer->ctable_val[1]) return;

    do {
        *rec2 = BPlusTree_get_next_record (titer->ctable_val[1]->rdbms_table, &titer->bpnode[1], &titer->index[1]);
        *rec2 =  qep_enforce_where (qep_struct->ctable_val2->schema_table, *rec2, qep_struct->expt_root);
    } while (!(*rec2) && titer->bpnode[1]);

    if (!titer->bpnode[1]) {
        *rec2 = NULL;
        *rec3 = NULL;
        return;
    }


    if (!titer->ctable_val[2]) return;

    do {
        *rec3 = BPlusTree_get_next_record (titer->ctable_val[2]->rdbms_table, &titer->bpnode[2], &titer->index[2]);
        *rec3 =  qep_enforce_where (qep_struct->ctable_val3->schema_table, *rec3, qep_struct->expt_root);
    } while (!(*rec3) && titer->bpnode[2]);

    if (!titer->bpnode[2]) {
        *rec3 = NULL;
        return;
    }

}

static bool
table_iterators_next (qep_struct_t *qep_struct, table_iterators_t *titer, void **rec1, void **rec2, void **rec3) {

    /* If [2] table is present*/
    if (titer->ctable_val[2]) {

        /* if [2] table is still iterating, get next record from [2]*/
        if (titer->bpnode[2]) {
            do {
                *rec3 = BPlusTree_get_next_record(titer->ctable_val[2]->rdbms_table, &titer->bpnode[2], &titer->index[2]);
                *rec3 =  qep_enforce_where (qep_struct->ctable_val3->schema_table, *rec3, qep_struct->expt_root);
            } while (!(*rec3) && titer->bpnode[2]);

            if (!titer->bpnode[2])
            {
            *rec3 = NULL;
            return false;
            }
            return true;
        }
        else {
            /* Iteration of [2] table is finished, increment iteraator of [1] table*/

                if (titer->bpnode[1]) {

                    do {
                        *rec2 = BPlusTree_get_next_record (titer->ctable_val[1]->rdbms_table, &titer->bpnode[1], &titer->index[1]);
                        *rec2 =  qep_enforce_where (qep_struct->ctable_val2->schema_table, *rec2, qep_struct->expt_root);
                    } while   (!(*rec2) && titer->bpnode[1]);

                    if (!titer->bpnode[1]) {
                        *rec2 = NULL;
                        *rec3 = NULL;
                        return false;
                    }

                    do {
                        *rec3 = BPlusTree_get_next_record (titer->ctable_val[2]->rdbms_table, &titer->bpnode[2], &titer->index[2]);
                        *rec3 =  qep_enforce_where (qep_struct->ctable_val3->schema_table, *rec3, qep_struct->expt_root);
                    }  while (!(*rec3) && titer->bpnode[2]);

                    if (!titer->bpnode[2]) {
                        *rec3 = false;
                        return false;
                    }
                    return true;
                }

                else {

                    /*  Iteration of [1] table is finished, increment iterator of [0] table */
                    if (titer->bpnode[0]) {
                        do {
                            *rec1 = BPlusTree_get_next_record (titer->ctable_val[0]->rdbms_table, &titer->bpnode[0], &titer->index[0]);
                            *rec1 =  qep_enforce_where (qep_struct->ctable_val1->schema_table, *rec1, qep_struct->expt_root);
                        }  while (!(*rec1) && titer->bpnode[0]);

                        if (!titer->bpnode[0]) {
                            *rec1 = NULL;
                            *rec2 = NULL;
                            *rec3 = NULL;
                            return false;
                        }

                        do {
                            *rec2 = BPlusTree_get_next_record (titer->ctable_val[1]->rdbms_table, &titer->bpnode[1], &titer->index[1]);
                            *rec2 =  qep_enforce_where (qep_struct->ctable_val2->schema_table, *rec2, qep_struct->expt_root);
                        }while   (!(*rec2) && titer->bpnode[1]);

                        if (!titer->bpnode[1]) {
                            *rec2 = NULL;
                            *rec3 = NULL;
                            return false;
                        }

                        do {
                            *rec3 = BPlusTree_get_next_record (titer->ctable_val[2]->rdbms_table, &titer->bpnode[2], &titer->index[2]);
                            *rec3 =  qep_enforce_where (qep_struct->ctable_val3->schema_table, *rec3, qep_struct->expt_root);
                        }  while (!(*rec3) && titer->bpnode[2]);

                        if (!titer->bpnode[2]) {
                            *rec3 = false;
                            return false;
                        }

                        return true;
                    }
                    else {
                        /* Iteration of [0] table is also finished*/
                        *rec1 = NULL;
                        *rec2 = NULL;
                        *rec3 = NULL;
                        return false;
                    }
                }
        }
        return true;
    }

    /* If table [2] is not present*/
    if (titer->ctable_val[1]) {

        if (titer->bpnode[1]) {
            
            do {
                *rec2 = BPlusTree_get_next_record (titer->ctable_val[1]->rdbms_table, &titer->bpnode[1], &titer->index[1]);            
                *rec2 =  qep_enforce_where (qep_struct->ctable_val2->schema_table, *rec2, qep_struct->expt_root);
            } while   (!(*rec2) && titer->bpnode[1]);

            if (!titer->bpnode[1]) {
                *rec2 = NULL;
                *rec3 = NULL;
                return false;
            }            

            return true;
        }
        else {

            if (titer->bpnode[0]) {

                do {
                    *rec1 = BPlusTree_get_next_record (titer->ctable_val[0]->rdbms_table, &titer->bpnode[0], &titer->index[0]);
                    *rec1 =  qep_enforce_where (qep_struct->ctable_val1->schema_table, *rec1, qep_struct->expt_root);
                }  while (!(*rec1) && titer->bpnode[0]);

                if (!titer->bpnode[0]) {
                    *rec1 = NULL;
                    *rec2 = NULL;
                    *rec3 = NULL;
                    return false;
                }

                do {
                    *rec2 = BPlusTree_get_next_record (titer->ctable_val[1]->rdbms_table, &titer->bpnode[1], &titer->index[1]);
                    *rec2 =  qep_enforce_where (qep_struct->ctable_val2->schema_table, *rec2, qep_struct->expt_root);                    
                } while   (!(*rec2) && titer->bpnode[1]);

                if (!titer->bpnode[1]) {
                    *rec2 = NULL;
                    *rec3 = NULL;
                    return false;
                }

                return true;
            }
            else {
                *rec1 = NULL;
                *rec2 = NULL;
                return false;
            }
        }
        return true;
    }

    /* If table [1] is not present*/
     if (titer->bpnode[0]) {
        do {
            *rec1 = BPlusTree_get_next_record (titer->ctable_val[0]->rdbms_table, &titer->bpnode[0], &titer->index[0]);
            *rec1 =  qep_enforce_where (qep_struct->ctable_val1->schema_table, *rec1, qep_struct->expt_root);
        } while (!(*rec1) && titer->bpnode[0]);

        if (!titer->bpnode[0]) {
            *rec1 = NULL;
            *rec2 = NULL;
            *rec3 = NULL;
            return false;
        }
        return true;
     }
     else {
        *rec1 = NULL;
        return false;
     }

    return true;
}




static bool
qep_execute_join (qep_struct_t *qep_struct, void **rec1, void **rec2, void **rec3) {

   if (!qep_struct->is_join_started) {

        table_iterators_first (qep_struct, &qep_struct->titer, rec1, rec2, rec3);
        qep_struct->is_join_started = true;
        return (*rec1 || *rec2 || *rec3);
   }

   return table_iterators_next (qep_struct, &qep_struct->titer, rec1, rec2, rec3);
}

static joined_row_t *
qep_execute_join_predicate (qep_struct_t *qep_struct, void *rec1, void *rec2, void *rec3) {

    joined_row_t *joined_row;

    if (!rec1 && !rec2 && !rec3) return NULL;


    /* Here apply the join predicate, let us assume the join predicate is always true*/


    joined_row = (joined_row_t *)calloc (1, sizeof (joined_row_t));

    if (rec1) {
        joined_row->schema_table[0] = qep_struct->ctable_val1->schema_table;
        joined_row->rec[0] = rec1;
    }
    if (rec2) {
        joined_row->schema_table[1] = qep_struct->ctable_val2->schema_table;
        joined_row->rec[1] = rec2;
    }
    if (rec3) {
        joined_row->schema_table[2] = qep_struct->ctable_val3->schema_table;
        joined_row->rec[2] = rec3;
    }
    return joined_row;
}

/* HashTable Setup */
#define HASH_PRIME_CONST    5381

static unsigned int 
hashfromkey(void *key) {

    unsigned char *str = (unsigned char *)key;
    unsigned int hash = HASH_PRIME_CONST;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static int 
equalkeys(void *k1, void *k2)
{
    char *ky1 = (char *)k1;
    char *ky2 = (char *)k2;
    int len1 = strlen(ky1);
    int len2 = strlen(ky2);
    if (len1 != len2) return len1 - len2;
    return (0 == memcmp(k1,k2, len1));
}

static int
sql_compute_group_by_key_size (qep_struct_t *qep_struct) {

    int i;
    int size = 0;
    qp_col_t *col;

    if (qep_struct->groupby.n == 0) return 0;

    for (i = 0; i < qep_struct->groupby.n ; i++) {

        col = qep_struct->groupby.col_list[i];
        size += col->schema_rec->dtype_size;
    }
    return size;
}

static void *
sql_compute_group_by_clause_keys (qep_struct_t *qep_struct,  joined_row_t  *joined_row) {

    return NULL;
}

void
qep_execute (qep_struct_t *qep_struct) {

    int row_no = 0;
    qp_col_t *col;
    bool is_aggregation = false;
    joined_row_t *joined_row = NULL;

    void *rec1 = NULL, *rec2 = NULL, *rec3 = NULL;

    while (qep_execute_join (qep_struct, &rec1, &rec2, &rec3)) {

        if (!(joined_row =  qep_execute_join_predicate(qep_struct, rec1, rec2, rec3))) {
            continue;
        }

        row_no++;
        /* Join predicate has been qualified*/

        /* Time for Grouping */

        /* Check if the query has group by clause */
        if (qep_struct->groupby.n) {

            void *ht_key = sql_compute_group_by_clause_keys (qep_struct, joined_row);
            list_node_t *lnode_head = (list_node_t *)hashtable_search (qep_struct->ht , ht_key);
            if (lnode_head) {
                list_node_t *new_lnode = (list_node_t *)calloc (1, sizeof (list_node_t ));
                new_lnode->data = (void *)joined_row;
                init_glthread (&new_lnode->glue);
                glthread_add_last (&lnode_head->glue, &new_lnode->glue);
                free(ht_key);
            }
            else {
                lnode_head =  (list_node_t *)calloc (1, sizeof (list_node_t ));
                init_glthread (&lnode_head->glue);
                lnode_head->data = NULL;
                list_node_t *new_lnode = (list_node_t *)calloc (1, sizeof (list_node_t ));
                new_lnode->data = (void *)joined_row;
                init_glthread (&new_lnode->glue);
                glthread_add_last (&lnode_head->glue, &new_lnode->glue);
                assert((!hashtable_insert (qep_struct->ht, ht_key, (void *)lnode_head)));
            }
            /* Collect all rows in HASTABLE buckets in case there is a groupby clause, if there is
             no group by clause, no need to accumulate rows in this case in hashtable.
             In SQL, collect rows only when it is necessary for performance*/
            continue;
        }

        /* We are here if there is no groupby clause*/
        /* Time for select */
        int i;
        void *val;

        for (i = 0; i < qep_struct->select.n; i++)
        {

            col = qep_struct->select.sel_colmns[i];

            if (!col->computed_value)
            {
                col->computed_value = calloc(col->schema_rec->dtype_size, 1);
            }
            if (col->agg_fn == SQL_AGG_FN_NONE)
            {
                val = sql_get_column_value_from_joined_row(joined_row, col);
                memcpy(col->computed_value, val, col->schema_rec->dtype_size);
            }
            else
            {
                is_aggregation = true;
               
                if (row_no == 1)
                {
                    val = sql_get_column_value_from_joined_row(joined_row, col);
                    if (col->agg_fn == SQL_COUNT) {
                        *(int *)(col->computed_value) = 1;
                    }
                    else {
                        memcpy(col->computed_value, val, col->schema_rec->dtype_size);
                    }
                }
                else
                {
                    val = sql_get_column_value_from_joined_row(joined_row, col);
                    sql_compute_aggregate(col->agg_fn, val, col->computed_value, col->schema_rec->dtype, col->schema_rec->dtype_size, row_no);
                }
            }
        }

        free(joined_row);

        /* Output */
        /* Case 1 :  No Group by Clause, Non-Aggregated Columns */
        if (!qep_struct->groupby.n && !is_aggregation) {

            if (row_no == 1) {
                sql_print_hdr  (qep_struct->select.sel_colmns, qep_struct->select.n);
            }

            sql_emit_select_output (qep_struct->select.n, qep_struct->select.sel_colmns);

            if (qep_struct->limit == row_no) {
                break;
            }
        }

    } // join loop ends

    /* Case 1 :  No Group by Clause, Non-Aggregated Columns */
    if (!qep_struct->groupby.n && !is_aggregation) {

        printf ("(%d rows)\n", row_no);
    }


     /* Case 2 :  No Group by Clause,  Aggregated Columns */
    else if (!qep_struct->groupby.n && is_aggregation) {

        sql_print_hdr  (qep_struct->select.sel_colmns, qep_struct->select.n);
        sql_emit_select_output (qep_struct->select.n, qep_struct->select.sel_colmns);
        printf ("(1 rows)\n");
    }

    /* case 3 :  Group by Clause */
    else if (qep_struct->groupby.n) {

    }
}
   
void 
qep_struct_init (qep_struct_t *qep_struct, BPlusTree_t *tcatalog, ast_node_t *root) {

    int n_cols = 0;
    BPluskey_t bpkey;
    ast_node_t ast_tmplate;

    memset (&ast_tmplate, 0, sizeof (ast_tmplate));
    ast_tmplate.entity_type = SQL_IDENTIFIER;
    ast_tmplate.u.identifier.ident_type = SQL_TABLE_NAME;
    ast_node_t *table_name_node = ast_find (root, &ast_tmplate);
    bpkey.key = table_name_node->u.identifier.identifier.name;
    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;
    ctable_val_t *ctable_val = (ctable_val_t *) BPlusTree_Query_Key (tcatalog, &bpkey);
    assert (ctable_val);
    qep_struct->ctable_val1 = ctable_val;
    
    ast_node_t *column_node;

    FOR_ALL_AST_CHILD (table_name_node, column_node) {
        n_cols++;
    } FOR_ALL_AST_CHILD_END;

    qep_struct->select.n = n_cols;
    qep_struct->select.sel_colmns = (qp_col_t **)calloc (n_cols, sizeof (qp_col_t *));
    int i = 0;
    ast_node_t *agg_node;
    FOR_ALL_AST_CHILD (table_name_node, column_node) {
        qep_struct->select.sel_colmns[i] = (qp_col_t *)calloc (1, sizeof (qp_col_t));
        qep_struct->select.sel_colmns[i]->ctable_val = ctable_val;
        bpkey.key = column_node->u.identifier.identifier.name;
        bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
        qep_struct->select.sel_colmns[i]->schema_rec = 
            (schema_rec_t *) BPlusTree_Query_Key (ctable_val->schema_table, &bpkey);
        qep_struct->select.sel_colmns[i]->agg_fn = SQL_AGG_FN_NONE;
        FOR_ALL_AST_CHILD (column_node, agg_node) {
            if (agg_node->entity_type != SQL_AGG_FN) continue;
            qep_struct->select.sel_colmns[i]->agg_fn = agg_node->u.agg_fn;
            break;
        } FOR_ALL_AST_CHILD_END;
        i++;
    } FOR_ALL_AST_CHILD_END;

    qep_struct->is_join_started = false;
    table_iterators_init (&qep_struct->titer,
                                    qep_struct->ctable_val1,
                                    qep_struct->ctable_val2,
                                    qep_struct->ctable_val3);
    qep_struct->ht = create_hashtable
                            (sql_compute_group_by_key_size (qep_struct), 
                            hashfromkey, equalkeys);    
    qep_init_where_clause (qep_struct, root);
}

void 
qep_deinit (qep_struct_t *qep_struct) {

    int i;
    qp_col_t *col;

    if (qep_struct->expt_root) {
        expt_destroy (qep_struct->expt_root);
    }
    
    if (qep_struct->groupby.n) {

        for (i = 0; i < qep_struct->groupby.n; i++) {
            col =  qep_struct->groupby.col_list[i];
            free(col);
        }
    }

    if (qep_struct->select.n) {

        for (i = 0; i < qep_struct->select.n; i++) {
            col =  qep_struct->select.sel_colmns[i];
            if (col->computed_value) {
                free(col->computed_value);
            }
            free(col);
        }
    }

    if (hashtable_count (qep_struct->ht)) {
        hashtable_destroy (qep_struct->ht, 1); 
        qep_struct->ht = NULL;
    }

}