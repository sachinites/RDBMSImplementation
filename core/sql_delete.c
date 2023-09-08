#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>
#include "Catalog.h"
#include "../BPlusTreeLib/BPlusTree.h"
#include "../Parsers/Ast.h"
#include "sql_const.h"
#include "rdbms_struct.h"
#include "sql_io.h"
#include "qplanner.h"

bool 
sql_validate_delete_query_data (BPlusTree_t *schema_table, ast_node_t *delete_root) {

    return true;
}

void 
sql_process_delete_query_internal (BPlusTree_t *tcatalog, ast_node_t *delete_root) {

    bool rc;
    qep_struct_t qep_struct;
    memset (&qep_struct, 0, sizeof (qep_struct));
    rc = qep_struct_init (&qep_struct, tcatalog, delete_root);
    if (!rc) {
        qep_deinit (&qep_struct);
        return;
    }
    qep_execute_delete (&qep_struct);
    qep_deinit (&qep_struct);
}