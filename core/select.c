#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "Catalog.h"
#include "select.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "../Parsers/Ast.h"
#include "sql_const.h"
#include "rdbms_struct.h"
#include "sql_io.h"

void 
sql_process_select_query_internal (BPlusTree_t *schema_table, 
                                                         BPlusTree_t *data_table,
                                                         ast_node_t *root) {

    /* we wil implement a very simple select statement of the format :
        select <col1>, <col2>, .... from <table_name> 
    */
    int i = 0;
    int total_rows = 0;
    void *record_ptr;
    BPluskey_t *bpkey_ptr;
    glthread_t col_list_head;
    list_node_t *lnode;
    ast_node_t *col_node;
    ast_node_t *table_name_node = root->child_list;

    init_glthread (&col_list_head);

    for (col_node = table_name_node->child_list; col_node; col_node = col_node->next) {

        lnode = calloc (1, sizeof (list_node_t));
        lnode->data = (void *)col_node->u.identifier.identifier.name;
        init_glthread (&lnode->glue);
        glthread_add_last (&col_list_head, &lnode->glue);
    }

    BPTREE_ITERATE_ALL_RECORDS_BEGIN(data_table, bpkey_ptr, record_ptr) {

    if  ((i % 24) == 0) {
        sql_print_hdr (&col_list_head);
    }
    i++;
    total_rows++;

     sql_emit_select_output(schema_table, &col_list_head, record_ptr);
        
   } BPTREE_ITERATE_ALL_RECORDS_END(data_table, bpkey_ptr, record_ptr);

    glthread_t *curr;
    ITERATE_GLTHREAD_BEGIN (&col_list_head, curr) {

        lnode = glue_to_list_node(curr);
        free(lnode);

    } ITERATE_GLTHREAD_END (&col_list_head, curr) 

    printf ("(%d rows)\n", total_rows);
}

bool 
sql_validate_select_query_data (BPlusTree_t *schema_table, ast_node_t *root) {

    return true;
}