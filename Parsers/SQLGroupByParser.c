
static int
parse_group_by_query (ast_node_t *select_root) {

    int token_code;
    ast_node_t *col_node;
    ast_node_t *group_by_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    group_by_node->entity_type = SQL_KEYWORD;
    group_by_node->u.identifier.ident_type = SQL_GROUP_BY;
    ast_add_child(select_root, group_by_node);

    token_code = yylex();

    switch(token_code) {

        case SQL_IDENTIFIER:
        case SQL_TABLE_COLMN_NAME:
            col_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
            col_node->entity_type = SQL_IDENTIFIER;
            col_node->u.identifier.ident_type = SQL_COLUMN_NAME;
            strncpy (col_node->u.identifier.identifier.name, yytext, 
                sizeof (col_node->u.identifier.identifier.name));
            ast_add_child (group_by_node, col_node);
            token_code = yylex();
            if (token_code == COMMA) {
                token_code = parse_plain_coumns(select_root);
            }
        break;
    }
    if (token_code == EOL) return SQL_PARSE_OK;
    return token_code;
} 