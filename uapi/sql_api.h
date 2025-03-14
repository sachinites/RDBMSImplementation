#ifndef __SQL_API__
#define __SQL_API__

#include "../../MathExpressionParser/Dtype.h"
#include "../../MathExpressionParser/MExprcppEnums.h"

#include <string>
#include <vector>

typedef struct BPlusTree BPlusTree_t;

class Dtype;

typedef void (*sql_record_reader_fn_ptr)(void *, std::vector<Dtype *> *); 

int
sql_query_exec (BPlusTree_t *sql_db, char * sql_query, char *err_msg); 

void 
sql_init_db (BPlusTree_t **db);

#endif 