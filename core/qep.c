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
#include "sql_where.h"
#include "sql_groupby.h"
#include "../c-hashtable/hashtable.h"
#include "../c-hashtable/hashtable_itr.h"
#include "sql_mexpr_intf.h"


extern BPlusTree_t TableCatalogDef;

void
qep_execute_select (qep_struct_t *qep_struct) {

}

void
qep_execute_delete (qep_struct_t *qep_struct) {

}

bool
qep_struct_init (qep_struct_t *qep_struct, BPlusTree_t *tcatalog) {

    return true;
}

void 
qep_deinit (qep_struct_t *qep_struct) {

}


bool 
qep_struct_record_table (qep_struct_t *qep_struct, unsigned char *table_name) {

    BPluskey_t bpkey;
    ctable_val_t *ctable_val;

    bpkey.key = table_name;
    bpkey.key_size = SQL_TABLE_NAME_MAX_SIZE;

    ctable_val = (ctable_val_t *) BPlusTree_Query_Key (&TableCatalogDef, &bpkey);
    if (!ctable_val) return false;

    qep_struct->join.tables[qep_struct->join.table_cnt++].ctable_val = ctable_val ;
    return true;
}

void 
sql_process_select_query (qep_struct_t *qep) {

}