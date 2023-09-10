
/* ALternate code to implement Create query which is written as per CFG*/

/* CFG : 

    Q -> MC
    M -> Identifier
    C -> (D)
    D -> D , E | E
        D -> E D'
        D' -> , E D' | epsilon
    E -> V T | V T P | V T N | ( E )
    V -> Identifier
    T -> Dtypes
    P -> primary key
    N - Not-NULL

*/

static parse_rc_t
CE (int *t, ast_node_t *table_name)  {

    parse_init ();

    token_code = cyylex();

    switch (token_code) {

        case SQL_IDENTIFIER:
            ast_node_t *col_name_node = (ast_node_t *)calloc(1, sizeof(ast_node_t));
            col_name_node->entity_type = SQL_IDENTIFIER;
            col_name_node->u.identifier.ident_type = SQL_COLUMN_NAME;
            strncpy(col_name_node->u.identifier.identifier.name, yytext, sizeof(col_name_node->u.identifier.identifier.name));
            ast_add_child(table_name, col_name_node);
            token_code = cyylex();
            if (!sql_valid_dtype(token_code)) {
                int i;
                for (i = SQL_DTYPE_FIRST + 1; i < SQL_DTYPE_MAX; i++) {
                    PARSER_LOG_ERR(token_code, i);
                }
                RETURN_PARSE_ERROR;
            }
            ast_node_t *dtype = (ast_node_t *)calloc(1, sizeof(ast_node_t));
            dtype->entity_type = SQL_DTYPE;
            dtype->u.dtype = token_code;
            ast_add_child(col_name_node, dtype);

            if (token_code == SQL_STRING)
            {
                token_code = cyylex();
                if (token_code != BRACK_START)
                {
                    PARSER_LOG_ERR(token_code, BRACK_START);
                    RETURN_PARSE_ERROR;
                }
                token_code = cyylex();
                if (token_code != SQL_INT)
                {
                    PARSER_LOG_ERR(token_code, SQL_INT);
                    RETURN_PARSE_ERROR;
                }
                int a = atoi(yytext);
                if (a >= 256)
                {
                    printf("\nError : VARCHAR max size supported is 255\n");
                    RETURN_PARSE_ERROR;
                }

                ast_node_t *dtype_attr_len = (ast_node_t *)calloc(1, sizeof(ast_node_t));
                dtype_attr_len->entity_type = SQL_DTYPE_ATTR;
                dtype_attr_len->u.dtype_attr = SQL_DTYPE_LEN;

                ast_add_child(dtype, dtype_attr_len);

                ast_node_t *dtype_attr_len_value = (ast_node_t *)calloc(1, sizeof(ast_node_t));
                dtype_attr_len_value->entity_type = SQL_IDENTIFIER;
                dtype_attr_len_value->u.identifier.ident_type = SQL_INTEGER_VALUE;
                memcpy(dtype_attr_len_value->u.identifier.identifier.name, &a, sizeof(a));

                ast_add_child(dtype_attr_len, dtype_attr_len_value);

                token_code = cyylex();
                if (token_code != BRACK_END)
                {
                    PARSER_LOG_ERR(token_code, BRACK_END);
                    RETURN_PARSE_ERROR;
                }
            }

            token_code = cyylex ();
            switch (token_code) {
                case SQL_PRIMARY_KEY:
                    ast_node_t *dtype_attr_pr_key = (ast_node_t *)calloc(1, sizeof(ast_node_t));
                    dtype_attr_pr_key->entity_type = SQL_DTYPE_ATTR;
                    dtype_attr_pr_key->u.dtype_attr = SQL_DTYPE_PRIMARY_KEY;
                    ast_add_child(dtype, dtype_attr_pr_key);
                    RETURN_PARSE_SUCCESS;
                    break;
                case SQL_NOT_NULL:
                    ast_node_t *dtype_attr_not_null = (ast_node_t *)calloc(1, sizeof(ast_node_t));
                    dtype_attr_pr_key->entity_type = SQL_DTYPE_ATTR;
                    dtype_attr_pr_key->u.dtype_attr = SQL_DTYPE_NOT_NULL;
                    ast_add_child(dtype, dtype_attr_not_null);
                    RETURN_PARSE_SUCCESS;
                    break;
                case BRACK_START:
                    err = CE(&lexc, table_name);
                    if (err == PARSE_ERR) RETURN_PARSE_ERROR;
                     token_code = cyylex ();
                     if (token_code != BRACK_END) {
                        PARSER_LOG_ERR(token_code, BRACK_END);
                        RETURN_PARSE_ERROR;
                     }
                     RETURN_PARSE_SUCCESS;
                default:
                    yyrewind(1);
            }
            break;

        case BRACK_START:
            err = CE(&lexc, table_name);
            if (err == PARSE_ERR) RETURN_PARSE_ERROR;
            token_code = cyylex ();
             if (token_code != BRACK_END) {
                PARSER_LOG_ERR(token_code, BRACK_END);
                RETURN_PARSE_ERROR;
            }
            RETURN_PARSE_SUCCESS;

        break;
        default:
            PARSER_LOG_ERR(token_code, SQL_IDENTIFIER);
            PARSER_LOG_ERR(token_code, BRACK_START);
            RETURN_PARSE_ERROR;
    }

    RETURN_PARSE_SUCCESS;
}


static parse_rc_t
CD_dash (int *t, ast_node_t *table_name)  {

    parse_init();

    token_code = cyylex();

    switch (token_code) {

        case COMMA:
            err = CE(&lexc, table_name);
            switch (err) {
                case PARSE_ERR:
                    yyrewind(1);
                    RETURN_PARSE_SUCCESS;
                case PARSE_SUCCESS:
                    err = CD_dash (&lexc, table_name);
                    switch (err) {
                        case PARSE_ERR:
                             yyrewind(1);
                            RETURN_PARSE_SUCCESS;
                        case PARSE_SUCCESS:
                            RETURN_PARSE_SUCCESS;
                    }
            }
        break;
        default:
            yyrewind(1);
            RETURN_PARSE_SUCCESS;
    }
}



static parse_rc_t
CD (int *t, ast_node_t *table_name) {

    parse_init ();

    err = CE(&lexc, table_name);

    if (err == PARSE_ERR)  RETURN_PARSE_ERROR;

    err = CD_dash(&lexc, table_name);

    if (err == PARSE_ERR)  RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS;
}


static parse_rc_t
CC (int *t, ast_node_t *table_name) {

    parse_init ();

    token_code = cyylex();

    if (token_code != BRACK_START) {
        PARSER_LOG_ERR(token_code, BRACK_START);
        RETURN_PARSE_ERROR;
    }

    err = CD (&lexc, table_name);

    if (err == PARSE_ERR)  RETURN_PARSE_ERROR;

    token_code = cyylex();

    if (token_code != BRACK_END) {
        PARSER_LOG_ERR(token_code, BRACK_END);
        RETURN_PARSE_ERROR;
    }

    RETURN_PARSE_SUCCESS;
}

static parse_rc_t
CM (int *t, ast_node_t *create_kw) {

    parse_init ();

    token_code = cyylex();

    switch (token_code) {
        case SQL_IDENTIFIER:
        break;
        default:
            PARSER_LOG_ERR(token_code, SQL_IDENTIFIER);
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
CQ (int *t, ast_node_t *create_kw) {

    parse_init();

    err = CM(&lexc, create_kw);

    if (err == PARSE_ERR)  RETURN_PARSE_ERROR;

    err = CC(&lexc, create_kw->child_list);

    if (err == PARSE_ERR)  RETURN_PARSE_ERROR;
    RETURN_PARSE_SUCCESS;
}

static parse_rc_t
parse_create_query_cfg(int *t, ast_node_t *create_kw) {

    parse_init ();

    yy_scan_string (lex_buffer);
    token_code = cyylex(); // consume 'create table'

    err = CQ(&lexc, create_kw);

    if (err == PARSE_ERR)  RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS;
}