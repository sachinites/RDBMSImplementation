#include <stdio.h>
#include <stdlib.h>
#include "qep.h"
#include "SqlMexprIntf.h"
#include "rdbms_struct.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "sql_join.h"
#include "sql_select.h"

extern BPlusTree_t TableCatalogDef;

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
   
    qep->data_src_lst = new std::list<exp_tree_data_src_t *>();
    
     /* Initialize Joined Row*/
    qep->joined_row_tmplate = (joined_row_t *)calloc (1, sizeof (joined_row_t));
    joined_row_t *joined_row_tmplate = qep->joined_row_tmplate;
    joined_row_tmplate->size = qep->join.table_cnt;
    joined_row_tmplate->key_array = (BPluskey_t **) calloc (qep->join.table_cnt, sizeof (BPluskey_t *));
    joined_row_tmplate->rec_array = (void **) calloc (qep->join.table_cnt, sizeof (void *));

    qep_create_alias_to_table_name_mapping (qep);
    
    rc = sql_query_initialize_join_clause (qep, tcatalog) ;
    if (!rc) return rc;

    rc = sql_query_initialize_select_column_list (qep, tcatalog) ;
    if (!rc) return rc;

    table_iterators_init (qep, &qep->titer);

    return true;
}

void
sql_execute_qep (qep_struct_t *qep) {

    bool rc;

    if (!sql_query_init_execution_plan (qep, &TableCatalogDef)) {

        printf ("Error : Failed to initialize Query Execution Plan\n");
        return;
    }

    sql_process_select_query (qep);
}


void 
qep_deinit (qep_struct_t *qep) {

    int i;
    qp_col_t *qp_col;
    exp_tree_data_src_t *data_src;
    
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

        }
    }

    free(qep->titer);
    qep->titer = NULL;

    if (qep->joined_row_tmplate) {
        free (qep->joined_row_tmplate->key_array);
        free (qep->joined_row_tmplate->rec_array);
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

}


