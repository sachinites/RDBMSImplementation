#ifndef __SQL_NAME__
#define __SQL_NAME__

#include <stdbool.h>
#include <unordered_map>

typedef struct qep_struct_ qep_struct_t;

bool 
sql_is_name_FQCN ( std::unordered_map<std::string, std::string> *table_alias, 
                                    char *name) ;

bool 
sql_is_name_ACN (std::unordered_map<std::string, std::string> *table_alias, 
                                char *name);

bool 
sql_is_name_LCN (char *name);

void 
sql_get_column_table_names ( qep_struct_t *qep,
                                                    char *input_column_name, 
                                                    char *table_name_out,
                                                    char *col_name_out);

void 
sql_convert_name_to_FQCN (qep_struct_t *qep, 
                                                char *input_column_name,
                                                int input_column_name_size);
#endif 

