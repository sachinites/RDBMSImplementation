/*

< create_query > ::= create table <table-name> (   < columns >    )
< table-name > ::=  identifier
< columns > ::= <column> | <column> , <columns>
< column> ::= identifer <dtype> |   identifer <dtype> <primary key | not null >
< dtype> ::= varchar(<digits>) | int | float
< digits> := <digit> | <digit><digits>
< digit> ::= 0|1|2|3|4|5|6|7|8|9
*/
static parse_rc_t
create_q_parse_column (int *t, ast_node_t *table_name, int *last_token_code) {

    parse_init();

    ast_node_t *dtype_attr_pr_key;
    ast_node_t *dtype_attr_not_null;

    token_code = cyylex ();

    if (token_code != SQL_IDENTIFIER) {
        PARSER_LOG_ERR(token_code, SQL_IDENTIFIER);
        RETURN_PARSE_ERROR;
    }

    ast_node_t *col_name_node = (ast_node_t *)calloc (1, sizeof (ast_node_t ));
    col_name_node->entity_type = SQL_IDENTIFIER;
    col_name_node->u.identifier.ident_type = SQL_COLUMN_NAME;
    strncpy (col_name_node->u.identifier.identifier.name, yytext, sizeof (col_name_node->u.identifier.identifier.name));

    ast_add_child (table_name, col_name_node );

    token_code = cyylex ();

    if (!sql_valid_dtype (token_code)) {

        int i;
        for (i = SQL_DTYPE_FIRST + 1; i < SQL_DTYPE_MAX; i++) {
            PARSER_LOG_ERR(token_code, i);
        }
        RETURN_PARSE_ERROR; 
    }

    ast_node_t *dtype =  (ast_node_t *)calloc (1, sizeof (ast_node_t ));
    dtype->entity_type = SQL_DTYPE;
    dtype->u.dtype = token_code;

    ast_add_child (col_name_node , dtype);

    if (token_code == SQL_STRING) {
        token_code = cyylex ();
        if (token_code != BRACK_START) {
            PARSER_LOG_ERR(token_code, BRACK_START);
            RETURN_PARSE_ERROR; 
        }
        token_code = cyylex();
        if (token_code != SQL_INT) {
            PARSER_LOG_ERR(token_code, SQL_INT);
            RETURN_PARSE_ERROR; 
        }        
        int a = atoi (yytext);
        if (a >= 256) {
            printf ("\nError : VARCHAR max size supported is 255\n");
            RETURN_PARSE_ERROR; 
        }

        ast_node_t *dtype_attr_len =  (ast_node_t *)calloc (1, sizeof (ast_node_t ));
        dtype_attr_len->entity_type = SQL_DTYPE_ATTR;
        dtype_attr_len->u.dtype_attr = SQL_DTYPE_LEN;

        ast_add_child (dtype, dtype_attr_len);

        ast_node_t *dtype_attr_len_value =  (ast_node_t *)calloc (1, sizeof (ast_node_t ));
        dtype_attr_len_value->entity_type = SQL_IDENTIFIER;
        dtype_attr_len_value->u.identifier.ident_type = SQL_INTEGER_VALUE;
        memcpy (dtype_attr_len_value->u.identifier.identifier.name, &a, sizeof (a));

         ast_add_child (dtype_attr_len, dtype_attr_len_value);

        token_code = cyylex ();
        if (token_code != BRACK_END) {
            PARSER_LOG_ERR(token_code, BRACK_END);
            RETURN_PARSE_ERROR; 
        }
    }

    token_code = cyylex ();
    *last_token_code = token_code;

    switch (token_code) {

        case SQL_PRIMARY_KEY:
            dtype_attr_pr_key =  (ast_node_t *)calloc (1, sizeof (ast_node_t ));
            dtype_attr_pr_key->entity_type = SQL_DTYPE_ATTR;
            dtype_attr_pr_key->u.dtype_attr = SQL_DTYPE_PRIMARY_KEY;
            ast_add_child (dtype, dtype_attr_pr_key);
            RETURN_PARSE_SUCCESS; 
            
        case SQL_NOT_NULL:
            dtype_attr_not_null =  (ast_node_t *)calloc (1, sizeof (ast_node_t ));
            dtype_attr_pr_key->entity_type = SQL_DTYPE_ATTR;
            dtype_attr_pr_key->u.dtype_attr = SQL_DTYPE_NOT_NULL ;
            ast_add_child (dtype, dtype_attr_not_null);
            RETURN_PARSE_SUCCESS; 

        case BRACK_END:
            RETURN_PARSE_SUCCESS; 
        case COMMA:
            RETURN_PARSE_SUCCESS; 
            break;
        default :
            PARSER_LOG_ERR(token_code, SQL_PRIMARY_KEY);
            PARSER_LOG_ERR(token_code, SQL_NOT_NULL);
            PARSER_LOG_ERR(token_code, BRACK_END);
            PARSER_LOG_ERR(token_code, COMMA);
            *last_token_code = 0;
            RETURN_PARSE_ERROR; 
    }

    RETURN_PARSE_SUCCESS; 
}

static parse_rc_t
create_q_parse_table_name ( int *t, ast_node_t *create_kw) {

    parse_init ();

    token_code = cyylex();

    switch (token_code) {
        case SQL_IDENTIFIER:
        break;
        default:
            RETURN_PARSE_ERROR;
    }

    ast_node_t *tble_name_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    tble_name_node->entity_type = SQL_IDENTIFIER;
    tble_name_node->u.identifier.ident_type = SQL_TABLE_NAME;
    strncpy(tble_name_node->u.identifier.identifier.name, yytext, sizeof (tble_name_node->u.identifier.identifier.name));
    ast_add_child (create_kw, tble_name_node);
    
    RETURN_PARSE_SUCCESS;
}

static parse_rc_t
parse_create_query(int *t,  ast_node_t *create_kw) {
    
    parse_init();

    int last_token_code;
    
    err  = create_q_parse_table_name (&lexc, create_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    token_code = cyylex();

    if (token_code != BRACK_START) {
        PARSER_LOG_ERR(token_code, BRACK_START);
        RETURN_PARSE_ERROR;
    }

    while (1) {

        err =  create_q_parse_column (&lexc, create_kw->child_list, &last_token_code);
        if (err == PARSE_ERR) RETURN_PARSE_ERROR;

        if (last_token_code == BRACK_END) RETURN_PARSE_SUCCESS;
        if (last_token_code == COMMA) continue;
        if (last_token_code == SQL_PRIMARY_KEY || last_token_code == SQL_NOT_NULL) {
            token_code = cyylex();
            switch (token_code) {
                case COMMA:
                    continue;
                case BRACK_END:
                    RETURN_PARSE_SUCCESS;
                default:
                    PARSER_LOG_ERR(token_code, COMMA);
                    PARSER_LOG_ERR(token_code, BRACK_END);
                    RETURN_PARSE_ERROR;
            }
        }
    }
    
    RETURN_PARSE_SUCCESS;
}

