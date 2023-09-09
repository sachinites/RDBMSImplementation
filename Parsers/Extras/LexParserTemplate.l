%{

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

int yylex();

typedef enum parse_rc_ {

    PARSE_ERR,
    PARSE_SUCCESS

} parse_rc_t;


#define MAX_STRING_SIZE 512
#define MAX_STACK_SIZE  100

static unsigned char current_buffer[MAX_STRING_SIZE] = {0};
static unsigned char *curr_ptr = current_buffer;

typedef struct stack {

    int top;
    int data[MAX_STACK_SIZE];
} stack_t;

static stack_t undo_stack = {-1, {0}};

static void 
push(int data) {
    assert (undo_stack.top < MAX_STACK_SIZE -1);
    undo_stack.data[++undo_stack.top] = data;
}

static int 
pop() {
    assert (undo_stack.top > -1);
    int res = undo_stack.data[undo_stack.top] ;
    undo_stack.top--;
    return res;
}

static int *glex_cnt = NULL;

static void 
yyrewind (int n) {

    if (n <= 0) return;
    if (curr_ptr == current_buffer) return;
    int data_len = 0;
    while (n)  {
        data_len += pop(); 
        n--;
        (*glex_cnt)--;
    }
    curr_ptr -= data_len;
    yy_scan_string(curr_ptr);
}

static int 
cyylex () {

    int token_code =  yylex();
    curr_ptr += yyleng;
    push(yyleng);
    (*glex_cnt)++;
    return token_code;
}

static void 
process_white_space(int n) {

    if (!glex_cnt) return;
    curr_ptr += n;
    push(n);
    (*glex_cnt) += n;
}

#define parse_init()             \
    int token_code = 0;          \
    int lexc = 0;    \
    parse_rc_t err = PARSE_SUCCESS; \
    glex_cnt = &lexc

#define RETURN_PARSE_ERROR  \
    yyrewind(*glex_cnt); \
    glex_cnt = t;   \
    return PARSE_ERR

#define RETURN_PARSE_SUCCESS    \
     (*t) += lexc;                       \
     glex_cnt = t;  \
    return PARSE_SUCCESS

#define PARSER_CALL(fn) \
    fn(&lexc)

#define CHECKPOINT(a)    \
    a = lexc

#define RESTORE_CHKP(a) \
    yyrewind(lexc - a)
    
#define CHECK_FOR_EOL              \
    token_code = cyylex();                   \
    if (token_code == EOL) {               \
        RETURN_PARSE_SUCCESS;  \
    }                                                       \
    else {                                                \
        yyrewind(1);                                \
    }

/* ========================================
            Warning : Do  Not Modify this file above this line 
    ======================================= */
            /* User specific parsing logic starts below */
%}





%{

/* This file implements the SQL Parser which accespts all SQL queries satisfying the below Grammar 

A Grammar to parse mathematical expression !

1. S -> E
2. E -> E + T | E - T | T
3. T -> T * F | T / F | F
4. F -> ( E ) | INTEGER | DECIMAL | VAR


Grammar for Inequalities : 

Add this production Rule as Ist rule to above Grammar 
0. Q -> S Ineq S  

*/


#define VAR    1
#define VAR_VAR 2
#define INTEGER 3
#define DECIMAL  4
#define BRACK_START 5
#define BRACK_END   6
#define LESS_THAN 7
#define GREATER_THAN 8
#define EQ 9
#define NOT_EQ 10
#define EOL 11
#define QUIT 12
#define OP_PLUS 13
#define OP_MINUS 14
#define OP_MULT 15
#define OP_DIV 16

const char *token_str (int token_code) {

    switch (token_code) {

        case VAR:   return "VAR";
        case VAR_VAR: return "VAR_VAR";
        case INTEGER: return "INTEGER";
        case DECIMAL: return "DECIMAL";
        case BRACK_START: return "(";
        case BRACK_END: return ")";
        case LESS_THAN: return "<";
        case GREATER_THAN: return ">";
        case EQ: return "=";
        case NOT_EQ: return "!=";
        case EOL: return "eol";
        case QUIT: return "quit";
        case OP_PLUS: return "+";
        case OP_MINUS: return "-";
        case OP_MULT: return "*";
        case OP_DIV: return "/";
        default:
            assert(0);
    }
    return NULL;
}

%}

%%

[1-9][0-9]* {
    return INTEGER;
}

[a-zA-Z0-9]+ {
    return VAR ;
}

[0-9]+\.[0-9]+ {

    return DECIMAL;
}

[a-zA-Z0-9]\.[a-zA-Z0-9] {
    return VAR_VAR;
}


"(" {
    return BRACK_START;
}

")" {
    return BRACK_END;
}

"<" {
    return LESS_THAN;
}

">" {
    return GREATER_THAN;
}

"=" {
    return EQ;
}

"!=" {
    return NOT_EQ;
}

"+" {
    return OP_PLUS;
}

"-" {
    return OP_MINUS;
}

"*" {
    return OP_MULT;
}

"/" {
    return OP_DIV;
}

\n {
    return EOL;
}

[ ] {
    /* Ignore */
    process_white_space(1);
}

[\t] {
    /*ignore*/
    process_white_space(4);
}

"\\q" {
    return QUIT;
}

. {
    printf ("Non parsable character : %c\n", *yytext);
}

%%

int 
main (int argc, char **argv) {

   strcpy (current_buffer , "(a + (b * c)) > 3\n");
    yy_scan_string (current_buffer);
    parse_init();
   
    token_code = cyylex ();

    while (token_code != EOL) {

        printf ("token_text = %s, token_len = %d, token_code = %s\n", 
                yytext, yyleng, token_str(token_code));
        token_code = cyylex();
    }
    return 0;
}
