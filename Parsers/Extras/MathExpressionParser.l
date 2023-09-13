%{

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <arpa/inet.h>

int yylex();

typedef enum parse_rc_ {

    PARSE_ERR,
    PARSE_SUCCESS

} parse_rc_t;


#define MAX_STRING_SIZE 512
#define MAX_STACK_SIZE  512

static unsigned char lex_buffer[MAX_STRING_SIZE] = {0};
static unsigned char *curr_ptr = lex_buffer;

typedef uint8_t * (*parser_alloc_token_value_fn_ptr_t) (uint16_t );
parser_alloc_token_value_fn_ptr_t parser_alloc_token_value_fn_ptr = NULL;

typedef struct lex_data_ {

    int token_code;
    int token_len;
    uint8_t *token_val;
} lex_data_t;

typedef struct stack {

    int top;
    lex_data_t data[MAX_STACK_SIZE];
} stack_t;

static stack_t undo_stack = {-1, {0, 0, 0}};

static void 
push(lex_data_t lex_data) {
    assert (undo_stack.top < MAX_STACK_SIZE -1);
    undo_stack.data[++undo_stack.top] = lex_data;
}

static lex_data_t
pop() {
    assert (undo_stack.top > -1);
    lex_data_t res = undo_stack.data[undo_stack.top] ;
    undo_stack.top--;
    return res;
}

static void 
yyrewind (int n) {

    if (n <= 0) return;
    if (curr_ptr == lex_buffer) return;
    int data_len = 0;
    lex_data_t lex_data;
    while (n)  {
        lex_data = pop();
        data_len += lex_data.token_len;
        n--;
        lex_data.token_code = 0;
        lex_data.token_len = 0;
        if (lex_data.token_val) {
            free (lex_data.token_val);
            lex_data.token_val = NULL;
        }
    }
    curr_ptr -= data_len;
    yy_scan_string(curr_ptr);
}

static int 
cyylex () {

    int token_code =  yylex();
    curr_ptr += yyleng;
    lex_data_t lex_data;
    lex_data.token_code = token_code;
    lex_data.token_len = yyleng;
    if (parser_alloc_token_value_fn_ptr ) {
        lex_data.token_val = parser_alloc_token_value_fn_ptr (token_code);
    }
    else {
        lex_data.token_val = NULL;
    }
    push(lex_data);
    return token_code;
}

static void 
process_white_space(int n) {

    lex_data_t lex_data;
    curr_ptr += n;
    lex_data.token_code = 0;
    lex_data.token_len = n;
    lex_data.token_val = NULL;
    push(lex_data);
}

#define parse_init()             \
    int token_code = 0;          \
    int _lchkp = undo_stack.top;    \
    parse_rc_t err = PARSE_SUCCESS

#define RETURN_PARSE_ERROR      \
    {yyrewind(undo_stack.top - _lchkp);     \
    return PARSE_ERR;}

#define RETURN_PARSE_SUCCESS    \
    return PARSE_SUCCESS

#define PARSER_CALL(fn) \
    fn()

#define CHECKPOINT(a)    \
    a = undo_stack.top

#define RESTORE_CHKP(a) \
    yyrewind(undo_stack.top - a)
    
#define CHECK_FOR_EOL                \
    {token_code = cyylex();                   \
    if (token_code == EOL) {                \
        RETURN_PARSE_SUCCESS;   \
    }}

static inline int
cyylexlh() {

    int token_code = cyylex();
    yyrewind(1);
    return token_code;
}

static inline int
cyylexlb() {

    yyrewind(1);
    int token_code = cyylex();
    yyrewind(1);
    return token_code;
}

#define PARSER_LOG_ERR(token_obtained, expected_token)  \
    printf ("%s(%d) : Token Obtained = %d (%s) , expected token = %d\n",    \
        __FUNCTION__, __LINE__, token_obtained, yytext, expected_token);

void 
Parser_init (parser_alloc_token_value_fn_ptr_t fn_ptr) {

    parser_alloc_token_value_fn_ptr = fn_ptr;
}

void 
Parser_stack_reset () {

    int i;
    lex_data_t *lex_data;

    for (i = 0; i < undo_stack.top; i++) {
        lex_data = &undo_stack.data[i];
        lex_data->token_code = 0;
        lex_data->token_len = 0;
        if (lex_data->token_val) {
            free (lex_data->token_val);
            lex_data->token_val = NULL;
        }
    }
    undo_stack.top = -1;
}

int 
Parser_get_current_stack_index () {
    return undo_stack.top;
}

#define ITERATE_LEX_STACK_BEGIN(i , j , token_code_, len_, value_)    \
{   int _k;                                                                                                              \
     for (_k = i; _k <= j && _k <= undo_stack.top; _k++) {                               \
     lex_data_t *lex_data = &undo_stack.data[_k];  \
     if (lex_data->token_code == 0 || lex_data->token_code == WHITE_SPACE) continue; \
     token_code_ = lex_data->token_code;               \
     len_ = lex_data->token_len;                               \
     value_ = lex_data->token_val;

#define ITERATE_LEX_STACK_END }}


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

Overall Grammar is :
0. Q -> S Ineq S  
1. S -> E
2. E -> E + T | E - T | T
3. T -> T * F | T / F | F
4. F -> ( E ) | INTEGER | DECIMAL | VAR

Now remove left recursion from it :

2. E -> E + T | E - T | T

E ->  T E'
E' -> + T E' | - T E' |  $   ( $ represents epsilon)

3. T -> T * F | T / F | F

T -> F T'
T' -> * F T' |   / F T'  |  $

Combining everything, final grammar is :
===============================

1. Q  ->   E Ineq E  
2. E  ->   T E'
3. E'  ->  + T E' | - T E' |  $
4. T  ->   F T'
5. T' ->   * F T' |   / F T'  |  $
6. F  ->   ( E ) |  P ( E ) | INTEGER | DECIMAL | VAR | G ( E, E)
7. P -> sqrt
8. G -> max
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
#define UAGG_FN  17
#define BAGG_FN  18
#define COMMA   19

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
        case UAGG_FN: return "UAgg_fn";
        case BAGG_FN: return "BAgg_fn";
        case COMMA: return ",";
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

(sqrt|sqr) {

    return UAGG_FN;
}

(max|min) {

    return BAGG_FN;
}

[0-9]+\.[0-9]+ {

    return DECIMAL;
}

[a-zA-Z0-9]+ {
    return VAR ;
}

[a-zA-Z0-9]+\.[a-zA-Z0-9]+ {
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

"," {
    return COMMA;
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


%%

/*
Template : 

switch(err) {
    case PARSE_ERR:
    break;
    case PARSE_SUCCESS:
    break;
}

*/
parse_rc_t Ineq () ;
parse_rc_t F ();
parse_rc_t T_dash () ;
parse_rc_t T () ;
parse_rc_t E_dash () ;
parse_rc_t E () ;
parse_rc_t Q () ;

parse_rc_t
Ineq () {

    parse_init();

    token_code = cyylex();
    switch(token_code) {
        case LESS_THAN:
        case GREATER_THAN:
        case EQ:
        case NOT_EQ:
            break;
        default:
            RETURN_PARSE_ERROR;
    }

    assert(err == PARSE_SUCCESS);
    RETURN_PARSE_SUCCESS;
}


parse_rc_t
G () {

     parse_init();

     token_code = cyylex();

     switch (token_code) {

        case BAGG_FN:
            RETURN_PARSE_SUCCESS;
        default:
            RETURN_PARSE_ERROR;
     }

}

parse_rc_t
P () {

    parse_init();

    token_code = cyylex();

    switch (token_code) {

        case UAGG_FN:
            RETURN_PARSE_SUCCESS;
        default:
            RETURN_PARSE_ERROR
    }
}

parse_rc_t
F () {

    parse_init();

    token_code = cyylex();

    switch (token_code) {

        case BRACK_START:
        {
            err = PARSER_CALL(E);
            switch (err) {
                case PARSE_ERR:
                    RETURN_PARSE_ERROR;
                case PARSE_SUCCESS:
                {
                    token_code = cyylex();
                    if (token_code != BRACK_END) {
                        RETURN_PARSE_ERROR;
                    }
                    RETURN_PARSE_SUCCESS;
                }
                break;
            }
        }
        break;
        case INTEGER:
        case DECIMAL:
        case VAR:
        case VAR_VAR:
            RETURN_PARSE_SUCCESS;
        default:
            yyrewind(1);
            err = PARSER_CALL(P);
            switch(err) {
                case PARSE_ERR:
                    err = PARSER_CALL(G);
                    switch(err) {
                        case PARSE_ERR:
                            RETURN_PARSE_ERROR;
                        case PARSE_SUCCESS:
                            token_code = cyylex();
                            switch(token_code) {
                                case BRACK_START:
                                    err = PARSER_CALL(E);
                                    switch(err) {
                                        case PARSE_ERR:
                                            RETURN_PARSE_ERROR;
                                        case PARSE_SUCCESS:
                                            token_code = cyylex();
                                            switch(token_code) {
                                                case COMMA:
                                                    err = PARSER_CALL(E);
                                                    switch(err) {
                                                        case PARSE_ERR:
                                                            RETURN_PARSE_ERROR;
                                                        case PARSE_SUCCESS:
                                                            token_code = cyylex();
                                                            if (token_code != BRACK_END) {
                                                                RETURN_PARSE_ERROR;
                                                            }
                                                            RETURN_PARSE_SUCCESS;
                                                    }
                                                default:
                                                    RETURN_PARSE_ERROR;
                                            }
                                    }
                                default:
                                    RETURN_PARSE_ERROR;
                            }
                    }
                case PARSE_SUCCESS:
                    token_code = cyylex();
                    switch(token_code){
                        case BRACK_START:
                            err = PARSER_CALL(E);
                            switch(err) {
                                case PARSE_ERR:
                                    RETURN_PARSE_ERROR;
                                case PARSE_SUCCESS:
                                    token_code = cyylex();
                                    if (token_code != BRACK_END) {
                                        RETURN_PARSE_ERROR;
                                    }
                                    RETURN_PARSE_SUCCESS;
                            }
                    }
            }
    }
}

parse_rc_t
T_dash () {

    parse_init();

    token_code = cyylex();
    switch(token_code){

        case OP_MULT:
        case OP_DIV:
        {
            err = PARSER_CALL(F);
            switch (err) {
                case PARSE_ERR:
                    RETURN_PARSE_ERROR;
                case PARSE_SUCCESS:
                    err = PARSER_CALL(T_dash);
                    switch (err) {
                        case PARSE_ERR:
                            RETURN_PARSE_ERROR;
                        case PARSE_SUCCESS:
                            RETURN_PARSE_SUCCESS;
                    }
            }
        }
        break;
        default:
            yyrewind(1);
            RETURN_PARSE_SUCCESS;
    }
}


parse_rc_t
T () {

    parse_init();

    err = PARSER_CALL(F);
    switch (err) {
        case PARSE_ERR:
            RETURN_PARSE_ERROR;
        case PARSE_SUCCESS:
            err = PARSER_CALL(T_dash);
            switch(err) {
                case PARSE_ERR:
                    RETURN_PARSE_ERROR;
                case PARSE_SUCCESS:
                    RETURN_PARSE_SUCCESS;
            }
    }
}

parse_rc_t
E_dash () {

    parse_init();

    token_code = cyylex();

    switch (token_code) {
        case OP_PLUS:
        case OP_MINUS:
        {
            err = PARSER_CALL(T);
            switch(err){
                case PARSE_ERR:
                    RETURN_PARSE_ERROR;
                case PARSE_SUCCESS:
                    err = PARSER_CALL(E_dash);
                    switch(err) {
                        case PARSE_ERR:
                            RETURN_PARSE_ERROR;
                        case PARSE_SUCCESS:
                             RETURN_PARSE_SUCCESS;
                    }
            }
        }
        break;
        default:
            yyrewind(1);
            RETURN_PARSE_SUCCESS;
    }
}

parse_rc_t
E () {

    parse_init();

    err = PARSER_CALL(T);
    switch (err) {
        case PARSE_ERR:
            RETURN_PARSE_ERROR;
        case PARSE_SUCCESS:
            err = PARSER_CALL(E_dash);
            switch(err) {
                case PARSE_ERR:
                   RETURN_PARSE_ERROR;
                case PARSE_SUCCESS:
                    RETURN_PARSE_SUCCESS;
            }
    }
}

parse_rc_t
Q () {

    parse_init();

    err = PARSER_CALL(E);
    switch (err) {
        case PARSE_ERR:
            RETURN_PARSE_ERROR;
        case PARSE_SUCCESS:
            err = PARSER_CALL(Ineq);
            switch(err) {
                case PARSE_ERR:
                    RETURN_PARSE_ERROR;
                case PARSE_SUCCESS:
                    err = PARSER_CALL(E);
                    switch (err) {
                        case PARSE_ERR:
                            RETURN_PARSE_ERROR;
                        case PARSE_SUCCESS:
                            RETURN_PARSE_SUCCESS;
                    }
            }
    }
}


int 
main (int argc, char **argv) {

    Parser_init(0);
#if 1
    strcpy (lex_buffer , 
        "a + b * c\n");
    yy_scan_string (lex_buffer);
    parse_init();
    err = PARSER_CALL(E);
    token_code = cyylex();
    assert (token_code == EOL);
    printf ("Valid Mathematical Expression\n");
    return 0;
#endif 

#if 0
    strcpy (current_buffer , 
        "max( sqrt(max(b.n , c)) + max(sqrt(b) * c, d) , sqrt(max(b.n , c)) + max(sqrt(b) * c, d))\n");
#endif
#if 0
    strcpy (current_buffer , 
        "max( sqrt(max(b.n , c)) + max(sqrt(b) * c, d) , sqrt(max(b.n , c)) + max(sqrt(b) * c, d))  !=  max( sqrt(max(b.n , c)) + max(sqrt(b) * c, d) , sqrt(max(b.n , c)) + max(sqrt(b) * c, d))\n");
#endif
#if 0
    strcpy (current_buffer , 
        "emp.salary * 3 < emp.accntbalance,   a + b * c < 1 ,   sqrt (max(a, b)) > 4\n");
    yy_scan_string (current_buffer);
    parse_init();
   
    err = PARSER_CALL(Q);
    switch (err) {
        case PARSE_ERR:
            printf ("Invalid Mathematical Expression\n");
            break;
        case PARSE_SUCCESS:
            token_code = cyylex();
            while (token_code == COMMA) {
                printf ("Valid Mathematical Expression\n");
                err = PARSER_CALL(Q);
                switch (err) {
                    case PARSE_ERR:
                        printf ("Invalid Mathematical Expression\n");
                        exit(0);
                    case PARSE_SUCCESS:
                        token_code = cyylex();
                        break;
                }
            }
            assert (token_code == EOL);
            printf ("Valid Mathematical Expression\n");
            break;
    }
#endif 
    Parser_stack_reset();
    return 0;
}
