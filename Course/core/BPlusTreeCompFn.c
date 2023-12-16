#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>
#include "../SqlParser/SqlEnums.h"
#include "../BPlusTreeLib/BPlusTree.h"

// B+ would going to support these three data types as of now = SQL Tables
// primary key of SQL table = combination of one or more colmns of SQL table = Key of a B+ tree  
// Key of a B+ Tree = <string 32> <int 4 B> <int 4B> = SQL Table in which the primary key is <name of employee> <emp-id> <dept-id>

// -1 -  key1 > key2 
//  0  - key1 = key2
// 1  - key2 > key 1

int 
rdbms_key_comp_fn ( BPluskey_t *key1, BPluskey_t *key2, key_mdata_t *key_mdata, int key_mdata_size) {

    int rc;
    int dsize;
    int offset = 0;
    sql_dtype_t dtype;

    int i;

    if (!key1 || !key1->key || !key1->key_size) return 1;
    if (!key2 || !key2->key || !key2->key_size) return -1;

    char *key1_ptr = (char *)key1->key;
    char *key2_ptr = (char *)key2->key;

    for (i = 0; i < key_mdata_size; i++) {

        dtype =  (sql_dtype_t)(key_mdata)[i].dtype; // dtype of 0th element in a B+ Tree key
        dsize = (key_mdata)[i].size;

        switch (dtype) {

            case SQL_STRING:
                rc = strncmp (key1_ptr + offset, key2_ptr + offset, dsize) ;
                if (rc < 0) return 1;
                if (rc > 0) return -1;
                offset += dsize;
                break;

            case SQL_INT:
                {
                    int *n1 = (int *)(key1_ptr + offset);
                    int *n2 = (int *)(key2_ptr + offset);
                    if (*n1 < *n2) return 1;
                    if (*n1 > *n2) return -1;
                    offset += dsize;
                }
                break;

            case SQL_DOUBLE:
              {
                    double *n1 = (double *)(key1_ptr + offset);
                    double*n2 = (double *)(key2_ptr + offset);
                    if (*n1 < *n2) return 1;
                    if (*n1 > *n2) return -1;
                    offset += dsize;
                }
                break;
        }
    }

    return 0;
}
