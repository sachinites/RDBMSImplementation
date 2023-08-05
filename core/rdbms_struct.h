#ifndef __RDBMS_STRUCT__
#define __RDBMS_STRUCT__

#include "../Parsers/SQLParserStruct.h"
#include "../gluethread/glthread.h"

typedef struct  key_mdata_ {

    sql_dype_t dtype;
    int size;

} key_mdata_t ;

typedef struct list_node_ {

    void *data;
    glthread_t glue;
} list_node_t;
GLTHREAD_TO_STRUCT (glue_to_list_node, list_node_t, glue);

#endif 
