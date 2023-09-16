#ifndef __PARSER_EXPORT__
#define __PARSER_EXPORT__

#include <stdint.h>

/* This file defines the dta strure used by parser as well used by other
    parts of the project */

#define MAX_MEXPR_LEN  512

typedef struct lex_data_ {

    int token_code;
    int token_len;
    uint8_t *token_val;
} lex_data_t;


#endif 