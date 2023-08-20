/**
 *  insert into <table name> values (val1, val2, 'val3', ......, valn)
 * <InsertQuery> ::= insert into <table-name> values ( <value-list>)
 * <table-name> ::= <identifier>
 * <value-list> ::= value | value , <value-list>
 * <value> ::= <digits> | 'identifier' 
 * <digits> ::= <digit> | <digit> <digits>
 * <digit> ::= 0|1|2|3|4|5|6|7|8|9
 * <identifier> ::= [a-zA-Z0-9]
*/

/* val1 | 'val1' */
static int 
 insert_q_parse_value (ast_node_t *table_name_node) {

    int32_t a;
    int token_code;

     ast_node_t *astnode = NULL;

    token_code = yylex();
    switch (token_code) {

        case INTEGER:
            a = atoi (yytext);
            astnode = (ast_node_t *)calloc (1, sizeof (ast_node_t));
            astnode->entity_type =  SQL_IDENTIFIER;
            astnode->u.identifier.ident_type = SQL_INTEGER_VALUE;
            memcpy (astnode->u.identifier.identifier.name, &a, sizeof (a));
            ast_add_child (table_name_node , astnode);
            break;
        case QUOTATION_MARK:
            token_code = yylex();
            switch (token_code) {
                case SQL_STRING_VALUE: /* Supports strings with spaces in between*/
                case SQL_IDENTIFIER: /* Supports single words without spaces */
                    astnode = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                    astnode->entity_type =  SQL_IDENTIFIER;
                    astnode->u.identifier.ident_type = SQL_STRING_VALUE;
                    memcpy (astnode->u.identifier.identifier.name, yytext, sizeof (astnode->u.identifier.identifier.name));
                    ast_add_child (table_name_node , astnode);
                    token_code = yylex();
                    if (token_code != QUOTATION_MARK) {
                        PARSER_ERROR_EXIT(token_code, QUOTATION_MARK);
                    }
                break;
                case SQL_IPV4_ADDR_VALUE:
                    astnode = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                    astnode->entity_type =  SQL_IDENTIFIER;
                    astnode->u.identifier.ident_type = SQL_IPV4_ADDR_VALUE;
                    memcpy (astnode->u.identifier.identifier.name, yytext, sizeof (astnode->u.identifier.identifier.name));
                    ast_add_child (table_name_node , astnode);
                    token_code = yylex();
                    if (token_code != QUOTATION_MARK) {
                        PARSER_ERROR_EXIT(token_code, QUOTATION_MARK);
                    }
                    break;
                    default:
                        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
                        break;
            }
            break;
            case DECIMAL_NUMBER:
                exit(0);
                break;
            default: 
                PARSER_ERROR_EXIT(0, token_code);
                break;
    }

    return yylex();
 }

/* val1, val2, 'val3', ......, valn) */
static int 
insert_q_parse_value_list (ast_node_t *table_name_node) {

    int token_code;

    while (1) {

        token_code = insert_q_parse_value (table_name_node);

        switch (token_code) {
            case COMMA:
                break;
            default:
                return token_code;
        }
    }
    return token_code;
}

static int 
insert_q_parse_table_name (ast_node_t *insert_kw) {

    int token_code = yylex();
    if (token_code != SQL_IDENTIFIER) {
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    }
   // printf ("Table Name = %s\n", yytext);

    ast_node_t *tble_name_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    tble_name_node->entity_type = SQL_IDENTIFIER;
    tble_name_node->u.identifier.ident_type = SQL_TABLE_NAME;
    strncpy(tble_name_node->u.identifier.identifier.name, yytext, sizeof (tble_name_node->u.identifier.identifier.name));
    ast_add_child (insert_kw, tble_name_node);

    return yylex();
}

static void 
parse_insert_query ( ast_node_t *insert_kw) {
    
    int token_code = insert_q_parse_table_name (insert_kw);

    if (strcmp (yytext, "values")) {
        printf ("'Error : 'values' keyword missing\n");
        PARSER_ERROR_EXIT(token_code, 0);
    }

    token_code = yylex();

    if (token_code != BRACK_START) {
         PARSER_ERROR_EXIT(token_code, BRACK_START);
    }

    token_code =  insert_q_parse_value_list (insert_kw->child_list);

    switch (token_code) {

        case BRACK_END:
            token_code = yylex();
            if (token_code != EOL) {
                PARSER_ERROR_EXIT(token_code, EOL);
            }
            break;
        default:
            PARSER_ERROR_EXIT(token_code, BRACK_END);
            break;
    }
}