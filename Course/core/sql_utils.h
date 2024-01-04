#ifndef __SQL_UTILS__
#define __SQL_UTILS__

#include <string.h>
#include <math.h>
#include "rdbms_struct.h"
#include "../BPlusTreeLib/BPlusTree.h"

qp_col_t *
sql_get_qp_col_by_name (   qp_col_t **qp_col_array, 
                                                        int n, 
                                                        char *name, 
                                                        bool is_alias);

#endif 