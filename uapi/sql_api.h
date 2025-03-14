#ifndef __SQL_API__
#define __SQL_API__

#include "../../MathExpressionParser/Dtype.h"
#include "../../MathExpressionParser/MExprcppEnums.h"

#include <string>
#include <vector>

class Dtype;

typedef void (*sql_record_reader_fn_ptr)(void *, std::vector<Dtype *> *); 

void
sql_query_exec (char * sql_query); 

#endif 