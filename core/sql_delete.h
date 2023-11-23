#ifndef __SQL_DELETE__
#define __SQL_DELETE__

typedef struct BPlusTree BPlusTree_t ;

#include <stdbool.h>

void
sql_drop_table (char *table_name);

#endif