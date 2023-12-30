#include <stdio.h>
#include <assert.h>
#include "sql_join.h"
#include "qep.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "Catalog.h"
#include "sql_const.h"

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
