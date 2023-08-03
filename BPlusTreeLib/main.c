#include <stdio.h>
#include <arpa/inet.h>
#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "BPlusTree.h"
#include "../core/rdbms_struct.h"

unsigned char *
tcp_ip_covert_ip_n_to_p(uint32_t ip_addr, 
                                        unsigned char* output_buffer){

    memset(output_buffer, 0, 16);
    ip_addr = htonl(ip_addr);
    inet_ntop(AF_INET, &ip_addr, (char *)output_buffer, 16);
    output_buffer[15] = '\0';
    return output_buffer;
}

uint32_t
tcp_ip_covert_ip_p_to_n(unsigned char * ip_addr){

    uint32_t binary_prefix = 0;
    inet_pton(AF_INET, (const char *)ip_addr, &binary_prefix);
    binary_prefix = htonl(binary_prefix);
    return binary_prefix;
}

static int8_t 
BplusTree_key_comp_fn_ip_addr (BPluskey_t *key1, BPluskey_t *key2, key_mdata_t *key_mdata, int size) {

	if (!key1->key && key2->key) return 1;
	if (key1->key && !key2->key) return -1;

	uint32_t key1_ipaddr;
	inet_pton (AF_INET, (const char *)key1->key, &key1_ipaddr);
	key1_ipaddr = htonl (key1_ipaddr);

	uint32_t key2_ipaddr;
	inet_pton (AF_INET, (const char *)key2->key, &key2_ipaddr);
	key2_ipaddr = htonl (key2_ipaddr);

	return key2_ipaddr - key1_ipaddr;
}

static int8_t 
BplusTree_key_comp_fn_default (BPluskey_t *key1, BPluskey_t *key2, key_mdata_t *key_mdata, int size) {

	if (!key1->key && key2->key) return 1;
	if (key1->key && !key2->key) return -1;

	assert (key1->key_size == key2->key_size);

	int rc = memcmp (key1->key, key2->key, key1->key_size);
	if (rc < 0) return 1;
	if (rc > 0 ) return -1;
	return 0;
}

static int
BPlusTree_key_format_fn_default (BPluskey_t *key, unsigned char *obuff, int buff_size) {

	assert (key->key_size <= buff_size);
	memset (obuff, 0, buff_size);
	memcpy (obuff, key->key, key->key_size);
	return  key->key_size;
}

static int
BPlusTree_key_format_fn_ipv4_addr (BPluskey_t *key, unsigned char *obuff, int buff_size) {

	assert (key->key_size <= buff_size);
	memset (obuff, 0, buff_size);
	inet_ntop (AF_INET, key->key, obuff, buff_size);
	return  16;
}


static int
BPlusTree_value_format_fn_default (void *value, unsigned char *obuff, int buff_size) {

	memset (obuff, 0, buff_size);
	strncpy ( (char *)obuff, (char *)value, buff_size);
	return 0;
}

static int
BPlusTree_value_format_fn_ipv4_addr (void *value, unsigned char *obuff, int buff_size) {
	memset (obuff, 0, buff_size);
	inet_ntop (AF_INET, value, obuff, buff_size);
	return  16;
}


extern int 
rdbms_key_comp_fn (BPluskey_t *key_1, BPluskey_t *key_2, key_mdata_t *key_mdata, int size) ;

int 
main (int argc, char **argv) {

	int len;
	int choice;
	uint32_t ip_addr_n;
	char *val_buff ;
	BPluskey_t bkey;
	unsigned char key[64];
	unsigned char value[128];
	BPlusTree_t tree;
	memset (&tree, 0, sizeof (tree));
	BPlusTree_init (&tree, 
			rdbms_key_comp_fn,
			BPlusTree_key_format_fn_ipv4_addr, 
			BPlusTree_value_format_fn_ipv4_addr,
			4, free);

    static key_mdata_t key_mdata[] = {  {SQL_IPV4_ADDR, 1} };
    tree.key_mdata = key_mdata;
    tree.key_mdata_size = 1;

	while (1) {

		printf ("1. Insert\n");
		printf ("2. Delete\n");
		printf ("3. Update\n");
		printf ("4. Range\n");
		printf ("5. Read\n");
		printf ("6. Destroy\n");

		scanf ("%d", &choice);
		fgets ( (char *)key, sizeof(key), stdin);
		fflush (stdin);

		switch (choice) {

			case 1:
				printf ("Insert Key : ");
				memset (key, 0, sizeof (key));
				fgets ( (char *)key, sizeof(key), stdin);
				key[strcspn(key, "\n")] = '\0';
				printf ("Insert Value : ");
				memset (value, 0, sizeof (value));
				fgets ( (char *)value, sizeof(value), stdin);
				value[strcspn(value, "\n")] = '\0';
				len =  4; //strlen ( (const char *)key);
				bkey.key = (char *)calloc (1, len);
				inet_pton (AF_INET, (const char *)key, (void *)bkey.key);
				bkey.key_size = len;
				len = 4; //strlen ( (const char *)value);
				val_buff = (char *)calloc (1, len);
				inet_pton (AF_INET, (const char *)value, (void *)val_buff );
				BPlusTree_Insert (&tree, &bkey, (void *)val_buff);
				break;
			case 2:
				{
				printf ("Insert Key : ");
				memset (key, 0, sizeof (key));
				fgets ( (char *)key, sizeof(key), stdin);
				key[strcspn(key, "\n")] = '\0';
				len =  4; //strlen ( (const char *)key);
				inet_pton (AF_INET, (const char *)key, (void *)&ip_addr_n);
				bkey.key = &ip_addr_n;
				bkey.key_size = len;
				BPlusTree_Delete (&tree, &bkey);
				break;
				}
			case 4:
				{
				// Query on a range [l, r]
				double start_time, end_time;
				uint32_t ip_addr1, ip_addr2;
				printf ("Insert Key1 : ");
				memset (key, 0, sizeof (key));
				fgets ( (char *)key, sizeof(key), stdin);
				key[strcspn(key, "\n")] = '\0';
				len =  4; //strlen ( (const char *)key);
				inet_pton (AF_INET, (const char *)key, (void *)&ip_addr1);
				bkey.key = &ip_addr1;
				bkey.key_size = len;

				printf ("Insert key2 : ");
				BPluskey_t bkey2;
				memset (key, 0, sizeof (key));
				fgets ( (char *)key, sizeof(key), stdin);
				key[strcspn(key, "\n")] = '\0';
				len =  4; //strlen ( (const char *)key);
				inet_pton (AF_INET, (const char *)key, (void *)&ip_addr2);
				bkey2.key = &ip_addr2;
				bkey2.key_size = len;

				start_time = clock();
				BPlusTree_Query_Range(&tree, &bkey, &bkey2);
				end_time = clock();
				printf("Query on a range, costs %lf s\n", (end_time - start_time) / CLOCKS_PER_SEC);
				break;
			}
		case 5:
				printf ("Insert Query Key : ");
				memset (key, 0, sizeof (key));
				fgets ( (char *)key, sizeof(key), stdin);
				key[strcspn(key, "\n")] = '\0';
				len = 4; // strlen ( (const char *)key);
				inet_pton (AF_INET, (const char *)key, (void *)&ip_addr_n);
				bkey.key = &ip_addr_n;
				bkey.key_size = len;
				BPlusTree_Query_Key(&tree, &bkey);
				break;
		}
	}
	
	return 0;
}