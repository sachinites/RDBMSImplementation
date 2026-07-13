#ifndef __SQL_PARSER_BIND__
#define __SQL_PARSER_BIND__

#include "../uapi/rdbms_ctx.h"
#include "ParserExport.h"

/* Declare in every grammar function before using parse_init()/cyylex() macros. */
#define RDBMS_PARSER_BIND(rdbms) \
    mexpr_parser_t *p = (rdbms)->parser

parse_rc_t select_query_parser (rdbms_t *rdbms);
parse_rc_t create_query_parser (rdbms_t *rdbms);
parse_rc_t insert_into_query_parser (rdbms_t *rdbms);
parse_rc_t delete_query_parser (rdbms_t *rdbms);
parse_rc_t update_query_parser (rdbms_t *rdbms);

#endif
