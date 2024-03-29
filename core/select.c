#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>
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

    bool rc;
    qep_struct_t qep_struct;
    memset (&qep_struct, 0, sizeof (qep_struct));
    rc = qep_struct_init (&qep_struct, tcatalog, root);
    if (!rc) {
        qep_deinit (&qep_struct);
        return;
    }
    qep_execute_select (&qep_struct);
    qep_deinit (&qep_struct);
}

bool 
sql_validate_select_query_data (BPlusTree_t *tcatalog, ast_node_t *root) {

    return true;
}