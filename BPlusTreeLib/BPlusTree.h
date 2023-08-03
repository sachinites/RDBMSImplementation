#ifndef __BPlusTree_H__
#define __BPlusTree_H__

#define MAX_CHILD_NUMBER 5
#include <stdint.h>
#include <stdbool.h>

typedef struct key_mdata_ key_mdata_t ;

typedef struct BPluskey {

	uint16_t key_size;
	void *key;

} BPluskey_t;

typedef struct BPlusTreeNode {
	bool isRoot, isLeaf;
	int key_num;
	BPluskey_t key[MAX_CHILD_NUMBER];
	void* child[MAX_CHILD_NUMBER];
	struct BPlusTreeNode* father;
	struct BPlusTreeNode* next;
	struct BPlusTreeNode* last;

} BPlusTreeNode;

typedef int (*BPlusTree_key_com_fn )(BPluskey_t *, BPluskey_t *, key_mdata_t *, int );
typedef int (*BPlusTree_key_format_fn) (BPluskey_t *, unsigned char *, int);
typedef int (*BPlusTree_value_format_fn) (void *, unsigned char *, int);
typedef void (*BPlusTree_value_free_fn) (void *);

typedef struct BPlusTree {

	BPlusTree_key_com_fn comp_fn;
	BPlusTree_key_format_fn key_fmt_fn;
	BPlusTree_value_format_fn value_fmt_fn;
	BPlusTreeNode *Root;
	uint16_t MaxChildNumber;
	BPlusTree_value_free_fn free_fn;
	key_mdata_t *key_mdata;
	int key_mdata_size;

} BPlusTree_t;

extern void BPlusTree_init (BPlusTree_t *, 
							BPlusTree_key_com_fn, 
							BPlusTree_key_format_fn,
							BPlusTree_value_format_fn, uint16_t MaxChildNumber,
							BPlusTree_value_free_fn free_fn);

extern void BPlusTree_SetMaxChildNumber(BPlusTree_t *, int);
extern void BPlusTree_Destroy(BPlusTree_t *);
extern int BPlusTree_Insert(BPlusTree_t *, BPluskey_t *, void*);

extern void BPlusTree_Query_Key(BPlusTree_t *tree,
				BPluskey_t *key);
				
extern void BPlusTree_Query_Range(BPlusTree_t *, 
								BPluskey_t *, BPluskey_t *);

extern void  BPlusTree_Modify(
			 BPlusTree_t *,
			 BPluskey_t * key, void* value);

extern void BPlusTree_Delete(
			 BPlusTree_t *,
			 BPluskey_t *);

#endif
