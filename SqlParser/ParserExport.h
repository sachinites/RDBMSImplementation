#ifndef __PARSER_EXPORT__
#define __PARSER_EXPORT__

#include <stdint.h>

#define MAX_MEXPR_LEN  512

extern "C" int yylex();

typedef enum parse_rc_ {

    PARSE_ERR,
    PARSE_SUCCESS

} parse_rc_t;

typedef struct lex_data_ {

    int token_code;
    int token_len;
    char* token_val;
} lex_data_t;

#define MAX_STRING_SIZE 512

typedef struct stack_ {

    int top;
    lex_data_t data[MAX_MEXPR_LEN];
} stack_t;

extern char lex_buffer[MAX_STRING_SIZE];
extern char *curr_ptr ;
extern char *lex_curr_token;
extern int lex_curr_token_len;
extern stack_t undo_stack;
extern void yyrewind (int n) ;
extern char *parser_alloc_token_value_default (uint16_t token_id);
extern int cyylex ();
extern void process_white_space(int n) ;
extern void RESTORE_CHKP(int a);

#define parse_init()             \
    int token_code = 0;          \
    int _lchkp = undo_stack.top;    \
    parse_rc_t err = PARSE_SUCCESS

#define RETURN_PARSE_ERROR  \
    {RESTORE_CHKP(_lchkp);     \
    return PARSE_ERR;}

#define RETURN_PARSE_SUCCESS    \
    return PARSE_SUCCESS

#define PARSER_CALL(fn) \
    fn()

#define CHECKPOINT(a)    \
    a = undo_stack.top
  
#define CHECK_FOR_EOL                \
    {token_code = cyylex();                   \
    if (token_code == EOL) {                \
        RETURN_PARSE_SUCCESS;   \
    }}

extern int cyylexlh() ;
extern  int cyylexlb() ;

#define PARSER_LOG_ERR(token_obtained, expected_token)  \
    printf ("%s(%d) : Token Obtained = %d (%s) , expected token = %d\n",    \
        __FUNCTION__, __LINE__, token_obtained, lex_curr_token, expected_token);

extern void Parser_stack_reset () ;
extern int  Parser_get_current_stack_index ();
extern void lex_set_scan_buffer (const char *buffer) ;

#define ITERATE_LEX_STACK_BEGIN(i , j , token_code_, len_, value_)    \
{   int _k;                                                                                                              \
     for (_k = i; _k <= j && _k <= undo_stack.top; _k++) {                               \
     lex_data_t *lex_data = &undo_stack.data[_k];  \
     if (lex_data->token_code == 0 || lex_data->token_code == WHITE_SPACE) continue; \
     token_code_ = lex_data->token_code;               \
     len_ = lex_data->token_len;                               \
     value_ = lex_data->token_val;

#define ITERATE_LEX_STACK_END }}

/* Common Codes */
#define PARSER_EOL  10000
#define PARSER_QUIT 10001
#define PARSER_WHITE_SPACE  10002
#define PARSER_CONTINUE_NEXTLINE    10003
#define PARSER_INVALID_CODE INT32_MAX

#endif 