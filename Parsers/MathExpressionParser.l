/* This file implements the SQL Parser which accespts all SQL queries satisfying the below Grammar 

A Grammar to parse mathematical expression !

WExpr ::= MExpr INEQ  MExpr |    (MExpr) INEQ  MExpr      |       MExpr INEQ  (MExpr)          |    (MExpr) INEQ  (MExpr)
INEQ  ::= < | > | = | != 
MExpr ::= Entity Mop MExpr  |    (Entity) Mop MExpr       |       Entity Mop (MExpr)           |    (Entity) Mop (MExpr)
Entity ::= VAR | RE
VAR ::= word | word.word
RE ::= ^[1-9][0-9]*  |   [0-9]*.[0-9]+ 
word ::= \w
Mop ::= + | - | * | /

*/


%{

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#define VAR    1
#define VAR_VAR 2
#define INTEGER 3
#define RE  4
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
#define Error 17
#define Success 18

const char *token_str (int token_code) {

    switch (token_code) {

        case VAR:   return "VAR";
        case VAR_VAR: return "VAR_VAR";
        case INTEGER: return "INTEGER";
        case RE: return "RE";
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

[a-zA-Z0-9]\.[a-zA-Z0-9] {
    return VAR_VAR;
}

^[1-9][0-9]*?[0-9]*.[0-9]+ {

    return RE;
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
}

[\t] {
    /*ignore*/
}

"\\q" {
    return QUIT;
}

. {
    printf ("Non parsable character : %c\n", *yytext);
}

%%

static int paranthesis_cnt = 0;
static int pos = 0;

static int 
INEQ() {

    return Success;
}

static int
MExpr() {

    return Success;
}

static int
WExpr () {

    int token_code;

    do {
        token_code = MExpr();
        if (token_code = Error) 
    while(0);

    return Success;
}



int 
main (int argc, char **argv) {

    int token_code = WExpr();
    if (token_code == Success) printf ("Accepted\n") else printf ("Rejected\n");
    return 0;
}