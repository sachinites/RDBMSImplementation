#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include "sql_join.h"
#include "qep.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "Catalog.h"
#include "sql_const.h"
#include "rdbms_struct.h"

/* Look up the catalog table using table name specified in join list
    and cache the pointer to ctable_val_t object, so that diring join
    execution we dont have to do any lookups ( performance )*/

bool 
sql_query_initialize_join_clause  (qep_struct_t *qep, BPlusTree_t *tcatalog) {

    int i;

    BPluskey_t bpkey;

    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;

    for (i = 0; i < qep->join.table_cnt; i++) {

        bpkey.key = (void *)qep->join.tables[i].table_name;
        qep->join.tables[i].ctable_val = (ctable_val_t *)BPlusTree_Query_Key 
                                                            (tcatalog, &bpkey);

       if (!qep->join.tables[i].ctable_val) {
            printf ("Error : Could not find table %s\n", qep->join.tables[i].table_name);
            return false;
       }
    }

    return true;
}

/* Fetch the First record from each participating tables. 
    1. If records from each table is read, then place it in joined row.
        set qep->is_join_started = true, done
    2. If record from any table could not be read (bcoz table is empty), then set qep->is_join_finished = true , done
*/
bool
table_iterators_first (qep_struct_t *qep, 
                                 table_iterators_t *titer ) {

    int i;
    void *rec;
    BPluskey_t *bp_key;

    qep->is_join_started = true;
    qep->is_join_finished = false;

    for (i = 0; i < qep->join.table_cnt; i++) {

        assert (titer->table_iter_data[i].bpnode == NULL);
        assert (titer->table_iter_data[i].index == 0);

        rec = BPlusTree_get_next_record (
                    titer->table_iter_data[i].ctable_val->record_table,
                    &titer->table_iter_data[i].bpnode,
                    &titer->table_iter_data[i].index,
                    &bp_key);

        if (!rec) {

            qep->is_join_finished = true;
            qep->is_join_started = false;
            return false;
        }

        // place the key and record read from ith table into joined row at index i

        qep->joined_row_tmplate->key_array[i] = bp_key;
        qep->joined_row_tmplate->rec_array[i] = rec;
    }

    return true;
}

void
table_iterators_next (qep_struct_t *qep, 
                                  table_iterators_t *titer, 
                                  int table_id) {

    void *rec = NULL;
    BPluskey_t *bp_key;

    if (table_id < 0) return;

    do {

        rec = BPlusTree_get_next_record(
                    titer->table_iter_data[table_id].ctable_val->record_table,
                    &titer->table_iter_data[table_id].bpnode,
                    &titer->table_iter_data[table_id].index,
                    &bp_key);

        if (!rec) break;

        qep->joined_row_tmplate->key_array[table_id] = bp_key;
        qep->joined_row_tmplate->rec_array[table_id] = rec;
        return;

    } while (0);

    // 
    qep->joined_row_tmplate->key_array[table_id] = NULL;
    qep->joined_row_tmplate->rec_array[table_id] = NULL;

    table_iterators_next (qep, titer, table_id - 1);

    if (table_id == 0) {

        qep->is_join_finished = true;
        return;
    }

    /* if the inner table finds that the outer table could not find any record, then abort the iteration*/
    if (!qep->joined_row_tmplate->rec_array[table_id - 1]) {
        qep->is_join_finished = true;
        return;
    }

    rec = BPlusTree_get_next_record(
                    titer->table_iter_data[table_id].ctable_val->record_table,
                    &titer->table_iter_data[table_id].bpnode,
                    &titer->table_iter_data[table_id].index,
                    &bp_key);

    assert(rec);

    qep->joined_row_tmplate->key_array[table_id] = bp_key;
    qep->joined_row_tmplate->rec_array[table_id] = rec;
}

bool
qep_execute_join (qep_struct_t *qep) {

    if (!qep->is_join_started) {

        table_iterators_first (qep, qep->titer);

        /* We we could not find ISt recprd from each joined tables*/
        if (qep->is_join_finished) return false;

        return true;
    }

    table_iterators_next (qep, qep->titer, qep->join.table_cnt - 1);

     return (!qep->is_join_finished);
}

void 
table_iterators_init (qep_struct_t *qep,
                                table_iterators_t **_titer) {

    int i;

    (*_titer) = (table_iterators_t *)calloc (1, 
                            sizeof(table_iterators_t) +  
                            sizeof(table_iter_data_t) * qep->join.table_cnt);

    table_iterators_t *titer = *_titer;

    titer->table_cnt = qep->join.table_cnt;

    for (i = 0; i < titer->table_cnt; i++) {
        titer->table_iter_data[i].bpnode = NULL;
        titer->table_iter_data[i].index = 0;
        titer->table_iter_data[i].ctable_val = qep->join.tables[i].ctable_val;
    }

}