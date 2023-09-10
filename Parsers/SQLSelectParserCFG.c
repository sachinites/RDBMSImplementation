
/* CFG 

Q -> select COLS from TABS WHERE GRPBY ORDERBY LMT

COLS -> ME L | ME L , COLS

TABS -> <ident> L | <ident> L , TABS

L -> $ | as <identifer>

WHERE -> $ | where A

A-> INEQ | B LOP A 

B -> INEQ

LOP -> and  |  or

INEQ -> ME  INS  ME

INS ->  <   |   >    |   =    |     !=   

GRPBY ->  $  |  group by COLS HAVING

HAVING  ->  $  |  having A

ORDERBY  -> $  |  order by ME C

C ->  asc |  dsc

LMT  -> $  |  limit <integer>

*/

static parse_rc_t
LMT (int *t, ast_node_t *select_kw) ;
static parse_rc_t
ORDER_BY (int *t, ast_node_t *select_kw) ;
static parse_rc_t
HAVING (int *t, ast_node_t *select_kw);
static parse_rc_t
GROUPBY (int *t, ast_node_t *select_kw);
static parse_rc_t
B (int *t, ast_node_t *select_kw);
static parse_rc_t
A (int *t, ast_node_t *select_kw);
static parse_rc_t
WHERE (int *t, ast_node_t *select_kw) ;
static parse_rc_t
TABS (int *t, ast_node_t *select_kw) ;
static parse_rc_t
L (int *t, ast_node_t *select_kw);
static parse_rc_t
COLS (int *t, ast_node_t *select_kw);
static parse_rc_t
SEL_Q (int *t, ast_node_t **select_root);



parse_rc_t
LMT (int *t, ast_node_t *select_kw) {

     parse_init ();

     token_code = cyylex();

     if (token_code != SQL_LIMIT) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;
     }

     token_code = cyylex();

     if (token_code != SQL_INT) {
        RETURN_PARSE_ERROR;
     }

     RETURN_PARSE_SUCCESS;
}


parse_rc_t
ORDER_BY (int *t, ast_node_t *select_kw) {

    parse_init ();

    token_code = cyylex();

    if (token_code != SQL_ORDER_BY) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;
    }

    err = E(&lexc);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    token_code = cyylex();

    if (token_code != SQL_ORDERBY_ASC &&
            token_code != SQL_ORDERBY_DSC) {

        yyrewind(1);
        RETURN_PARSE_SUCCESS;   // assume it is ASC when not specified
    }

    RETURN_PARSE_SUCCESS;
}

parse_rc_t
HAVING (int *t, ast_node_t *select_kw) {

    parse_init ();

    token_code = cyylex();

    if (token_code != SQL_HAVING) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;
    }

    err = A (&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS;
}

parse_rc_t
GROUPBY (int *t, ast_node_t *select_kw) {

    parse_init();

    token_code = cyylex();

    if (token_code != SQL_GROUP_BY) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;
    }

    err = COLS (&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    err = HAVING (&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS;
}

parse_rc_t
L (int *t, ast_node_t *select_kw) {

    parse_init ();

    token_code = cyylex();

    if (token_code != SQL_AS) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;
    }

    token_code = cyylex();
    if (token_code != SQL_IDENTIFIER) RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS;
}

parse_rc_t
B (int *t, ast_node_t *select_kw) {

    parse_init();

    err = Q(&lexc);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS;
}

parse_rc_t
A (int *t, ast_node_t *select_kw) {

    parse_init ();
    
    int chkp_pre_b;
    CHECKPOINT(chkp_pre_b);
    
    err = B(&lexc, NULL);

    switch (err) {

        case PARSE_ERR:
            err = Q(&lexc);
            switch (err) {
                case PARSE_ERR:
                    RETURN_PARSE_ERROR;
                break;
                case PARSE_SUCCESS:
                    RETURN_PARSE_SUCCESS; // A-> INEQ 
                break;
            }
        break;
        case PARSE_SUCCESS:
            token_code = cyylex();
            switch (token_code) {
                case SQL_AND:
                case SQL_OR:
                    err = A (&lexc, select_kw);
                    switch (err) {
                        case PARSE_ERR:
                            RESTORE_CHKP(chkp_pre_b);
                            err = Q(&lexc);
                            if (err == PARSE_ERR) RETURN_PARSE_ERROR;
                            RETURN_PARSE_SUCCESS; 
                        break;
                        case PARSE_SUCCESS:
                            RETURN_PARSE_SUCCESS; // A-> B LOP A 
                        break;
                    }
                break;
                default:
                    RESTORE_CHKP(chkp_pre_b);
                    err = Q(&lexc);
                    if (err == PARSE_ERR) RETURN_PARSE_ERROR;
                    RETURN_PARSE_SUCCESS; 
            }
        break;
    }
    RETURN_PARSE_SUCCESS; 
}

parse_rc_t
WHERE (int *t, ast_node_t *select_kw) {

    parse_init ();

    token_code = cyylex();

    if (token_code != SQL_WHERE) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;    // WHERE -> $
    }

    err = A(&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS; // WHERE -> where A
}


parse_rc_t
TABS (int *t, ast_node_t *select_kw) {

    parse_init ();

    token_code = cyylex();

    if (token_code != SQL_IDENTIFIER) {
        RETURN_PARSE_ERROR;
    }

    err = L(&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    token_code = cyylex();

    if (token_code != COMMA) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;  // TABS -> <ident>
    }

    err = TABS(&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;
    RETURN_PARSE_SUCCESS; // TABS -> <ident> , TABS
}

parse_rc_t
COLS (int *t, ast_node_t *select_kw) {

    parse_init();

    err = E(&lexc);
    
    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    err = L(&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    token_code = cyylex ();

    if (token_code != COMMA) {
        yyrewind(1);
        RETURN_PARSE_SUCCESS;  // COLS -> ME 
    }

    err = COLS (&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS; // COLS -> ME , COLS
}

parse_rc_t
SEL_Q (int *t, ast_node_t **select_root) {

    parse_init();

    token_code = cyylex();

    if (token_code != SQL_SELECT_Q) {
        PARSER_LOG_ERR(token_code, SQL_SELECT);
        RETURN_PARSE_ERROR;
    }

    *select_root = (ast_node_t *)calloc(1, sizeof(ast_node_t));
    (*select_root)->entity_type = SQL_QUERY_TYPE;
    (*select_root)->u.q_type = SQL_SELECT_Q;

    ast_node_t *select_kw = (ast_node_t *)calloc(1, sizeof(ast_node_t));
    select_kw->entity_type = SQL_KEYWORD;
    select_kw->u.kw = SQL_SELECT;

    ast_add_child(*select_root, select_kw);

    err = COLS (&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    token_code = cyylex();

    if (token_code != SQL_FROM) RETURN_PARSE_ERROR;

    err = TABS(&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    err = WHERE(&lexc, select_kw);

     if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    err = GROUPBY(&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    err = ORDER_BY(&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    err = LMT(&lexc, select_kw);

    if (err == PARSE_ERR) RETURN_PARSE_ERROR;

    /* Query Must be completely consumed*/
    token_code = cyylex();
    if (token_code != SEMI_COLON) RETURN_PARSE_ERROR;

    RETURN_PARSE_SUCCESS;
}

static  ast_node_t *
parse_select_query_cfg () {

    parse_init ();

    ast_node_t *select_root = NULL;

    err = SEL_Q (&lexc, &select_root);

    if (err == PARSE_ERR) {
        ast_destroy_tree_from_root(select_root);
        select_root = NULL;
        printf ("Query failed\n");
    }
    else {
        printf ("Success\n");
    }

    return select_root;
}