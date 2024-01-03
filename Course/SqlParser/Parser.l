%{

#include "ParserExport.h"


stack_t undo_stack = {-1, {0, 0, 0}};

char lex_buffer[MAX_STRING_SIZE];
char *curr_ptr = lex_buffer;
char *lex_curr_token;
int lex_curr_token_len;

static void 
lex_push (lex_data_t lex_data) {

    assert (undo_stack.top < MAX_MEXPR_LEN - 1);
    undo_stack.data[++undo_stack.top] = lex_data;
    lex_curr_token = (char *)lex_data.token_val;
    lex_curr_token_len = lex_data.token_len;
}

static lex_data_t
lex_pop () {

    assert (undo_stack.top > -1);
    lex_data_t res = undo_stack.data[undo_stack.top];
    undo_stack.top--;
    lex_curr_token = ( char *)res.token_val;
    lex_curr_token_len = res.token_len;
    return res;
}

static void 
process_white_space(int n) {

    lex_data_t lex_data;
    curr_ptr += n;
    lex_data.token_code = PARSER_WHITE_SPACE;
    lex_data.token_len = n;
    lex_data.token_val = NULL;
    lex_push(lex_data);
}

int 
cyylex() {

    int token_code = yylex();
    lex_data_t lex_data;
    lex_data.token_code = token_code;
    lex_data.token_len = yyleng;
    curr_ptr += yyleng;
    lex_data.token_val = (char *)calloc (1, yyleng + 1);
    strncpy (lex_data.token_val, yytext, yyleng);
    lex_data.token_val[yyleng] = '\0';
    lex_push(lex_data);
    return token_code;
}

void 
yyrewind (int n) {

    if (n <= 0) return;
    if (curr_ptr == lex_buffer) return;
    int data_len = 0;
    lex_data_t lex_data;
    while (n)  {
        lex_data = lex_pop();
        data_len += lex_data.token_len;
        if (lex_data.token_code == PARSER_WHITE_SPACE) {
            continue;
        }
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
    if (undo_stack.top >= 0) {
        lex_data = undo_stack.data[undo_stack.top];
        lex_curr_token = lex_data.token_val;
        lex_curr_token_len = lex_data.token_len;
    }    
}

void
RESTORE_CHKP(int a) {

    lex_data_t lex_data;
    int data_len = 0;

    while (undo_stack.top > a) {
        
        lex_data =  lex_pop();
        data_len += lex_data.token_len;

        if (lex_data.token_val) { 
            free (lex_data.token_val);
            lex_data.token_val = NULL;
        }
    }
    curr_ptr -= data_len;
    yy_scan_string(curr_ptr);
    if (undo_stack.top >= 0) {
        lex_data = undo_stack.data[undo_stack.top];
        lex_curr_token = lex_data.token_val;
        lex_curr_token_len = lex_data.token_len;
    }
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
    lex_curr_token = NULL;
    lex_curr_token_len = 0;
}

void 
lex_set_scan_buffer (const char *buffer) {

    yy_scan_string (buffer);
}

/* Dont change anything above this line in this file */

// dedicated to declare all global variables which parser will use END

#include "SqlEnums.h"



%}


%%

"select" {
    return SQL_SELECT_Q;
}

"create" {
    return SQL_CREATE_Q;
}

"insert into" {
    return SQL_INSERT_Q;
}

"delete" {
    return SQL_DELETE_Q;
}

"drop" {
    return SQL_DROP_TABLE_Q;
}

"varchar" {
    return SQL_STRING;
}

"int" {
    return SQL_INT;
}

decimal|double {
    return SQL_DOUBLE;
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
    return SQL_BRACKET_START;
}

")" {
    return SQL_BRACKET_END;
}

"'" {
    return SQL_QUOTATION_MARK;
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

"in" {
    return SQL_IN;
}

"LIKE" {
    
    return SQL_LIKE;
}

"sqrt" {
    return SQL_MATH_SQRT;
}

"sqr" {
    return SQL_MATH_SQR;
}

"hmax" {
    return SQL_MATH_MAX;
}

"hmin" {
    return SQL_MATH_MIN;
}

"sin" {
    return SQL_MATH_SIN;
}

"pow" {
    return SQL_MATH_POW;
}

\n {
    return PARSER_EOL;
}


"," {
    return SQL_COMMA;
}

[ ] { 
    /* Ignore */
    process_white_space(1);
}

[\t] {
    /*ignore*/
    process_white_space(4);
}

0|-?[1-9][0-9]* {
    return SQL_INTEGER_VALUE;
}

-?[0-9]*\.[0-9]+ {
    return SQL_DOUBLE_VALUE;
}

[a-zA-Z0-9_]+ {
    return SQL_IDENTIFIER;
}

[a-zA-Z0-9_]+\.[a-zA-Z0-9_]+ {
    return SQL_IDENTIFIER_IDENTIFIER;
}

'[^']*' {
    // string enclosed within single quotes. This regular expression allows multiple words separated by spaces
    return SQL_STRING_VALUE;
}

\"[^"]*\" {
    // string enclosed within double quotes. This regular expression allows multiple words separated by spaces
    return SQL_STRING_VALUE;
}

. {
    
}


%%
