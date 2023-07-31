/*

< create_query > ::= create table <table-name> (   < columns >    )
< table-name > ::=  identifier
< columns > ::= <column> | <column> , <columns>
< column> ::= identifer <dtype> |   identifer <dtype> <primary key | not null >
< dtype> ::= varchar(<digits>) | int | float
< digits> := <digit> | <digit><digits>
< digit> ::= 0|1|2|3|4|5|6|7|8|9
*/

static int 
create_q_parse_columns () {

    int token_code ;

    token_code = yylex ();
    if (token_code != SQL_IDENTIFIER) {
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    }
    printf ("col name = %s ", yytext);
    token_code = yylex ();
    if (!sql_valid_dtype (token_code)) {
        PARSER_ERROR_EXIT(token_code, 0);
    }
    printf ("whose dtype name = %s (%d) ", yytext, token_code);

    if (token_code == SQL_STRING) {
        token_code = yylex ();
        if (token_code != BRACK_START) {
            PARSER_ERROR_EXIT(token_code, BRACK_START);
        }
        token_code = yylex();
        if (token_code != INTEGER) {
            PARSER_ERROR_EXIT(token_code, INTEGER);
        }        
        int a = atoi (yytext);
        if (a >= 256) {
            printf ("\nError : VARCHAR max size supported is 255\n");
            exit(0);
        }
        printf (" and varchar len is %d ", a);
        token_code = yylex ();
        if (token_code != BRACK_END) {
            PARSER_ERROR_EXIT(token_code, BRACK_END);
        }
    }
    token_code = yylex ();
    switch (token_code) {
        case SQL_PRIMARY_KEY:
            printf ("which is primary key ");
            token_code = yylex ();
             switch(token_code) {
                case COMMA:
                case  BRACK_END:
                    break;
                default:
                    PARSER_ERROR_EXIT(token_code, 0);
                    break;
             }
             break;
        case SQL_NOT_NULL:
             printf ("which is not null ");
             token_code = yylex ();
             switch(token_code) {
                case COMMA:
                case  BRACK_END:
                    break;
                default:
                    PARSER_ERROR_EXIT(token_code, 0);
                    break;
             }
             break;
        case COMMA:
        case  BRACK_END:
        break;
        default :
            PARSER_ERROR_EXIT(token_code, 0);
    }

    return token_code;
}

static int
create_q_parse_table_name () {

    int token_code = yylex();
    if (token_code != SQL_IDENTIFIER) {
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    }
    printf ("Table Name = %s\n", yytext);

    token_code = yylex ();
    if (token_code != BRACK_START) {
        PARSER_ERROR_EXIT(token_code, BRACK_START);
    }

    return token_code;
}

static void 
parse_create_query() {
    
    int token_code = create_q_parse_table_name ();
    token_code =  create_q_parse_columns ();

    while (1) {
        switch (token_code) {
            case COMMA:
                printf ("\n");
                token_code = create_q_parse_columns();
                break;
            case BRACK_END:
                printf ("\n");
                return;
        }
    }
}