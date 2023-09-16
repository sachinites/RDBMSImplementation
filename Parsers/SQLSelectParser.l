%{

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <arpa/inet.h>

int yylex();

typedef enum parse_rc_ {

    PARSE_ERR,
    PARSE_SUCCESS

} parse_rc_t;


#define MAX_STRING_SIZE 512

static unsigned char lex_buffer[MAX_STRING_SIZE] = {0};
static unsigned char *curr_ptr = lex_buffer;

typedef uint8_t * (*parser_alloc_token_value_fn_ptr_t) (uint16_t );

#include "ParserExport.h"

typedef struct stack {

    int top;
    lex_data_t data[MAX_MEXPR_LEN];
} stack_t;

static stack_t undo_stack = {-1, {0, 0, 0}};

static void 
push(lex_data_t lex_data) {
    assert (undo_stack.top < MAX_MEXPR_LEN -1);
    undo_stack.data[++undo_stack.top] = lex_data;
}

static lex_data_t
pop() {
    assert (undo_stack.top > -1);
    lex_data_t res = undo_stack.data[undo_stack.top] ;
    undo_stack.top--;
    return res;
}

static void 
yyrewind (int n) {

    if (n <= 0) return;
    if (curr_ptr == lex_buffer) return;
    int data_len = 0;
    lex_data_t lex_data;
    while (n)  {
        lex_data = pop();
        data_len += lex_data.token_len;
        n--;
        lex_data.token_code = 0;
        lex_data.token_len = 0;
        if (lex_data.token_val) {
            free (lex_data.token_val);
            lex_data.token_val = NULL;
        }
    }
    curr_ptr -= data_len;
    yy_scan_string(curr_ptr);
}

static unsigned char *
parser_alloc_token_value_default (uint16_t token_id) {

    unsigned char *ptr = (unsigned char *)(calloc (1, yyleng + 1));
    strncpy (ptr, yytext, yyleng);
    ptr [yyleng] = '\0';
    return (void *)ptr;
}

parser_alloc_token_value_fn_ptr_t parser_alloc_token_value_fn_ptr =
    parser_alloc_token_value_default ;

static int 
cyylex () {

    int token_code =  yylex();
    curr_ptr += yyleng;
    lex_data_t lex_data;
    lex_data.token_code = token_code;
    lex_data.token_len = yyleng;
    lex_data.token_val = parser_alloc_token_value_fn_ptr (token_code);
    push(lex_data);
    return token_code;
}

static void 
process_white_space(int n) {

    lex_data_t lex_data;
    curr_ptr += n;
    lex_data.token_code = 0;
    lex_data.token_len = n;
    lex_data.token_val = NULL;
    push(lex_data);
}

#define parse_init()             \
    int token_code = 0;          \
    int _lchkp = undo_stack.top;    \
    parse_rc_t err = PARSE_SUCCESS

#define RETURN_PARSE_ERROR      \
    {yyrewind(undo_stack.top - _lchkp);     \
    return PARSE_ERR;}

#define RETURN_PARSE_SUCCESS    \
    return PARSE_SUCCESS

#define PARSER_CALL(fn) \
    fn(NULL)

#define CHECKPOINT(a)    \
    a = undo_stack.top

#define RESTORE_CHKP(a) \
    yyrewind(undo_stack.top - a)
    
#define CHECK_FOR_EOL                \
    {token_code = cyylex();                   \
    if (token_code == EOL) {                \
        RETURN_PARSE_SUCCESS;   \
    }}

static inline int
cyylexlh() {

    int token_code = cyylex();
    yyrewind(1);
    return token_code;
}

static inline int
cyylexlb() {

    yyrewind(1);
    int token_code = cyylex();
    yyrewind(1);
    return token_code;
}

#define PARSER_LOG_ERR(token_obtained, expected_token)  \
    printf ("%s(%d) : Token Obtained = %d (%s) , expected token = %d\n",    \
        __FUNCTION__, __LINE__, token_obtained, yytext, expected_token);

void 
Parser_init (parser_alloc_token_value_fn_ptr_t fn_ptr) {

    parser_alloc_token_value_fn_ptr = fn_ptr;
}

void 
Parser_stack_reset () {

    int i;
    lex_data_t *lex_data;

    for (i = 0; i < undo_stack.top; i++) {
        lex_data = &undo_stack.data[i];
        lex_data->token_code = 0;
        lex_data->token_len = 0;
        if (lex_data->token_val) {
            free (lex_data->token_val);
            lex_data->token_val = NULL;
        }
    }
    undo_stack.top = -1;
    curr_ptr = lex_buffer;
}

int 
Parser_get_current_stack_index () {
    return undo_stack.top;
}

#define ITERATE_LEX_STACK_BEGIN(i , j , token_code_, len_, value_)    \
{   int _k;                                                                                                              \
     for (_k = i; _k <= j && _k <= undo_stack.top; _k++) {                               \
     lex_data_t *lex_data = &undo_stack.data[_k];  \
     if (lex_data->token_code == 0 || lex_data->token_code == WHITE_SPACE) continue; \
     token_code_ = lex_data->token_code;               \
     len_ = lex_data->token_len;                               \
     value_ = lex_data->token_val;

#define ITERATE_LEX_STACK_END }}


/* ========================================
            Warning : Do  Not Modify this file above this line 
    ======================================= */
            /* User specific parsing logic starts below */



/* User specific header files */
#include "SQLParserStruct.h"
#include "common.h"
#include "Ast.h"
#include "../core/sql_const.h"
#include "../core/sql_where.h"
#include "../core/Catalog.h"
#include "../core/sql_utils.h"
#include "MExpr.h"

static uint8_t *
parser_alloc_token_value (u_int16_t token_code) {

    uint8_t *token_buffer = NULL;
    
    switch (token_code) {

        case SQL_IDENTIFIER:
            token_buffer = (uint8_t *)calloc (1, SQL_COLUMN_NAME_MAX_SIZE);
            strncpy (token_buffer , yytext, yyleng);
            break;
        case SQL_IDENTIFIER_IDENTIFIER:
            token_buffer = (uint8_t *)calloc (1, SQL_COMPOSITE_COLUMN_NAME_SIZE);
            strncpy (token_buffer , yytext, yyleng);
            break;
        case SQL_INTEGER_VALUE:
            token_buffer = (uint8_t *)calloc (1, sizeof (uint32_t));
            *(int *)(token_buffer) = atoi(yytext);
            break;
        case SQL_STRING_VALUE:
            token_buffer = (uint8_t *)calloc (1,  SQL_OPERAND_MAX_VALUE);
            assert (yyleng < SQL_OPERAND_MAX_VALUE);
            strncpy (token_buffer , yytext, yyleng);
            break;
        case SQL_IPV4_ADDR_VALUE:
            token_buffer = (uint8_t *)calloc (1, sizeof (uint32_t));
            inet_pton (AF_INET,  yytext, token_buffer);
            break;
        case SQL_DOUBLE_VALUE:
        {
            char *endp;
            token_buffer = (uint8_t *)calloc (1, sizeof (double));
            *(double *)token_buffer = strtod(yytext, &endp);
        }
            break;
        default:
            break;
    }
    return token_buffer;
}


%}

%%

"select" {
    return SQL_SELECT_Q;
}

"create table" {
    return SQL_CREATE_Q;
}

"insert into" {
    return SQL_INSERT_Q;
}

"delete" {
    return SQL_DELETE_Q;
}

"varchar" {
    return SQL_STRING;
}

"int" {
    return SQL_INT;
}

"double" {
    return SQL_DOUBLE;
}

"ipv4" {
    return SQL_IPV4_ADDR;
}

"primary key" {
    return SQL_PRIMARY_KEY;
}

"not null" {
    return SQL_NOT_NULL;
}

"max" {
    return SQL_MAX;
}

"sum" {
    return SQL_SUM;
}

"min" {
    return SQL_MIN;
}

"count" {
    return SQL_COUNT;
}

"avg" {
    return SQL_AVG;
}

"from" {
    return SQL_FROM;
}

"where" {
    return SQL_WHERE;
}

"group by" {
    return SQL_GROUP_BY;
}

"order by" {
    return SQL_ORDER_BY;
}

"limit" {
    return SQL_LIMIT;
}


"having" {
    return SQL_HAVING;
}

"asc" {
    return SQL_ORDERBY_ASC;
}

"desc" {
    return SQL_ORDERBY_DSC;
}


"(" {
    return BRACK_START;
}

")" {
    return BRACK_END;
}

"'" {
    return QUOTATION_MARK;
}

"<" {
    return SQL_LESS_THAN;
}

">" {
    return SQL_GREATER_THAN;
}

"=" {
    return SQL_EQ;
}

"!=" {
    return SQL_NOT_EQ;
}

"and" {
    return SQL_AND;
}

"or" {
    return SQL_OR;
}

"as" {
    return SQL_AS;
}

"*" {
    return SQL_MATH_MUL;
}

"+" {
    return SQL_MATH_PLUS;
}

"-" {
    return SQL_MATH_MINUS;
}

"/" {
    return SQL_MATH_DIV;
}

";" {
    return SEMI_COLON;
}

"sqrt" {
    return SQL_MATH_SQRT;
}

"sqr" {
    return SQL_MATH_SQR;
}

"mmax" {
    return SQL_MATH_MAX;
}

"mmin" {
    return SQL_MATH_MIN;
}

"sin" {
    return SQL_MATH_SIN;
}

"pow" {
    return SQL_MATH_POW;
}

\n {
    return EOL;
}

"\\\n" {

}

"\\dt\n" {
    return SHOW_DB_TABLES;
}

"," {
    return COMMA;
}

[ ] {
    /* Ignore */
    process_white_space(1);
}

[\t] {
    /*ignore*/
    process_white_space(4);
}

"\\q" {
    return QUIT;
}

(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9]) {

    return SQL_IPV4_ADDR_VALUE;
}

[1-9][0-9]* {
    return SQL_INTEGER_VALUE;
}

[0-9]*\.[0-9]+ {
    return SQL_DOUBLE_VALUE;
}

[a-zA-Z0-9_]+ {
    return SQL_IDENTIFIER;
}

[a-zA-Z0-9_]+\.[a-zA-Z0-9_]+ {
    return SQL_IDENTIFIER_IDENTIFIER;
}


%{

#if 0
^[a-zA-Z0-9][a-zA-Z0-9_.,-_'"!()\[\]{} ]* {
    
    return SQL_STRING_VALUE;
}
#endif

%}



. {
    printf ("Non parsable character : %c\n", *yytext);
    exit(0);
}

%%

/* #include other parser .c src files here */
#include "ExpressionParser.c"




static int paranthesis_cnt = 0;

static int
parse_columns (ast_node_t *parent) {

    ast_node_t *col_node ;
    ast_node_t *agg_fn_node;
    int agg_fn_token_code;
    int token_code = yylex();

    while (1) {

        switch (token_code) {

            case SQL_SUM:
            case SQL_MIN:
            case SQL_MAX:
            case SQL_COUNT:
            case SQL_AVG:
                agg_fn_token_code = token_code;
                token_code = yylex();
                PARSER_ERROR_EXIT(token_code, BRACK_START);
                token_code = yylex();
                if (token_code != SQL_IDENTIFIER &&
                    token_code != SQL_IDENTIFIER_IDENTIFIER) {
                    PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
                }
                col_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                col_node->entity_type = SQL_IDENTIFIER;
                col_node->u.identifier.ident_type = SQL_COLUMN_NAME;
                strncpy (col_node->u.identifier.identifier.name, yytext, 
                        sizeof (col_node->u.identifier.identifier.name));
                ast_add_child (parent, col_node);
                token_code = yylex();
                PARSER_ERROR_EXIT(token_code, BRACK_END);
                agg_fn_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                agg_fn_node->entity_type =  SQL_AGG_FN;
                agg_fn_node->u.agg_fn =  agg_fn_token_code;
                ast_add_child (col_node, agg_fn_node);

                token_code = yylex();
                if (token_code == COMMA) {
                    token_code = yylex();
                    continue;
                }
                break;
            case SQL_IDENTIFIER:
            case SQL_IDENTIFIER_IDENTIFIER:

                col_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                col_node->entity_type = SQL_IDENTIFIER;
                col_node->u.identifier.ident_type = SQL_COLUMN_NAME;
                strncpy (col_node->u.identifier.identifier.name, yytext, 
                        sizeof (col_node->u.identifier.identifier.name));
                ast_add_child (parent, col_node);

                token_code = yylex();
                if (token_code == COMMA) {
                    token_code = yylex();
                    continue;
                }
                break;
            case SQL_MATH_MUL:
            {
                ast_node_t *astnode = (ast_node_t *)calloc (1,sizeof (ast_node_t));
                astnode->entity_type = SQL_IDENTIFIER;
                astnode->u.identifier.ident_type = SQL_COLUMN_NAME;
                strncpy (astnode->u.identifier.identifier.name, yytext, 
                sizeof (astnode->u.identifier.identifier.name));
                ast_add_child (parent, astnode);
            // printf ("Colmn name is : %s\n", yytext);
                token_code = yylex();
                return token_code;
            }
            default:
                return token_code;
        }
    }
    return token_code;
}


static int 
parse_where_condition(ast_node_t *where_kw, where_literal_t **wlit) {

    int value;
    int op_code;
    int table_id;
    where_cond_t *wc;
    ast_node_t ast_tmplate;
    ast_node_t *table_ast_node;

    ast_node_t *root = where_kw->parent;

    char in_col_name [SQL_COMPOSITE_COLUMN_NAME_SIZE] = {0};
    char operand_col_name [SQL_COMPOSITE_COLUMN_NAME_SIZE] = {0};
    char operand_identifer [SQL_OPERAND_MAX_VALUE] = {0};

    int token_code = yylex();

    while (token_code == BRACK_START) {
        paranthesis_cnt++;
        (*wlit)->where_token_type = WHERE_LITERAL_OPERATOR_TOKEN_CODE;
        (*wlit)->u.token_id = BRACK_START;
        (*wlit)++;
        token_code = yylex();
    }

    (*wlit)->where_token_type = WHERE_LITERAL_WHERE_COND;
    wc = &(*wlit)->u.wc;
    wc->col.agg_fn = SQL_AGG_FN_NONE;

    int agg_fn_token_code = SQL_AGG_FN_NONE;

    switch (token_code) {
        case SQL_SUM:
        case SQL_MIN:
        case SQL_MAX:
        case SQL_COUNT:
        case SQL_AVG:
            agg_fn_token_code = token_code;
            token_code = yylex();
            PARSER_ERROR_EXIT(token_code, BRACK_START);
            token_code = yylex();
            break;
        default:
            break;
    }

    if (token_code != SQL_IDENTIFIER &&
        token_code != SQL_IDENTIFIER_IDENTIFIER) {
        
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
    }

    strcpy (in_col_name, yytext);

    char lone_col_name [SQL_COLUMN_NAME_MAX_SIZE] = {0};
    char table_name [SQL_TABLE_NAME_MAX_SIZE] = {0};
    char *table_name_ptr = table_name;

    parser_split_table_column_name (in_col_name, table_name, lone_col_name);

    ast_tmplate.entity_type = SQL_KEYWORD;
    ast_tmplate.u.kw = SQL_FROM;
    ast_node_t *from_kw = ast_find (root, &ast_tmplate);
    assert(from_kw);

    if (table_name[0] == '\0') {
        /* Pick the first table from the FROM list */
        table_name_ptr = from_kw->child_list->u.identifier.identifier.name;
        table_id =  *(int *)from_kw->child_list->data;
    }
    else {
        ast_tmplate.entity_type = SQL_IDENTIFIER;
        ast_tmplate.u.kw = SQL_TABLE_NAME;
        strncpy (ast_tmplate.u.identifier.identifier.name, 
                      table_name, SQL_TABLE_NAME_MAX_SIZE );
        table_ast_node = ast_find_identifier (from_kw, &ast_tmplate);
        if (!table_ast_node) {
            printf ("Error : Unrecognized Column <%s::%s>\n", 
                table_name, lone_col_name);
            return SQL_PARSE_ERROR;
        }
        table_id = *(int *)table_ast_node->data;
    }

    if (!Catalog_get_column (NULL, table_name_ptr, lone_col_name, &wc->col) ) {
        printf ("Error : Unrecognized Column <%s::%s>\n", 
            table_name_ptr, in_col_name);
        return SQL_PARSE_ERROR;
    }
    wc->col.owner_table_id = table_id;

    if (agg_fn_token_code != SQL_AGG_FN_NONE) {
        token_code = yylex();
        PARSER_ERROR_EXIT(token_code, BRACK_END);
        wc->col.agg_fn = agg_fn_token_code;
    }

    token_code = yylex();

    /* supported operators */
    op_code = token_code;

    switch(token_code) {

        case SQL_LESS_THAN:
        case SQL_GREATER_THAN:
        case SQL_EQ:
        case SQL_NOT_EQ:
        wc->op = token_code;
        break;
        default:
            printf ("%d operator not supported\n", token_code);
            return SQL_PARSE_ERROR;
    }

    //printf ("%s op(%d) ", col_name, token_code);
    token_code = yylex();

    switch (token_code) {

        case SQL_INTEGER_VALUE:
            value = atoi (yytext);
            wc->right_op.w_opd = WH_VALUE;
            wc->right_op.u.value.dtype = SQL_INT;
            wc->right_op.u.value.size = sql_dtype_size (wc->right_op.u.value.dtype);
            wc->right_op.u.value.val = calloc (wc->right_op.u.value.size, 1);
            *(int *)(wc->right_op.u.value.val) = value;
            break;

        /* Aggregate column in group by clause ?? Dont make sense ! */
        case SQL_SUM:
        case SQL_MIN:
        case SQL_MAX:
        case SQL_COUNT:
        case SQL_AVG:
            agg_fn_token_code = token_code;
            token_code = yylex();
            PARSER_ERROR_EXIT(token_code, BRACK_START);
            token_code = yylex();
            switch (token_code) {
                case SQL_IDENTIFIER:
                case SQL_STRING_VALUE:
                case SQL_IDENTIFIER_IDENTIFIER:
                    memset(in_col_name , 0, sizeof (in_col_name));
                    memset (operand_col_name, 0 , sizeof (operand_col_name));
                    strcpy (operand_col_name, yytext);
                    memset ( table_name, 0, sizeof (table_name));
                    table_name_ptr = table_name;
                    memset (lone_col_name, 0, sizeof (lone_col_name));
                    parser_split_table_column_name (operand_col_name, table_name, lone_col_name);
                    if (table_name[0] == '\0') {
                        /* Pick the first table from the FROM list */
                        ast_tmplate.entity_type = SQL_KEYWORD;
                        ast_tmplate.u.kw = SQL_FROM;
                        ast_node_t *from_kw = ast_find (root, &ast_tmplate);
                        assert(from_kw);
                        table_name_ptr = from_kw->child_list->u.identifier.identifier.name;
                        table_id =  *(int *)from_kw->child_list->data;
                        assert(!table_id);
                    }
                    else {
                        ast_tmplate.entity_type = SQL_IDENTIFIER;
                        ast_tmplate.u.kw = SQL_TABLE_NAME;
                        strncpy (ast_tmplate.u.identifier.identifier.name, 
                            table_name, SQL_TABLE_NAME_MAX_SIZE );
                        table_ast_node = ast_find_identifier (from_kw, &ast_tmplate);
                        if (!table_ast_node) {
                            printf ("Error : Unrecognized Column <%s::%s>\n", 
                                table_name, lone_col_name);
                            return SQL_PARSE_ERROR;
                        }
                        table_id = *(int *)table_ast_node->data;
                    }
                    if (!Catalog_get_column (NULL, 
                                        table_name_ptr, 
                                        lone_col_name, &wc->right_op.u.col)) {
                        printf ("Error : Unrecognized Column <%s::%s>\n", 
                            table_name_ptr, lone_col_name);
                    return SQL_PARSE_ERROR;
                    }
                    wc->right_op.u.col.owner_table_id = table_id;
                    wc->right_op.u.col.agg_fn = agg_fn_token_code;
                    token_code = yylex();
                     PARSER_ERROR_EXIT(token_code, BRACK_END);
                    break;
                default:
                    PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
            }
        break;

        case SQL_IDENTIFIER:
        case SQL_STRING_VALUE:
        case SQL_IDENTIFIER_IDENTIFIER:
            strcpy (operand_col_name, yytext);
            memset ( table_name, 0, sizeof (table_name));
            table_name_ptr = table_name;
            memset (lone_col_name, 0, sizeof (lone_col_name));
            parser_split_table_column_name (operand_col_name, table_name, lone_col_name);
            if (table_name[0] == '\0') {
                /* Pick the first table from the FROM list */
                ast_tmplate.entity_type = SQL_KEYWORD;
                ast_tmplate.u.kw = SQL_FROM;
                ast_node_t *from_kw = ast_find (root, &ast_tmplate);
                assert(from_kw);
                table_name_ptr = from_kw->child_list->u.identifier.identifier.name;
                table_id =  *(int *)from_kw->child_list->data;
                assert(!table_id);
            }
            else {
                ast_tmplate.entity_type = SQL_IDENTIFIER;
                ast_tmplate.u.kw = SQL_TABLE_NAME;
                strncpy (ast_tmplate.u.identifier.identifier.name, 
                      table_name, SQL_TABLE_NAME_MAX_SIZE );
                table_ast_node = ast_find_identifier (from_kw, &ast_tmplate);
                if (!table_ast_node) {
                    printf ("Error : Unrecognized Column <%s::%s>\n", 
                        table_name, lone_col_name);
                    return SQL_PARSE_ERROR;
                }
                table_id = *(int *)table_ast_node->data;
            }
            wc->right_op.w_opd = WH_COL;
            if (!Catalog_get_column (NULL, 
                                table_name_ptr, 
                                lone_col_name, &wc->right_op.u.col)) {
                printf ("Error : Unrecognized Column <%s::%s>\n", 
                    table_name_ptr, lone_col_name);
               return SQL_PARSE_ERROR;
            }
            wc->right_op.u.col.owner_table_id = table_id;
            break;
        case QUOTATION_MARK:
            token_code = yylex();
            //PARSER_ERROR_EXIT(token_code, SQL_STRING_VALUE);
            PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
            strcpy (operand_identifer, yytext);
            //printf ("\'%s\'\n", operand_identifer);
            wc->right_op.w_opd = WH_VALUE;
            wc->right_op.u.value.dtype = SQL_STRING;
            wc->right_op.u.value.size = strlen (yytext);
            wc->right_op.u.value.val = calloc (wc->right_op.u.value.size, 1);
            strcpy (wc->right_op.u.value.val, yytext);
            token_code = yylex();
            PARSER_ERROR_EXIT(token_code, QUOTATION_MARK);
            break;
        case SQL_IPV4_ADDR_VALUE:
            wc->right_op.w_opd = WH_VALUE;
            wc->right_op.u.value.dtype = SQL_IPV4_ADDR;
            wc->right_op.u.value.size = sql_dtype_size (wc->right_op.u.value.dtype);
            //wc->right_op.u.value.size = 16;
            wc->right_op.u.value.val = calloc (wc->right_op.u.value.size, 1);
            inet_pton (AF_INET, (const char *)yytext, wc->right_op.u.value.val);
            //sql_debug_print_where_cond (wc);
            //strcpy (wc->right_op.u.value.val , yytext);
            break;
        default:
            PARSER_ERROR_EXIT(token_code, 0);
    }

    /* What follows after condition : Logical Operators */
    token_code =  yylex();
    
    while (token_code == BRACK_END) {
        (*wlit)++;
        (*wlit)->where_token_type = WHERE_LITERAL_OPERATOR_TOKEN_CODE;
        (*wlit)->u.token_id = BRACK_END;
        paranthesis_cnt--;
        token_code = yylex();
    }

    return token_code;
}

static int
parse_where_conditions(ast_node_t *where_kw) {

    int token_code;
    where_literal_t *wlit;
    
    where_literal_t *where_literals_arr = 
        (where_literal_t *) calloc ( SQL_MAX_WHERE_LITERAL_ARRAY_SIZE, sizeof (where_literal_t));
    
    ast_node_t *where_literals_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    where_literals_node->entity_type =  SQL_IDENTIFIER;
    where_literals_node->u.identifier.ident_type = SQL_PTR;
    memcpy ((void *)where_literals_node->u.identifier.identifier.name, (void *)&where_literals_arr, sizeof (void *));
    ast_add_child (where_kw, where_literals_node);

    wlit = &where_literals_arr[0];

    while(1) {

        token_code = parse_where_condition(where_kw, &wlit);

        switch (token_code) {

            case SQL_AND:
            case SQL_OR:
            case SQL_NOT:
               /// printf ("logical Op : %s\n", yytext);
                wlit++;
                wlit->where_token_type = WHERE_LITERAL_OPERATOR_TOKEN_CODE;
                wlit->u.token_id = token_code;
            break;
            case SQL_GROUP_BY:
            case EOL:
                wlit++;
                wlit->where_token_type = WHERE_LITERAL_OPERATOR_TOKEN_CODE;
                wlit->u.token_id = EOL;  // denotes end of where condition clause
                //sql_debug_print_where_literals (where_literals_arr);
                return token_code;
        }
        wlit++;
    }

    return token_code;
}

static int
parse_tables(ast_node_t *select_kw) {

    int token_code;
    int id = 0;
    while(1) {

        token_code = yylex();
        PARSER_ERROR_EXIT(token_code, SQL_IDENTIFIER);
        //printf ("Table Name : %s\n", yytext);
        ast_node_t *tble_name_node = (ast_node_t *)calloc (1, sizeof (ast_node_t));
        tble_name_node->entity_type = SQL_IDENTIFIER;
        tble_name_node->u.identifier.ident_type = SQL_TABLE_NAME;
        strncpy(tble_name_node->u.identifier.identifier.name, 
                    yytext, sizeof (tble_name_node->u.identifier.identifier.name));
        tble_name_node->data = (int *)calloc(1, sizeof (int));
        *(int *)tble_name_node->data = id++;
        ast_add_child (select_kw, tble_name_node);
        token_code = yylex();
        if (token_code == COMMA) {
            continue;
        }
        return token_code;
    }
}

#include "SQLGroupByParser.c"



/* This file implements the SQL Parser which accespts all SQL queries satisfying the below Grammar 

    <sql_query>         ::= <select_query> | <update_query> | <delete_query>
    <select_query>     ::=  SELECT <columns> FROM <tables> WHERE <conditions>
    <columns>            ::= <plain_columns> | <aggregate_colmns> | *
    <plain_columns>  ::= <column> | <column> , <plain_columns>
    <aggregate_columns>     ::= <agg_column> | <agg_column> , <aggregate_columns>
    <agg_column> ::= <function>(<column>)
    <tables> ::= <table> | <table> , <tables>
    <conditions> ::= <condition> | <condition> <lop> <conditions>
    <condition>  ::= <column> <mop> <digits> |  <column> <mop> <column> | <column> <mop> 'identifer'
    <function>     ::= COUNT | SUM | AVG | MIN | MAX
     <column>      ::= identifier
     <table>          ::= identifier
     <lop>  ::= AND | OR | NOT | IN | BETWEEN | LIKE | EXISTS
     <digits> := <digit> | <digit><digits>
     <digit> ::= 0|1|2|3|4|5|6|7|8|9
     <mop>  ::= <   |   >   |   =   |   != 
*/

static int
parse_select_query (ast_node_t *root) {

    int token_code;
    ast_node_t *curr, *select_kw;
    ast_node_t *from_kw, *where_kw, *having_kw;

    select_kw = (ast_node_t *)calloc (1, sizeof (ast_node_t));
    select_kw->entity_type = SQL_KEYWORD;
    select_kw->u.kw = SQL_SELECT;
    ast_add_child (root, select_kw);
   
   token_code = parse_columns(select_kw /* selet keyword */);

   switch (token_code) {

    case SQL_FROM:

        from_kw = (ast_node_t *)calloc (1, sizeof (ast_node_t));
        from_kw->entity_type = SQL_KEYWORD;
        from_kw->u.kw = SQL_FROM;
        ast_add_child (root, from_kw);

        token_code = parse_tables(from_kw /* FROM keyword*/);

        if (select_kw->child_list->next == NULL &&
                strcmp (select_kw->child_list->u.identifier.identifier.name, "*") == 0) {
            /* If user typed select * .... */
            ast_node_t *star_node = select_kw->child_list;
            select_kw->child_list = NULL;
            star_node->parent = NULL;
            ast_destroy_tree (star_node);
            for (curr = from_kw->child_list; curr; curr = curr->next) {
                if (!sql_process_select_wildcard  (NULL, select_kw, curr)) {
                    return SQL_PARSE_ERROR;
                }
            }
        }

        switch(token_code) {

            case SQL_WHERE:

                where_kw = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                where_kw->entity_type = SQL_KEYWORD;
                where_kw->u.kw = SQL_WHERE;
                ast_add_child (root, where_kw);

                token_code = parse_where_conditions(where_kw);
                 switch(token_code) {
                    case SQL_GROUP_BY:
                        token_code = parse_group_by_query(root);
                        switch (token_code) {
                            case EOL:
                                return  SQL_PARSE_OK;
                            case SQL_HAVING:
                                having_kw = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                                having_kw->entity_type = SQL_KEYWORD;
                                having_kw->u.kw = SQL_HAVING;
                                ast_add_child (root, having_kw);
                                token_code = parse_where_conditions(having_kw);
                                switch (token_code) {
                                    case EOL:
                                        return SQL_PARSE_OK;
                                    default:
                                        return SQL_PARSE_ERROR;
                                }
                        }
                    break;
                    case EOL:
                        return SQL_PARSE_OK;
                 }
                break;
            case SQL_GROUP_BY:
                token_code = parse_group_by_query(root);
                switch (token_code) {
                    case EOL:
                        return  SQL_PARSE_OK;
                    case SQL_HAVING:
                        having_kw = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                        having_kw->entity_type = SQL_KEYWORD;
                        having_kw->u.kw = SQL_HAVING;
                        ast_add_child (root, having_kw);
                        token_code = parse_where_conditions(having_kw);
                        switch (token_code) {
                            case EOL:
                                return SQL_PARSE_OK;
                            default:
                                return SQL_PARSE_ERROR;
                        }
                }   
            break;            
            case EOL:
                return SQL_PARSE_OK;
        }
        break;
    case SQL_GROUP_BY:
        token_code = parse_group_by_query(root);
        switch (token_code) {
            case EOL:
                return  SQL_PARSE_OK;
            case SQL_HAVING:
                having_kw = (ast_node_t *)calloc (1, sizeof (ast_node_t));
                 having_kw->entity_type = SQL_KEYWORD;
                 having_kw->u.kw = SQL_HAVING;
                ast_add_child (root, having_kw);
                token_code = parse_where_conditions(having_kw);
                switch (token_code) {
                    case EOL:
                    return SQL_PARSE_OK;
                    default:
                    return SQL_PARSE_ERROR;
                }
        }     
    break;
    case EOL:
        return SQL_PARSE_OK;
    break;
        default:
        PARSER_ERROR_EXIT(token_code, 0);
        break;
   }
   return SQL_PARSE_OK;
}

static  void  
parse_update_query ()  {}

#include "SQLCreateParser.c"
#include "SQLCreateParserCFG.c"
#include "SQLSelectParserCFG.c"
#include "SQLInsertIntoParser.c"
#include "SQLDeleteParser.c"
#include "../core/sql_intf.h"

static parse_rc_t
parse_sql_query () {

    parse_init();
    bool rc;
    int err_code;
    ast_node_t *astroot = NULL;
    paranthesis_cnt = 0;

    clock_t tclk = clock();

    token_code = yylex();

    switch (token_code) {

#if 1
        case SQL_SELECT_Q:
            //yyless(yyleng);
            astroot = (ast_node_t *)calloc (1, sizeof (ast_node_t));
            astroot->entity_type = SQL_QUERY_TYPE;
            astroot->u.q_type = SQL_SELECT_Q;
            err_code = parse_select_query  (astroot);
            //ast_print (astroot, 0);
            if (paranthesis_cnt) {
                printf ("Error : Unbalanced Paranthesis\n");
                 ast_destroy_tree_from_root (astroot);
                 RETURN_PARSE_ERROR;
            }
            if (err_code == SQL_PARSE_ERROR) {
                ast_destroy_tree_from_root (astroot);
                RETURN_PARSE_SUCCESS;
            }
            sql_process_select_query (NULL, astroot);
            break;
#else
        case SQL_SELECT_Q:
            astroot = parse_select_query_cfg();
            if (astroot) {
                sql_process_select_query (NULL, astroot);
            }
        break;
#endif
        case SQL_UPDATE_Q:
           // yyless(yyleng);
            //parse_update_query ();
            break;
        case SQL_CREATE_Q:
            astroot = (ast_node_t *)calloc (1, sizeof (ast_node_t));
            astroot->entity_type = SQL_QUERY_TYPE;
            astroot->u.q_type = SQL_CREATE_Q;
            //err = parse_create_query(&lexc, astroot);
            err = parse_create_query_cfg(astroot);
            if (err == PARSE_ERR) {
                printf ("Table Creation Failed\n");
                break;
            }
            //ast_print (astroot, 0);
            sql_process_create_query(NULL, astroot);
            token_code = cyylex();
            assert(token_code == EOL);
            break;
        case SHOW_DB_TABLES:
            sql_show_table_catalog (NULL);
            break;
        case SQL_INSERT_Q:
             astroot = (ast_node_t *)calloc (1, sizeof (ast_node_t));
             astroot->entity_type = SQL_QUERY_TYPE;
             astroot->u.q_type = SQL_INSERT_Q;
             parse_insert_query (astroot);
             //ast_print (astroot, 0);
             sql_process_insert_query(NULL, astroot);
             break;
        case SQL_DELETE_Q:
            astroot = (ast_node_t *)calloc (1, sizeof (ast_node_t));
             astroot->entity_type = SQL_QUERY_TYPE;
             astroot->u.q_type = SQL_DELETE_Q;
             if (parse_delete_query (astroot) == SQL_PARSE_OK) {
                sql_process_delete_query(NULL, astroot);
             }
             break;
        case SQL_PARSE_OK:
        case EOL:
            ast_destroy_tree_from_root (astroot);
            RETURN_PARSE_SUCCESS;
        case QUIT:
            exit(0);
        default:
            printf ("%s : Unsupported Command\n", yytext);
            yylex(); // consume \n
            break;
    }

    tclk= clock() - tclk;
    double time_taken = ((double)tclk * 1000)/CLOCKS_PER_SEC; 
    printf("%f msec\n", time_taken);
    ast_destroy_tree_from_root (astroot);
    if (err == PARSE_ERR) RETURN_PARSE_ERROR;
    RETURN_PARSE_SUCCESS;
}

/* Tested with :

select name1 as N1, name2 as N2, sqrt(age + salary) as N3 from rt1 as RT1, rt2 as RT2 where age > N3 and sqrt
(age) = sqr(salary) group by mmax(age, salary), salary having (sqrt(age + salary) > 1000) and (bal > 1000) order 
by RT1 asc limit 2;

select sqr(name) * 3 + 4, sqrt(age * salary) - age * 4 from tab1, tabe where mmax(salary, amount) * 3 < 10000 and 
salary > sqr(10) group by name, sqr(name) * 3 + 4 having  (sqrt(age * salary)) = (age * 4) order by name asc 
limit 4;

*/

#if 0
int 
main (int argc, char **argv) {

    Parser_init (parser_alloc_token_value);

    parse_init ();

    strcpy (lex_buffer, "a + 2\n" );
    yy_scan_string (lex_buffer);

    err = E();

    if (err == PARSE_ERR) {
        printf ("Invalid MExpr\n");
    }
    else {
        printf ("Valid MExpr, curr_ptr = %s\n", curr_ptr);
        token_code = cyylexlh();
        if (token_code == EOL) {
            printf ("Parsed Complete\n");
        }
    }
    Parser_stack_reset ();
    return 0;
}
#endif 
#if 1

int 
main (int argc, char **argv) {

    memset (lex_buffer, 0 , sizeof (lex_buffer));

    while(1) {
        printf ("postgres=# ");
        fgets (lex_buffer, sizeof (lex_buffer), stdin);
        if (lex_buffer[0] == '\n') {
            lex_buffer[0] = 0;
            continue;
        }
        yy_scan_string (lex_buffer);
        parse_sql_query();
        memset (lex_buffer, 0 , sizeof (lex_buffer));
        Parser_stack_reset();
    }
    return 0;
}

#endif 

#if 0
int 
main (int argc, char **argv) {

    //Parser_init (parser_alloc_token_value);
    
    parse_init();

    //strcpy (lex_buffer , "mmin (sin (sqr (sqrt (a + b) +  mmax (b * c, 2))), mmax(d, e))\n");
    //strcpy (lex_buffer , "sqrt(10 + 6)\n");
    //strcpy (lex_buffer , "mmax(3 * 5, 2)\n");
    strcpy (lex_buffer , "mmax (sqr (4), sqrt (25)) + 5\n");
    //strcpy (lex_buffer, "mmax(6, mmax(8, mmax (3, mmax (5, 7))))\n");

    yy_scan_string (lex_buffer);
   
   err = PARSER_CALL(E);

    assert (err = PARSE_SUCCESS);
    token_code = cyylexlh ();
    assert (token_code == EOL);

    int size_out = 0;
    lex_data_t **postfix = mexpr_convert_infix_to_postfix (
                                            &undo_stack.data[0], undo_stack.top + 1, &size_out);

    int i;
    lex_data_t *lex_data;

    printf ("Postfix expression : \n");

    for (i = 0; i < size_out; i++) {

        lex_data = postfix[i];
        printf ("%s ", (char *)lex_data->token_val);
    }

    printf("\n");
    
    mexpt_node_t *root = 
        mexpr_convert_postfix_to_expression_tree (postfix, size_out);

    mexpr_debug_print_expression_tree (root);

    printf ("Evaluation Expression Tree : \n");

    res_t res = mexpt_evaluate (root);

    if (res.rc) {

        if (double_is_integer (res.ovalue)) {

            int temp = (int)res.ovalue;
            printf ("Result = %d\n", temp);
        }
        else {
            printf ("Result = %lf\n", res.ovalue);
        }
    }
    else {
        
        printf ("Evaluation Failed\n");
    }

    mexpt_destroy (root);
    Parser_stack_reset ();
    return 0;
}
#endif 
