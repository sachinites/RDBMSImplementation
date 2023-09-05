#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <memory.h>
#include <arpa/inet.h>
#include "../stack/stack.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "qplanner.h"
#include "Catalog.h"
#include "../Parsers/Ast.h"
#include "../gluethread/glthread.h"
#include "sql_io.h"
#include "sql_utils.h"
#include "sql_where.h"
#include "sql_groupby.h"
#include "../c-hashtable/hashtable.h"
#include "../c-hashtable/hashtable_itr.h"


#define NOT_SUPPORT_YET assert(0)
extern BPlusTree_t TableCatalogDef;


static void *
 qep_enforce_where (qep_struct_t *qep_struct, BPlusTree_t  *schema_table, void *record, expt_node_t *expt_root, int table_id) {

    if (!record) return NULL;
    if (!expt_root) return record;
    
    joined_row_t  joined_row;
    memset (&joined_row, 0, sizeof (joined_row));

    BPlusTree_t *schema_table_array[1];
    void *rec_array[1];
    int table_id_array[1];

     joined_row.schema_table_array= schema_table_array;
     joined_row.rec_array = rec_array;
     joined_row.table_id_array = table_id_array;
     joined_row.size = 1;
     schema_table_array[0] = schema_table;
     rec_array[0] = record;
     table_id_array[0] = table_id;

    bool rc = sql_evaluate_where_expression_tree (qep_struct, expt_root, &joined_row);
    
    if (rc) return record;
    return NULL;
 }

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
        titer->table_iter_data[i].ctable_val  = qep->ctable_val[i];
    }
}

static void
table_iterators_first (qep_struct_t *qep_struct, 
                                 table_iterators_t *titer, 
                                 void **rec_array,
                                 int table_id){

        void *rec = NULL;

        if (table_id < 0) return;

        do
        {
            rec = BPlusTree_get_next_record(
                        titer->table_iter_data[table_id].ctable_val->rdbms_table,
                        &titer->table_iter_data[table_id].bpnode,
                        &titer->table_iter_data[table_id].index);

            rec = qep_enforce_where(qep_struct,
                        titer->table_iter_data[table_id].ctable_val->schema_table, 
                        rec, qep_struct->expt_root, table_id);

        } while (!(rec) && titer->table_iter_data[table_id].bpnode);

        rec_array[table_id] = rec;

        /* No need to scan further*/
        if (!rec) {
            qep_struct->is_join_finished = true;
            return;
        }

        table_iterators_first (qep_struct, titer, rec_array, table_id -1);
}

static void
table_iterators_next (qep_struct_t *qep_struct, 
                                  table_iterators_t *titer, 
                                  void **rec_array,
                                  int table_id) {

    void *rec = NULL;

    if (table_id < 0) return ;

    do
    {
        rec = BPlusTree_get_next_record(
                    titer->table_iter_data[table_id].ctable_val->rdbms_table,
                    &titer->table_iter_data[table_id].bpnode,
                    &titer->table_iter_data[table_id].index);

        rec = qep_enforce_where(qep_struct,
                    titer->table_iter_data[table_id].ctable_val->schema_table,
                    rec, qep_struct->expt_root, table_id);

    } while (!(rec) && titer->table_iter_data[table_id].bpnode);

    /* If record is found*/
    if (rec) {
            rec_array[table_id] = rec;
            return;
    }
    else {
        rec_array[table_id] = NULL;
        table_iterators_next(qep_struct, titer, rec_array, table_id - 1);

        /* If We could not find qualified record in the top level table, abort the iteration */
        if (table_id == 0) {
            qep_struct->is_join_finished = true;
            return;
        }

        /* If secondary table finds that parent table could not find any qualified records, abort the iteration*/
        if (!rec_array[table_id - 1]) return;

        do {
            rec = BPlusTree_get_next_record(
                            titer->table_iter_data[table_id].ctable_val->rdbms_table,
                            &titer->table_iter_data[table_id].bpnode,
                            &titer->table_iter_data[table_id].index);

            rec = qep_enforce_where(qep_struct,
                            titer->table_iter_data[table_id].ctable_val->schema_table,
                            rec, qep_struct->expt_root, table_id);

        } while (!(rec) && titer->table_iter_data[table_id].bpnode);

        assert(rec);
        rec_array[table_id] = rec;
    }
}

static bool
qep_execute_join (qep_struct_t *qep_struct, void **rec_array) {

   if (!qep_struct->is_join_started) {

        table_iterators_first (qep_struct, qep_struct->titer, rec_array, qep_struct->join.table_cnt -1);

        qep_struct->is_join_started = true;

        /* We could not get Ist Qualified record from each joined tables*/
        if (qep_struct->is_join_finished) return false;
        return true;
   }

    table_iterators_next (qep_struct, qep_struct->titer, rec_array, qep_struct->join.table_cnt -1);
    return !qep_struct->is_join_finished;
}


static bool
qep_execute_join_predicate (qep_struct_t *qep_struct, joined_row_t *joined_row) {

   if (!qep_struct->expt_root) return true;
   return sql_evaluate_where_expression_tree (qep_struct, qep_struct->expt_root, joined_row);
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

void
qep_execute_select (qep_struct_t *qep_struct) {

    int i;
    int row_no = 0;
    qp_col_t *col;
    bool is_aggregation = false;

    joined_row_t joined_row;

    joined_row.size = qep_struct->join.table_cnt;
    joined_row.rec_array = (void **) calloc (qep_struct->join.table_cnt, sizeof (void *));
    joined_row.schema_table_array = (BPlusTree_t **)
        calloc (qep_struct->join.table_cnt, sizeof (BPlusTree_t *));
    joined_row.table_id_array = (int *)calloc (1, sizeof (int));

    for (i = 0; i < qep_struct->join.table_cnt; i++) {
        joined_row.schema_table_array[i] = 
            qep_struct->titer->table_iter_data[i].ctable_val->schema_table;
        joined_row.table_id_array[i] = i;
    }

    while (qep_execute_join (qep_struct,  joined_row.rec_array)) {

        /* Optimization : If only one table is involved, no need to evaluate join-predicate*/
        if (qep_struct->join.table_cnt > 1 &&
                !qep_execute_join_predicate(qep_struct, &joined_row)) {
            continue;
        }

        row_no++;
        /* Join predicate has been qualified*/

        /* Time for Grouping */

        /* Check if the query has group by clause */
        if (qep_struct->groupby.n) {

            void *ht_key = sql_compute_group_by_clause_keys (qep_struct, &joined_row);
            list_node_t *lnode_head = (list_node_t *)hashtable_search (qep_struct->ht , ht_key);

            if (lnode_head) {
                list_node_t *new_lnode = (list_node_t *)calloc (1, sizeof (list_node_t ));
                new_lnode->data = (void *)joined_row.rec_array;
                joined_row.rec_array = (void **) calloc (qep_struct->join.table_cnt, sizeof (void *));
                init_glthread (&new_lnode->glue);
                glthread_add_last (&lnode_head->glue, &new_lnode->glue); /* To be Optimized to O(1)*/
                free(ht_key);
            }
            else {
                lnode_head =  (list_node_t *)calloc (1, sizeof (list_node_t ));
                init_glthread (&lnode_head->glue);
                lnode_head->data = NULL;
                list_node_t *new_lnode = (list_node_t *)calloc (1, sizeof (list_node_t ));
                new_lnode->data = (void *)joined_row.rec_array;
                joined_row.rec_array = (void **) calloc (qep_struct->join.table_cnt, sizeof (void *));
                init_glthread (&new_lnode->glue);
                glthread_add_last (&lnode_head->glue, &new_lnode->glue); /* To be Optimized to O(1)*/
                assert((hashtable_insert (qep_struct->ht, ht_key, (void *)lnode_head)));
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
                val = sql_get_column_value_from_joined_row(&joined_row, col);
                memcpy(col->computed_value, val, col->schema_rec->dtype_size);
            }
            else
            {
                is_aggregation = true;
               
                if (row_no == 1)
                {
                    val = sql_get_column_value_from_joined_row(&joined_row, col);
                    if (col->agg_fn == SQL_COUNT) {
                        *(int *)(col->computed_value) = 1;
                    }
                    else {
                        memcpy(col->computed_value, val, col->schema_rec->dtype_size);
                    }
                }
                else
                {
                    val = sql_get_column_value_from_joined_row(&joined_row, col);
                    sql_compute_aggregate(col->agg_fn, val,
                                                            col->computed_value, 
                                                            col->schema_rec->dtype,
                                                            col->schema_rec->dtype_size,
                                                            row_no);
                }
            }
        }

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

    free (joined_row.rec_array);
    free (joined_row.schema_table_array);
    free(joined_row.table_id_array);

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

        sql_process_group_by (qep_struct);
    }
}

static  qp_col_t *
qep_init_column (qep_struct_t *qep_struct, 
                             BPlusTree_t *tcatalog,
                              ast_node_t *from_kw,
                              ast_node_t *column_node) {
       
        int i = 0;
        int table_id;
        BPluskey_t bpkey;
        ast_node_t *agg_node;
        ast_node_t ast_tmplate;
        ctable_val_t *ctable_val;
        unsigned char table_name_out [SQL_TABLE_NAME_MAX_SIZE];
        unsigned char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE];
        unsigned char *table_name_ptr = table_name_out;

        qp_col_t *new_col =  (qp_col_t *)calloc (1, sizeof (qp_col_t));
        parser_split_table_column_name (column_node->u.identifier.identifier.name, table_name_out, lone_col_name);
        table_name_ptr = (table_name_out[0] == '\0') ? 
                                        from_kw->child_list->u.identifier.identifier.name: \
                                        table_name_out;

        /* Lookup Table id of the owning table*/
        if (table_name_out[0] == '\0') {
            /* If column is specified wihout its table name in SQL query,
                then ISt table specified after FROM is taken*/
            table_id = 0;
        }
        else if (column_node->data) {
            /* If column node itself carries the table id, then use it. COlumn node
                will have table IDs if user specified * in select SQL query. 
                see sql_process_select_wildcard( ) */
            table_id = *(int *)column_node->data;
        }
        else {
            /* Otherwise pick the table Id from table node to which the column belongs*/
            ast_tmplate.entity_type = SQL_IDENTIFIER;
            ast_tmplate.u.kw = SQL_TABLE_NAME;
            strncpy(ast_tmplate.u.identifier.identifier.name, table_name_ptr, SQL_TABLE_NAME_MAX_SIZE);
            ast_node_t *table_node = ast_find_identifier (from_kw, &ast_tmplate);
            if (!table_node) {
                printf ("Error : Could not find owner table for column %s\n", 
                    column_node->u.identifier.identifier.name);
                return false;
            }
            table_id = *(int *)table_node->data;
        }

        bpkey.key = table_name_ptr;
        bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;
        ctable_val = (ctable_val_t *) BPlusTree_Query_Key (tcatalog, &bpkey);
        new_col->ctable_val = ctable_val;
        bpkey.key = lone_col_name;
        bpkey.key_size = SQL_COLUMN_NAME_MAX_SIZE;
        new_col->schema_rec = 
            (schema_rec_t *) BPlusTree_Query_Key (ctable_val->schema_table, &bpkey);
        assert(new_col->schema_rec);
        new_col->owner_table_id = table_id;
        new_col->agg_fn = SQL_AGG_FN_NONE;
        FOR_ALL_AST_CHILD (column_node, agg_node) {
            if (agg_node->entity_type != SQL_AGG_FN) continue;
            new_col->agg_fn = agg_node->u.agg_fn;
            break;
        } FOR_ALL_AST_CHILD_END;
        return new_col;
}

static void 
qep_init_where_having_clause (qep_struct_t *qep_struct, ast_node_t *root, sql_keywords_t kw) {

    int size_out = 0;
    ast_node_t ast_tmplate;

    assert (kw == SQL_WHERE || kw == SQL_HAVING);
    ast_tmplate.entity_type = SQL_KEYWORD;
    ast_tmplate.u.kw = kw;

    ast_node_t *clause_node = ast_find (root, &ast_tmplate);
    
    if (!clause_node) return;

    ast_node_t *where_literals_node = clause_node->child_list;

    where_literal_t *where_literals_arr = NULL;

    memcpy ((void *)&where_literals_arr,
                    where_literals_node->u.identifier.identifier.name,
                    sizeof (void *));
    assert (where_literals_arr);

    where_literal_t **where_literals_postfix = 
        sql_where_clause_infix_to_postfix (where_literals_arr, &size_out);

    if (!where_literals_postfix) return;
#if 0
    printf ("postfix : \n");
    sql_debug_print_where_literals2 (where_literals_postfix, size_out);
#endif
    if (kw == SQL_WHERE) {
    qep_struct->expt_root = 
                sql_where_convert_postfix_to_expression_tree (where_literals_postfix , size_out);
    }
    else {
        qep_struct->groupby.expt_root = 
                sql_where_convert_postfix_to_expression_tree (where_literals_postfix , size_out);
    }
#if 0
    printf ("Expression Tree :\n");
    sql_debug_print_expression_tree (qep_struct->expt_root);
#endif
    free (where_literals_postfix);
}

bool
qep_struct_init (qep_struct_t *qep_struct, BPlusTree_t *tcatalog, ast_node_t *root) {

    int i ;
    int table_id;
    int n_cols = 0;
    int table_cnt = 0;
    BPluskey_t bpkey;
    ctable_val_t *ctable_val;
    ast_node_t ast_tmplate, *curr;

    memset (&ast_tmplate, 0, sizeof (ast_tmplate));
    ast_tmplate.entity_type = SQL_KEYWORD;
    ast_tmplate.u.kw = SQL_FROM;
    ast_node_t *from_kw = ast_find (root, &ast_tmplate);

    for (curr = from_kw->child_list; curr; curr = curr->next) {
        table_cnt++;
    }

    qep_struct->ctable_val = (ctable_val_t **) calloc (table_cnt, sizeof (ctable_val_t * ));

    for (curr = from_kw->child_list, i = 0; curr; curr = curr->next, i++) {

        bpkey.key = curr->u.identifier.identifier.name;
        bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;

        ctable_val = (ctable_val_t *) BPlusTree_Query_Key (tcatalog, &bpkey);

        if (!ctable_val) {
            printf ("Error : relation %s does not exist\n", curr->u.identifier.identifier.name);
            return false;
        }
        qep_struct->ctable_val[i] = ctable_val ;
    }
    
    qep_struct->join.table_cnt = table_cnt;

    ast_node_t *column_node;

    ast_tmplate.entity_type = SQL_KEYWORD;
    ast_tmplate.u.kw = SQL_SELECT;
    ast_node_t *select_kw = ast_find (root, &ast_tmplate);

    FOR_ALL_AST_CHILD (select_kw, column_node) {
        n_cols++;
    } FOR_ALL_AST_CHILD_END;
    
    qep_struct->select.n = n_cols;
    qep_struct->select.sel_colmns = (qp_col_t **)calloc (n_cols, sizeof (qp_col_t *));
    i = 0;
    unsigned char table_name_out [SQL_TABLE_NAME_MAX_SIZE];
    unsigned char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE];
    unsigned char *table_name_ptr = table_name_out;

    FOR_ALL_AST_CHILD (select_kw, column_node) {

        qep_struct->select.sel_colmns[i++] = qep_init_column(qep_struct, 
                                                                        tcatalog,
                                                                        from_kw,
                                                                        column_node);

    } FOR_ALL_AST_CHILD_END;

    qep_struct->is_join_started = false;
    qep_struct->is_join_finished = false;

    table_iterators_init (qep_struct, &qep_struct->titer);

    qep_struct->ht = create_hashtable
                            (sql_compute_group_by_key_size (qep_struct), 
                            hashfromkey, equalkeys);    

    qep_init_where_having_clause  (qep_struct, root, SQL_WHERE);

    /* Initializing Group by Clause BEGIN */
    ast_tmplate.entity_type = SQL_KEYWORD;
    ast_tmplate.u.kw = SQL_GROUP_BY;
    ast_node_t *groupby_kw = ast_find (root, &ast_tmplate);
     n_cols = 0;

    FOR_ALL_AST_CHILD (groupby_kw, column_node) {

        if (column_node->entity_type == SQL_KEYWORD &&
                column_node->u.kw == SQL_HAVING) continue;
        n_cols++;
    } FOR_ALL_AST_CHILD_END;

    qep_struct->groupby.n = n_cols;
    qep_struct->groupby.col_list = (qp_col_t **)calloc (n_cols, sizeof (qp_col_t *));
    i = 0;

    FOR_ALL_AST_CHILD (groupby_kw, column_node) {

        if (column_node->entity_type == SQL_KEYWORD &&
                column_node->u.kw == SQL_HAVING) continue;

        qep_struct->groupby.col_list[i++] = qep_init_column(qep_struct, 
                                                                        tcatalog,
                                                                        from_kw,
                                                                        column_node);

    } FOR_ALL_AST_CHILD_END;    

    qep_init_where_having_clause (qep_struct, root, SQL_HAVING);
    /* Initializing Group by Clause DONE */
}

void
qep_execute_delete (qep_struct_t *qep_struct) {

    void *rec;
    glthread_t *curr;
    list_node_t *lnode;
    uint32_t count = 0;
    BPluskey_t *bpkey;
    BPluskey_t *bpkey_copy;
    list_node_t list_node_head;

    list_node_head.data = NULL;
    init_glthread (&list_node_head.glue);

    BPTREE_ITERATE_ALL_RECORDS_BEGIN(qep_struct->ctable_val[0]->rdbms_table, bpkey, rec) {

        rec = qep_enforce_where (qep_struct, qep_struct->ctable_val[0]->schema_table, rec, qep_struct->expt_root, 0);
        if (!rec) continue;
        lnode = (list_node_t *) calloc (1, sizeof (list_node_t));
        /* Do not cache the direct ptr to the keys in a linkedList, because on node deletion,
            B+Tree internally restructure itself and re-adjust key pointers ! Hence cache the
            copy of the keys to delete later*/
        bpkey_copy = (BPluskey_t *) calloc (1, sizeof (BPluskey_t));
        *bpkey_copy = *bpkey;
        lnode->data = (void *)bpkey_copy;
        init_glthread (&lnode->glue);
        glthread_add_next (&list_node_head.glue, &lnode->glue);

    } BPTREE_ITERATE_ALL_RECORDS_END(qep_struct->ctable_val1->rdbms_table, bpkey, rec) ;

    ITERATE_GLTHREAD_BEGIN(&list_node_head.glue, curr) {
        
        lnode = glue_to_list_node(curr);
        bpkey = (BPluskey_t *) lnode->data;
        BPlusTree_Delete (qep_struct->ctable_val[0]->rdbms_table, bpkey);
        free(bpkey);
        remove_glthread(&lnode->glue);
        free(lnode);
        count++;

    } ITERATE_GLTHREAD_END(&list_node_head.glue, curr) ;

    printf ("DELETE %u\n", count);
}

void 
qep_deinit (qep_struct_t *qep_struct) {

    int i;
    qp_col_t *col;

    free (qep_struct->ctable_val);

    if (qep_struct->expt_root) {
        expt_destroy (qep_struct->expt_root);
    }
    
    if (qep_struct->groupby.n) {

        for (i = 0; i < qep_struct->groupby.n; i++) {
            col =  qep_struct->groupby.col_list[i];
            if (col->computed_value) {
                free(col->computed_value);
            }
            free(col);
        }
        free(qep_struct->groupby.col_list);

        if (qep_struct->groupby.expt_root) {
            expt_destroy  (qep_struct->groupby.expt_root) ;
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
        free ( qep_struct->select.sel_colmns);
    }

    if (hashtable_count (qep_struct->ht)) {
        hashtable_destroy (qep_struct->ht, 1); 
        qep_struct->ht = NULL;
    }

    free (qep_struct->titer);
}