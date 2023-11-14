#ifndef __SQL_API__
#define __SQL_API__

#include <string>
#include <vector>

class Dtype;

typedef void (*sql_record_reader_fn_ptr)(void *, std::vector<Dtype *> *); 

void
sql_select_exec (std::string sql_select_query, 
                            sql_record_reader_fn_ptr sql_record_reader, 
                            void *app_data);

#endif 