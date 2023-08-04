#include "../BPlusTreeLib/BPlusTree.h"

void
sql_print_hdr (BPlusTree_t *TableSchema) {


}

void 
sql_print_record (BPlusTree_t *TableSchema, unsigned char *record, bool hdr) {

    if (hdr) {
        sql_print_hdr (TableSchema);
    }

    
}