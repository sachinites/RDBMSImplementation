/* This file implements the SQL Parser which accespts all SQL queries satisfying the below Grammar 

A Grammar to parse mathematical expression !

WExpr ::= 1 (WExpr) |
                  2  MExpr INEQ  MExpr | 
                  3 (MExpr) INEQ  MExpr |
                  4  MExpr INEQ  (MExpr) |
                  5 (MExpr) INEQ  (MExpr)

INEQ  ::= < | > | = | != 

MExpr ::= 1 Entity Mop MExpr          |   
                  2 (Entity) Mop MExpr       |      
                  3 Entity Mop (MExpr)       |    
                  4 (Entity) Mop (MExpr)    |  
                  5 Entity    |  
                  6 (Entity) | 
                  7 (MExpr)

Entity ::= VAR | RE | INTEGER | DECIMAL
Mop ::= + | - | * | /


Ex : to derive this Mathemetical Expression : (a + (b * c)) > 3 
Sequence of production rules applied : 
WExpr   -->    (MExpr1) INEQ  MExpr2
INEQ     -->    <
MExpr1 -->    Entity1 Mop1 (MExpr3)
MExpr2  -->   Entity2
Entity2   -->   INTEGER
Entity1   -->   VAR
Mop1       -->   +
MExpr3 -->    Entity3 Mop2 MExpr4
Entity3   -->   VAR
Mop2       -->   *
MExpr4   --> Entity4
Entity4    --> VAR

*/


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

stack_t undo_stack = {-1, {0}};

static void 
push(int data) {
    assert (undo_stack.top < MAX_STACK_SIZE -1);
    undo_stack.data[++undo_stack.top] = data;
}

int 
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

    curr_ptr += n;
    push(n);
    (*glex_cnt) += n;
}

#define parse_init()    \
    int lex_lcl_counter = 0;    \
    glex_cnt = &lex_lcl_counter

#define RETURN_PARSE_ERROR  \
    yyrewind(*glex_cnt); \
    glex_cnt = t;   \
    return PARSE_ERR

#define RETURN_PARSE_SUCCESS    \
     glex_cnt = t;  \
    return PARSE_SUCCESS

#define PARSER_CALL(fn) \
    fn(&lex_lcl_counter)

#define CHECK_FOR_EOL              \
    token_code = cyylex();                   \
    if (token_code == EOL) {               \
        RETURN_PARSE_SUCCESS;  \
    }                                                       \
    else {                                                \
        yyrewind(1);                                \
    }

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

/*
Template : 

switch(err) {
    case PARSE_ERR:
    break;
    case PARSE_SUCCESS:
    break;
}

*/


static  parse_rc_t 
Mop(int *t) {

    parse_rc_t err = PARSE_SUCCESS;
    parse_init();

    int token_code = cyylex();
    switch (token_code){
        case OP_PLUS:
        case OP_MINUS:
        case OP_MULT:
        case OP_DIV:
            break;
        default:
            RETURN_PARSE_ERROR;
    }

    assert(err == PARSE_SUCCESS);
    RETURN_PARSE_SUCCESS;    
}

static parse_rc_t 
Entity(int *t) {

    parse_rc_t err = PARSE_SUCCESS;
    parse_init();

    int token_code = cyylex();
    switch (token_code) {
        case VAR:
        case VAR_VAR:
        case INTEGER:
        case DECIMAL:
            break;
        default:
            RETURN_PARSE_ERROR;
    }

    assert(err == PARSE_SUCCESS);
    RETURN_PARSE_SUCCESS;    
}

static parse_rc_t
Ineq (int *t) {

    parse_rc_t err = PARSE_SUCCESS;
    parse_init();

    int token_code = cyylex();
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

static parse_rc_t
MExpr (int *t) {

    parse_rc_t err = PARSE_SUCCESS;
    parse_init();

    int token_code = cyylex();

    if (token_code == BRACK_START) {
        // (      2,4,6,7
        err = PARSER_CALL(Entity);   // (Entity       2,4,6
        switch (err) {
            case PARSE_ERR:
            {
                err = PARSER_CALL(MExpr);   // (MExpr       7
                switch (err) {
                    case PARSE_ERR:
                    {
                         RETURN_PARSE_ERROR;
                    }
                    break;
                    case PARSE_SUCCESS:
                    {
                        token_code = cyylex();
                        if (token_code != BRACK_END) {
                            RETURN_PARSE_ERROR;
                        }
                        CHECK_FOR_EOL;  // (MExpr)       7 Done
                        RETURN_PARSE_ERROR; 
                    }
                    break;
                }
            }
            break;
            case PARSE_SUCCESS: // (Entity       2,4,6
            {
                token_code = cyylex();
                if (token_code != BRACK_END) {
                     RETURN_PARSE_ERROR;
                }
                // (Entity)       2,4,6
                CHECK_FOR_EOL;     // (Entity)       6 Done
                err = PARSER_CALL(Mop);  // (Entity)  Mop         2,4
                switch (err) {
                    case PARSE_ERR:
                    {
                         RETURN_PARSE_ERROR;
                    }
                    break;
                    case PARSE_SUCCESS: // (Entity)  Mop         2,4
                    {
                        token_code = cyylex();
                        if (token_code == BRACK_START) {
                                // (Entity)  Mop  (       4
                                err = PARSER_CALL(MExpr);   // (Entity)  Mop  (MExpr        4
                                switch (err) {
                                    case PARSE_ERR:
                                    {
                                        RETURN_PARSE_ERROR;
                                    }
                                    break;
                                    case PARSE_SUCCESS:
                                    {
                                        token_code = cyylex();
                                        if (token_code != BRACK_END) {
                                            RETURN_PARSE_ERROR;
                                        }
                                         CHECK_FOR_EOL;// (Entity)  Mop  (MExpr)        4 Done
                                         RETURN_PARSE_ERROR;
                                    }
                                    break;
                                }
                        }
                        else {
                            yyrewind(1);
                                // (Entity)  Mop         2
                                err = PARSER_CALL(MExpr);// (Entity)  Mop  MExpr       2
                                switch (err) {
                                    case PARSE_ERR:
                                    {
                                        RETURN_PARSE_ERROR;
                                    }
                                    break;
                                    case PARSE_SUCCESS:
                                    {
                                        CHECK_FOR_EOL;// (Entity)  Mop  MExpr        2 Done
                                        RETURN_PARSE_ERROR;
                                    }
                                    break;
                                }
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
    else {
        yyrewind(1); 
        //     1,3,5
        err = PARSER_CALL(Entity);//  Entity   1,3,5
        switch (err) {
            case PARSE_ERR:
            {
                RETURN_PARSE_ERROR;
            }
            break;
            case PARSE_SUCCESS://  Entity   1,3,5
            {
                CHECK_FOR_EOL;  // Entity    5 Done
                //  Entity   1,3
                err = PARSER_CALL(Mop); //  Entity  Mop      1,3
                switch (err) {
                    case PARSE_ERR:
                    {
                         RETURN_PARSE_ERROR;
                    }
                    break;
                    case PARSE_SUCCESS:
                    {
                        token_code = cyylex();
                        if (token_code == BRACK_START) {
                            //  Entity  Mop  (    3
                            err = PARSER_CALL(MExpr); //  Entity  Mop  (MExpr    3
                            switch (err) {
                                case PARSE_ERR:
                                {
                                    RETURN_PARSE_ERROR;
                                }
                                break;
                                case PARSE_SUCCESS:
                                {
                                    token_code = cyylex();
                                    if (token_code != BRACK_END) {
                                        RETURN_PARSE_ERROR;
                                    }
                                    CHECK_FOR_EOL;   //  Entity  Mop  (MExpr)    3 Done
                                    RETURN_PARSE_ERROR;
                                }
                                break;
                            }
                        }
                        else {
                            yyrewind(1);
                              //  Entity  Mop      1
                              err = PARSER_CALL(MExpr); //  Entity  Mop  MExpr    1
                              switch (err) {
                                case PARSE_ERR:
                                {
                                    RETURN_PARSE_ERROR;
                                }
                                break;
                                case PARSE_SUCCESS:
                                {
                                     CHECK_FOR_EOL;   //  Entity  Mop  MExpr    1 Done
                                     RETURN_PARSE_ERROR;
                                }
                                break;
                              }
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    assert(err == PARSE_SUCCESS);
    RETURN_PARSE_SUCCESS;
}



static parse_rc_t
WExpr (int *t) {

    parse_rc_t err = PARSE_SUCCESS;
    parse_init();

    int token_code = cyylex();
    
    if (token_code == BRACK_START) {
        // (     1   3   5
        err = PARSER_CALL(WExpr);  //   (WExpr     1
        switch (err) {     //   (WExpr     1
            case PARSE_ERR:
            {
                 err = PARSER_CALL(MExpr);  //   (MExpr     3    5
                 switch(err) {   //   (MExpr     3    5
                    case PARSE_ERR:
                    {
                            RETURN_PARSE_ERROR;
                    }
                    break;
                    case PARSE_SUCCESS:
                    {
                        token_code = cyylex();
                        if (token_code != BRACK_END) {
                            RETURN_PARSE_ERROR;
                        }
                        //   (MExpr)     3    5
                        err = PARSER_CALL(Ineq);    //   (MExpr)    INEQ              3    5
                        switch(err) {
                            case PARSE_ERR:
                            {
                                RETURN_PARSE_ERROR;
                            }   
                            break;
                            case PARSE_SUCCESS:   //   (MExpr)    INEQ              3    5
                            {
                                token_code = cyylex();
                                if (token_code == BRACK_START) {
                                     //   (MExpr)    INEQ   (           5
                                     err = PARSER_CALL(MExpr); //   (MExpr)    INEQ   (MExpr           5
                                     switch(err) {
                                        case PARSE_ERR:
                                        {
                                             RETURN_PARSE_ERROR;
                                        }
                                        break;
                                        case PARSE_SUCCESS:
                                        {
                                            token_code = cyylex();
                                            if (token_code != BRACK_END) {
                                                RETURN_PARSE_ERROR;
                                            }
                                            CHECK_FOR_EOL; //   (MExpr)    INEQ   (MExpr)           5 Done !
                                            RETURN_PARSE_ERROR;
                                        }
                                        break;
                                     }
                                }
                                else {
                                    yyrewind(1);
                                    err = PARSER_CALL(MExpr);    //   (MExpr)    INEQ   MExpr           3
                                    switch(err) {
                                        case PARSE_ERR:
                                        {
                                            RETURN_PARSE_ERROR;
                                        }
                                        break;
                                        case PARSE_SUCCESS:
                                        {
                                            CHECK_FOR_EOL; //   (MExpr)    INEQ   MExpr           3 Done !
                                            RETURN_PARSE_ERROR;
                                        }
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                    }
                    break;
                 }
            }
            break;
            case PARSE_SUCCESS://   (WExpr     1
            {
                token_code = cyylex();
                if (token_code != BRACK_END) {
                    RETURN_PARSE_ERROR;
                }
                CHECK_FOR_EOL;  //   (WExpr)     1 Done 
                RETURN_PARSE_ERROR;
            }
            break;
        }   
    }

      //      2    4
    else {
        yyrewind(1);
        err = PARSER_CALL(MExpr);   // MExpr        2    4
        switch(err) {
            case PARSE_ERR:
            {
                RETURN_PARSE_ERROR;
            }
            break;
            case PARSE_SUCCESS:
            {
                err = PARSER_CALL(Ineq);  // MExpr  INEQ      2    4
                switch (err) {
                    case PARSE_ERR:
                    {
                        RETURN_PARSE_ERROR;
                    }
                    break;
                    case PARSE_SUCCESS:
                    {
                        token_code = cyylex();
                        if (token_code == BRACK_START) {
                            // MExpr  INEQ   (         4
                            err = PARSER_CALL(MExpr); // MExpr  INEQ   (MExpr         4
                            switch (err) {
                                case PARSE_ERR:
                                {
                                    RETURN_PARSE_ERROR;
                                }
                                break;
                                case PARSE_SUCCESS:
                                {
                                    token_code = cyylex();
                                    if (token_code != BRACK_END) {
                                         RETURN_PARSE_ERROR;
                                    }
                                    CHECK_FOR_EOL; // MExpr  INEQ   (MExpr)         4 Done
                                    RETURN_PARSE_ERROR;
                                }
                                break;
                            }
                        }
                        else {
                             // MExpr  INEQ           2
                             err = PARSER_CALL(MExpr);   // MExpr  INEQ    MExpr       2
                             switch (err) {
                                case PARSE_ERR:
                                {
                                    RETURN_PARSE_ERROR;
                                }
                                break;
                                case PARSE_SUCCESS:
                                {
                                     CHECK_FOR_EOL; // MExpr  INEQ  MExpr         2 Done
                                     RETURN_PARSE_ERROR;
                                }
                                break;
                             }
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    assert(err == PARSE_SUCCESS);
    RETURN_PARSE_SUCCESS;
}


int 
main (int argc, char **argv) {

    
   // strcpy (current_buffer , "(a + (b * c)) > 3\n");
    strcpy (current_buffer , "a > b\n");
    yy_scan_string (current_buffer);
    parse_init();

    parse_rc_t err = PARSE_SUCCESS;
    
    err = PARSER_CALL(WExpr);

    switch (err) {
        case PARSE_SUCCESS:
            printf ("String is Valid\n");
            break;
        case PARSE_ERR:
            printf ("String is Not-Valid\n");
            break;
        default: ;
    }
    pause();
    return 0;
}


#if 0
int 
main (int argc, char **argv) {

    strcpy (current_buffer , "123 + 45.34 > 123\n");
    yy_scan_string (current_buffer);
    int token_code = cyylex();
    printf ("token_code = %d, scanned input = %s, token_str = %s\n", token_code, yytext, token_str (token_code));
    yyrewind(1);
    token_code = cyylex();
    printf ("token_code = %d, scanned input = %s, token_str = %s\n", token_code, yytext, token_str (token_code));
    token_code = cyylex();
    printf ("token_code = %d, scanned input = %s, token_str = %s\n", token_code, yytext, token_str (token_code));
    token_code = cyylex();
    printf ("token_code = %d, scanned input = %s, token_str = %s\n", token_code, yytext, token_str (token_code));
    yyrewind(1);
    yyrewind(1);
    token_code = cyylex();
    printf ("token_code = %d, scanned input = %s, token_str = %s\n", token_code, yytext, token_str (token_code));
    return 0;
}
#endif