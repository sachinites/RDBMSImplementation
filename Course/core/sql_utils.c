
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>
#include "qep.h"
#include "sql_const.h"
#include "Catalog.h"
#include "sql_utils.h"
#include "sql_create.h"
#include "SqlMexprIntf.h"

extern BPlusTree_t TableCatalogDef;

qp_col_t *
sql_get_qp_col_by_name (   qp_col_t **qp_col_array, 
                                                        int n, 
                                                        char *name, 
                                                        bool is_alias) {

    int i;
    int len;
    qp_col_t *qp_col;

    len = strlen (name) ;
    
    for (i = 0; i < n; i++) {

        qp_col = qp_col_array[i];

        if (is_alias) {

            if (!qp_col->alias_provided_by_user) continue;
            if (strncmp (name, qp_col->alias_name, SQL_ALIAS_NAME_LEN)) continue;
            if (len != strlen (qp_col->alias_name)) continue;
            return qp_col;
        }
        else {

            if (!sql_is_single_operand_expression_tree  (qp_col->sql_tree)) continue;

            if (strncmp (
                name, 
                qp_col->alias_name,
                SQL_FQCN_SIZE)) continue;

            return qp_col;
        }
    }

    return NULL;
}