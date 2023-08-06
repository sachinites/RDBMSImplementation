#include <stddef.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include "sql_const.h"
#include "rdbms_struct.h"
#include "Catalog.h"
#include "sql_io.h"
#include "../BPlusTreeLib/BPlusTree.h"

static void 
print_line(int num_columns, int column_width) {
    for (int i = 0; i < num_columns; i++) {
        for (int j = 0; j < column_width; j++) {
            putchar('-');
        }
        putchar('+');
    }
    putchar('\n');
}

void 
sql_print_hdr (glthread_t *col_list_head ) {

    glthread_t *curr;
    list_node_t *lnode;
    unsigned char *column_name;
    int num_columns = 0;

    // Count the number of columns to calculate the width of each column
    ITERATE_GLTHREAD_BEGIN(col_list_head, curr) {
        num_columns++;
    } ITERATE_GLTHREAD_END(col_list_head, curr);

    int column_width = 20; // Default column width
    if (num_columns > 0) {
        column_width = 80 / num_columns; // Adjust column width based on available space
    }

    // Print the top line
    print_line(num_columns, column_width);

    // Print the header row with column names
    ITERATE_GLTHREAD_BEGIN(col_list_head, curr) {
        lnode = glue_to_list_node(curr);
        column_name = (char *)lnode->data;
        printf("%-*s|", column_width, column_name);
    } ITERATE_GLTHREAD_END(col_list_head, curr);

    printf("\n");

    // Print the line below the header
    print_line(num_columns, column_width);
}

void sql_emit_select_output(BPlusTree_t *schema_table, 
                                              glthread_t *col_list_head,
                                              void *record_ptr) {

    glthread_t *curr;
    list_node_t *lnode;
    unsigned char *column_name;
    schema_rec_t *schema_rec;
    BPluskey_t bpkey_tmplate;

    int num_columns = 0;

    // Count the number of columns to calculate the width of each column
    ITERATE_GLTHREAD_BEGIN(col_list_head, curr) {
        num_columns++;
    } ITERATE_GLTHREAD_END(col_list_head, curr);

    int column_width = 20; // Default column width
    if (num_columns > 0) {
        column_width = 80 / num_columns; // Adjust column width based on available space
    }

    // Print each row of data
    ITERATE_GLTHREAD_BEGIN(col_list_head, curr) {
        lnode = glue_to_list_node(curr);
        column_name = (char *)lnode->data;
        bpkey_tmplate.key = (void *)column_name;
        bpkey_tmplate.key_size = SQL_COLUMN_NAME_MAX_SIZE;
        schema_rec = (schema_rec_t *)BPlusTree_Query_Key(schema_table, &bpkey_tmplate);
        assert(schema_rec);

        switch (schema_rec->dtype) {
            case SQL_STRING:
                printf("%-*s|", column_width, (char *)record_ptr + schema_rec->offset);
                break;
            case SQL_INT:
                printf("%-*d|", column_width, *(int *)((char *)record_ptr + schema_rec->offset));
                break;
            case SQL_FLOAT:
                printf("%-*f|", column_width, *(float *)((char *)record_ptr + schema_rec->offset));
                break;
            case SQL_IPV4_ADDR: {
                unsigned char ipv4_addr_str[16];
                inet_ntop(AF_INET, (char *)record_ptr + schema_rec->offset, ipv4_addr_str, 16);
                printf("%-*s|", column_width, ipv4_addr_str);
                break;
            }
            default:
                assert(0);
        }
    } ITERATE_GLTHREAD_END(col_list_head, curr);

    printf("\n");

}






