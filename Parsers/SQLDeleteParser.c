
int
parse_delete_query (ast_node_t *delete_root) {

    int token_code;

    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, SQL_FROM);
    ast_node_t *from_kw = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    from_kw->entity_type = SQL_KEYWORD;
    from_kw->u.kw = SQL_FROM;
    ast_add_child (delete_root, from_kw);
    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    ast_node_t *tble_name_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    tble_name_node->entity_type = SQL_IDENTIFIER;
    tble_name_node->u.identifier.ident_type = SQL_TABLE_NAME;
    strncpy(tble_name_node->u.identifier.identifier.name, yytext, sizeof (tble_name_node->u.identifier.identifier.name));
    ast_add_child (from_kw, tble_name_node);
    token_code = yylex();
    if (token_code == EOL) return SQL_PARSE_OK;
    PARSER_ERROR_EXIT(token_code, SQL_WHERE);
    ast_node_t *where_kw =  (ast_node_t *)calloc (1, sizeof (ast_node_t));
    where_kw->entity_type = SQL_KEYWORD;
    where_kw->u.kw = SQL_WHERE;
    ast_add_child (delete_root, where_kw);
    token_code = parse_where_conditions(where_kw);
    if (token_code != EOL) {
        PARSER_ERROR_EXIT(token_code, EOL);
    }
    return SQL_PARSE_OK;
}