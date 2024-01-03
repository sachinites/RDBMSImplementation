#ifndef __PARSER_EXPORT__
#define __PARSER_EXPORT__


#include <assert.h>

// Write all the supporting C - code declarations/definitions

// Define all supporive data structures BEGIN
typedef enum parse_rc_ {

    PARSE_ERR,
    PARSE_SUCCESS

} parse_rc_t;


typedef struct lex_data_ {

    int token_code;
    int token_len;
    char* token_val;

} lex_data_t;


#define MAX_MEXPR_LEN  512
#define MAX_STRING_SIZE 512
#define PARSER_EOL  10000
#define PARSER_WHITE_SPACE 10002

typedef struct stack_ {

    int top;
    lex_data_t data[MAX_MEXPR_LEN];
} stack_t;


// Define all supporive data structures END


// dedicated to declare all global variables which parser will use BEGIN

extern "C" int yylex();

extern char lex_buffer[MAX_STRING_SIZE];
extern stack_t undo_stack;
extern int cyylex();
extern void yyrewind(int n);
extern char *curr_ptr ;
extern char *lex_curr_token;
extern int lex_curr_token_len;
extern void  Parser_stack_reset ();
extern void lex_set_scan_buffer (const char *buffer);
extern void RESTORE_CHKP(int a);

#define CHECKPOINT(a)    \
    a = undo_stack.top

#define RETURN_PARSE_SUCCESS    \
    return PARSE_SUCCESS

#define RETURN_PARSE_ERROR  \
    {RESTORE_CHKP(_lchkp);     \
    return PARSE_ERR;}

#define parse_init()             \
    int token_code = 0;          \
    int _lchkp = undo_stack.top;    \
    parse_rc_t err = PARSE_SUCCESS

#define PARSER_LOG_ERR(token_obtained, expected_token)  \
    printf ("%s(%d) : Token Obtained = %d (%s) , expected token = %d\n",    \
        __FUNCTION__, __LINE__, token_obtained, lex_curr_token, expected_token);
        
#endif 
