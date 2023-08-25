
bool
parse_delete_query (ast_node_t *delete_root) {

    int token_code;

    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, SQL_FROM);
    token_code = yylex();
    PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    ast_node_t *tble_name_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    tble_name_node->entity_type = SQL_IDENTIFIER;
    tble_name_node->u.identifier.ident_type = SQL_TABLE_NAME;
    strncpy(tble_name_node->u.identifier.identifier.name, yytext, sizeof (tble_name_node->u.identifier.identifier.name));
    ast_add_child (delete_root, tble_name_node);
    token_code = yylex();
    if (token_code == EOL) return true;
    PARSER_ERROR_EXIT(token_code, SQL_WHERE);
    token_code = parse_where_conditions(delete_root);
    if (token_code != EOL) {
        PARSER_ERROR_EXIT(token_code, EOL);
    }
    return true;
}