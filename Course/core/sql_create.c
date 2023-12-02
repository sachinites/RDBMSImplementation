#include <memory.h>
#include <stdio.h>
#include "sql_create.h"

void 
sql_create_data_destroy (sql_create_data_t *cdata) {

    memset (cdata, 0, sizeof (*cdata));
}

 void 
 sql_process_create_query (sql_create_data_t *cdata) {

    printf ("%s() : called ...\n", __FUNCTION__);
 }

