
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

1. Q  ->   E Ineq E  | ( Q )
2. E  ->   T E'
3. E'  ->  + T E' | - T E' |  $
4. T  ->   F T'
5. T' ->   * F T' |   / F T'  |  $
6. F  ->   ( E ) |  P ( E ) | INTEGER | DECIMAL | VAR | G ( E, E)
7. P -> sqrt | sqr | sin        // urinary fns
8. G -> max | min | pow   // binary functions 
*/

parse_rc_t Ineq () ;
parse_rc_t G ();
parse_rc_t P ();
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
        case SQL_LESS_THAN:
        case SQL_GREATER_THAN:
        case SQL_EQ:
        case SQL_NOT_EQ:
            break;
        default:
            RETURN_PARSE_ERROR;
    }

    assert(err == PARSE_SUCCESS);
    RETURN_PARSE_SUCCESS;
}

/* Parse the Binary Math functions */
parse_rc_t
G () {

     parse_init();

     token_code = cyylex();

     switch (token_code) {

        case SQL_MATH_MAX:
        case SQL_MATH_MIN:
        case SQL_MATH_POW:
            RETURN_PARSE_SUCCESS;
        default:
            RETURN_PARSE_ERROR;
     }

}

/* Parse the unary Math functions */
parse_rc_t
P () {

    parse_init();

    token_code = cyylex();

    switch (token_code) {

        case SQL_MATH_SQRT:
        case SQL_MATH_SQR:
        case SQL_MATH_SIN:
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
        case SQL_INTEGER_VALUE:
        case SQL_DOUBLE:
        case SQL_IDENTIFIER:
        case SQL_IDENTIFIER_IDENTIFIER:
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

        case SQL_MATH_MUL:
        case SQL_MATH_DIV:
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
        case SQL_MATH_PLUS:
        case SQL_MATH_MINUS:
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
    int chkp_initial;
    
    CHECKPOINT(chkp_initial);

    token_code = cyylex();

    while (token_code == BRACK_START) {

        err = PARSER_CALL(Q);
        if (err == PARSE_ERR) break;
        
        token_code = cyylex();
        if (token_code != BRACK_END) break;

        RETURN_PARSE_SUCCESS;   // Q -> (Q)
    }

    RESTORE_CHKP(chkp_initial);

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
