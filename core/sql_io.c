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
print_spaces (char c , int n) {
    while (n--) {
        printf ("%c", c);
    }
}

void 
sql_print_hdr (BPlusTree_t *schema_table, glthread_t *col_list_head ) {

    glthread_t *curr;
    list_node_t *lnode;
    BPluskey_t bpkey_tmplate;
    schema_rec_t *schema_rec;

    return ;

    ITERATE_GLTHREAD_BEGIN (col_list_head, curr) {

        lnode = glue_to_list_node(curr);
        bpkey_tmplate.key = (void *)lnode->data;
        bpkey_tmplate.key_size = SQL_COLUMN_NAME_MAX_SIZE;
        schema_rec = (schema_rec_t *) BPlusTree_Query_Key (schema_table, &bpkey_tmplate);
       

    } ITERATE_GLTHREAD_END (col_list_head, curr);
}

void 
 sql_emit_select_output (BPlusTree_t *schema_table, 
                                        glthread_t *col_list_head, 
                                        void *record_ptr) {

    glthread_t *curr;
    list_node_t *lnode;
    unsigned char *column_name;
    schema_rec_t *schema_rec;
    BPluskey_t bpkey_tmplate;

    ITERATE_GLTHREAD_BEGIN (col_list_head, curr) {

        lnode =  glue_to_list_node(curr);
        column_name = (char *)lnode->data;
        printf ("%s : ", column_name);
        bpkey_tmplate.key = (void *)column_name;
        bpkey_tmplate.key_size = SQL_COLUMN_NAME_MAX_SIZE;
        schema_rec = (schema_rec_t *) BPlusTree_Query_Key (schema_table, &bpkey_tmplate);
        assert(schema_rec);
        switch (schema_rec->dtype) {
            case SQL_STRING:
                printf ("%-20s\t", (char *)record_ptr + schema_rec->offset);
                break;
            case SQL_INT:
                printf ("%-6d\t", *(int *)((char *)record_ptr + schema_rec->offset));
                break;
            case SQL_FLOAT:
                printf ("%-10f\t", *(float *)((char *)record_ptr + schema_rec->offset));
                break;
            case SQL_IPV4_ADDR:
            {
                unsigned char ipv4_addr_str [16];
                inet_ntop (AF_INET, (char *)record_ptr + schema_rec->offset, ipv4_addr_str, 16);
                printf ("%-16s\t", ipv4_addr_str);
            }
            break;
            default:
                assert(0);
        }
    }  ITERATE_GLTHREAD_END (col_list_head, curr);
}