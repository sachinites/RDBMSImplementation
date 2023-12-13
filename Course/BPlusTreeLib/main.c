#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>
#include "BPlusTree.h"

typedef enum dtype_{
	
	DTYPE_STRING,
	DTYPE_INT,
	DTYPE_DOUBLE

} dtype_t;	

// B+ would going to support these three data types as of now = SQL Tables
// primary key of SQL table = combination of one or more colmns of SQL table = Key of a B+ tree  
// Key of a B+ Tree = <string 32> <int 4 B> <int 4B> = SQL Table in which the primary key is <name of employee> <emp-id> <dept-id>

// -1 -  key1 > key2 
//  0  - key1 = key2
// 1  - key2 > key 1
static int 
bplus_tree_key_comp_fn ( BPluskey_t *key1, BPluskey_t *key2, key_mdata_t *key_mdata, int key_mdata_size) {

    int rc;
    int dsize;
    int offset = 0;
    dtype_t dtype;

    int i;

    if (!key1 || !key1->key || !key1->key_size) return 1;
    if (!key2 || !key2->key || !key2->key_size) return -1;

    char *key1_ptr = (char *)key1->key;
    char *key2_ptr = (char *)key2->key;

    for (i = 0; i < key_mdata_size; i++) {

        dtype =  (dtype_t)(key_mdata)[i].dtype; // dtype of 0th element in a B+ Tree key
        dsize = (key_mdata)[i].size;

        switch (dtype) {

            case DTYPE_STRING:
                rc = strncmp (key1_ptr + offset, key2_ptr + offset, dsize) ;
                if (rc < 0) return 1;
                if (rc > 0) return -1;
                offset += dsize;
                break;

            case DTYPE_INT:
                {
                    int *n1 = (int *)(key1_ptr + offset);
                    int *n2 = (int *)(key2_ptr + offset);
                    if (*n1 < *n2) return 1;
                    if (*n1 > *n2) return -1;
                    offset += dsize;
                }
                break;

            case DTYPE_DOUBLE:
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

int 
main (int argc, char **argv) {

    int choice;
    BPlusTree_t tree;
    char discard_buffer[2];

    memset(&tree, 0, sizeof(BPlusTree_t));

    // key = string 32, value = string of 32 chars
    static key_mdata_t key_mdata[] = {    {DTYPE_STRING, 32} };

    // key = <string32> <int> <int>  
    //static key_mdata_t key_mdata[] = {    {DTYPE_STRING, 32}, {DTYPE_INT, 4}, {DTYPE_INT, 4} };

    BPlusTree_init (&tree, 
                                bplus_tree_key_comp_fn,   // ptr to the key comparison fn
                                NULL, 
                                NULL,
                                 4,  // max no of children that a B+ Tree node can accomodate (fanout of B+ Tree)
                                NULL,  // free the record in a B+ Tree leaf nodes , default will be free()
                                key_mdata, 
                                sizeof(key_mdata)/sizeof(key_mdata_t));

	while (1) {

		printf ("1. Insert\n");
		printf ("2. Delete\n");
		printf ("3. Update\n");
		printf ("4. Read\n");
		printf ("5. Destroy\n");
		printf ("6. Iterate over all Records\n");
		printf ("7. exit\n");

        scanf ("%d", &choice);

        //read \n an discard it
        fgets ((char *) discard_buffer, sizeof(discard_buffer), stdin);
        fflush(stdin);

        switch (choice) {

            case 1:
            {
                BPluskey_t bpkey; // stack variable
                /* Take a new buffer from heap, because this buffer will
                going to chache by B+ Tree internally */
                char *key_buffer = (char *)calloc (1, 32);
                /* Set up the key*/
                bpkey.key = (void *)key_buffer;
                bpkey.key_size = 32;
                /* Take key as input from the user */
                printf ("Insert Key : ");
                fgets(key_buffer, 32, stdin);
                // remove trailing \n
                key_buffer[strcspn(key_buffer, "\n")] = '\0';
                /* Setup the value*/
                /* Take a new value bufer from heap, because this buffer will
                going to be cache by B+ Tree interbally*/
                char *value_buffer = (char *) calloc (1, 32);
                /* Take the value from use as input*/
                printf ("Insert Value : ");
                fgets(value_buffer, 32, stdin);
                /* Insert the key value pair in the B+ Tree*/
                BPlusTree_Insert (&tree, &bpkey, (void*)value_buffer);
            }
            break;

        case 2:
            {
                BPluskey_t bpkey;
                char key_buffer[32];
                /*Set up the key*/
                bpkey.key = (void *)key_buffer;
                bpkey.key_size = 32;
                /* Take key as input from the user*/
                fgets (key_buffer, sizeof(key_buffer), stdin);
                // remove trailing \n
                key_buffer[strcspn(key_buffer, "\n")] = '\0';
                /* Delete the key + Record from the B+ Tree*/
                BPlusTree_Delete (&tree, &bpkey);
            }
            break;



        case 3:
        {
            BPluskey_t bpkey;
            char key_buffer[32];
            /* Set up the key*/
            bpkey.key = (void *)key_buffer;
            bpkey.key_size = 32;
            /* Take key as input from the user*/
            fgets (key_buffer, sizeof(key_buffer), stdin);
            // remove trailing \n
            key_buffer[strcspn(key_buffer, "\n")] = '\0';            

            char *old_value_bufer = (char *)BPlusTree_Query_Key(&tree, &bpkey);
            if (!old_value_bufer) {
                printf ("key not found\n");
                scanf("\n");
                break;
            }
            printf ("Old Record = %s\n", old_value_bufer);
            /* Take the New Value as Input from the user*/
            char *new_value_buffer = (char *)calloc (1, 32);
            printf ("Insert new Value ? : \n");
            fgets (new_value_buffer, 32, stdin);
            // remove trailing \n
            new_value_buffer[strcspn(new_value_buffer, "\n")] = '\0';	
            /* it will free the old record value and put the new one*/
            BPlusTree_Modify(&tree, &bpkey, (void *)new_value_buffer);
        }
        break;
        
        case 4:
			{
				BPluskey_t bpkey;
				char key_buffer[32];
				/* Setup the key*/
				bpkey.key = (void *)key_buffer;
				bpkey.key_size = sizeof(key_buffer);
				/* Take Key Input from the user*/
				printf ("Insert Key for Read ? : ");
				fgets ( key_buffer, bpkey.key_size, stdin);
				 // Remove trailing newline
				key_buffer[strcspn(key_buffer, "\n")] = '\0';
				char *val_buff = (char *)BPlusTree_Query_Key(&tree, &bpkey);
				if (!val_buff) {
					printf ("Key not found\n");
					scanf("\n");
					break;
				}
				printf ("Value = %s\n", (char *)val_buff);
			}
			break;

        case 5:
            {
                BPlusTree_Destroy (&tree);
                printf ("Successfully Destroyed the B+ Tree\n");
            }
            break;

        case 6:
            {
                BPluskey_t *bpkey;
                void *rec;

                BPTREE_ITERATE_ALL_RECORDS_BEGIN((&tree), bpkey, rec) {

                    printf ("Key = %s , value = %s\n", (char *)bpkey->key, (char *)rec);

                } BPTREE_ITERATE_ALL_RECORDS_END((&tree), bpkey, rec)
            }
            break;
        
        }
    }
    return 0;

}