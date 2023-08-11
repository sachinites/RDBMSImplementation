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
#include "qplanner.h"

void 
sql_process_select_query_internal (BPlusTree_t *tcatalog,
                                                         ast_node_t *root) {

    qep_t *qep = select_qep_prepare_execution_plan (tcatalog, root);
    qep_execute (qep);
}

bool 
sql_validate_select_query_data (BPlusTree_t *schema_table, ast_node_t *root) {

    return true;
}