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
create_q_parse_columns (ast_node_t *table_name) {

    int token_code ;

    token_code = yylex ();
    if (token_code != SQL_IDENTIFIER) {
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    }
    printf ("col name = %s ", yytext);

    ast_node_t *col_name_node = (ast_node_t *)calloc (1, sizeof (ast_node_t ));
    col_name_node->entity_type = SQL_IDENTIFIER;
    col_name_node->u.identifier.ident_type = SQL_COLUMN_NAME;
    strncpy (col_name_node->u.identifier.identifier.name, yytext, sizeof (col_name_node->u.identifier.identifier.name));

    ast_add_child (table_name, col_name_node );

    token_code = yylex ();
    if (!sql_valid_dtype (token_code)) {
        PARSER_ERROR_EXIT(token_code, 0);
    }
    printf ("whose dtype name = %s (%d) ", yytext, token_code);

    ast_node_t *dtype =  (ast_node_t *)calloc (1, sizeof (ast_node_t ));
    dtype->entity_type = SQL_DTYPE;
    dtype->u.dtype = token_code;

    ast_add_child (col_name_node , dtype);

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

        ast_node_t *dtype_attr_len =  (ast_node_t *)calloc (1, sizeof (ast_node_t ));
        dtype_attr_len->entity_type = SQL_DTYPE_ATTR;
        dtype_attr_len->u.dtype_attr = SQL_DTYPE_LEN;

        ast_add_child (dtype, dtype_attr_len);

        ast_node_t *dtype_attr_len_value =  (ast_node_t *)calloc (1, sizeof (ast_node_t ));
        dtype_attr_len_value->entity_type = SQL_IDENTIFIER;
        dtype_attr_len_value->u.identifier.ident_type = SQL_INTEGER_VALUE;
        memcpy (dtype_attr_len_value->u.identifier.identifier.name, &a, sizeof (a));

         ast_add_child (dtype_attr_len, dtype_attr_len_value);

        token_code = yylex ();
        if (token_code != BRACK_END) {
            PARSER_ERROR_EXIT(token_code, BRACK_END);
        }
    }
    token_code = yylex ();
    switch (token_code) {
        case SQL_PRIMARY_KEY:
            printf ("which is primary key ");
            ast_node_t *dtype_attr_pr_key =  (ast_node_t *)calloc (1, sizeof (ast_node_t ));
            dtype_attr_pr_key->entity_type = SQL_DTYPE_ATTR;
            dtype_attr_pr_key->u.dtype_attr = SQL_DTYPE_PRIMARY_KEY;
            ast_add_child (dtype, dtype_attr_pr_key);

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
            ast_node_t *dtype_attr_not_null =  (ast_node_t *)calloc (1, sizeof (ast_node_t ));
            dtype_attr_pr_key->entity_type = SQL_DTYPE_ATTR;
            dtype_attr_pr_key->u.dtype_attr = SQL_DTYPE_NOT_NULL ;
            ast_add_child (dtype, dtype_attr_not_null);

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

    return yylex();
}

static int
create_q_parse_table_name ( ast_node_t *create_kw) {

    int token_code = yylex();
    if (token_code != SQL_IDENTIFIER) {
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    }
    printf ("Table Name = %s\n", yytext);

    ast_node_t *tble_name_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    tble_name_node->entity_type = SQL_IDENTIFIER;
    tble_name_node->u.identifier.ident_type = SQL_TABLE_NAME;
    strncpy(tble_name_node->u.identifier.identifier.name, yytext, sizeof (tble_name_node->u.identifier.identifier.name));
    ast_add_child (create_kw, tble_name_node);

    token_code = yylex ();
    if (token_code != BRACK_START) {
        PARSER_ERROR_EXIT(token_code, BRACK_START);
    }

    return token_code;
}

static void 
parse_create_query( ast_node_t *create_kw) {
    
    int token_code = create_q_parse_table_name (create_kw);
    token_code =  create_q_parse_columns (create_kw->child_list);

    while (1) {
        switch (token_code) {
            case COMMA:
                printf ("\n");
                token_code = create_q_parse_columns(create_kw->child_list);
                break;
            case EOL:
                return;
        }
    }
}

