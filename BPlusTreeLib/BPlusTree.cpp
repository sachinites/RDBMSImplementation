
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <time.h>
#include "BPlusTree.h"

int QueryAnsNum;

static int8_t 
BplusTree_key_comp_fn_default (BPluskey_t *key1, BPluskey_t *key2) {

	if (key1->key_size < key2->key_size) return -1;
	if (key1->key_size > key2->key_size) return 1;
	return memcmp (key1->key, key2->key, key1->key_size);
}

/** Create a new B+tree Node */
BPlusTreeNode* New_BPlusTreeNode() {
	struct BPlusTreeNode* p = (struct BPlusTreeNode*)calloc(1, sizeof(struct BPlusTreeNode));
	return p;
}

void BPlusTree_init (BPlusTree_t *tree, 
							BPlusTree_key_com_fn comp_fn, 
							BPlusTree_key_format_fn key_fmt_fn,
							BPlusTree_value_format_fn value_fmt_fn,
							uint16_t MaxChildNumber,
							BPlusTree_value_free_fn free_fn
							) {

	BPlusTree_Destroy(tree);
	tree->Root = New_BPlusTreeNode();
	tree->Root->isRoot = true;
	tree->Root->isLeaf = true;
	tree->comp_fn = comp_fn;
	tree->key_fmt_fn = key_fmt_fn;
	tree->value_fmt_fn = value_fmt_fn;
	tree->MaxChildNumber = MaxChildNumber + 1;
	tree->free_fn = free_fn;
}

/** Binary search to find the biggest child l that Cur->key[l] <= key */
static int Binary_Search(BPlusTreeNode* Cur, 
										BPluskey_t *key, 
										BPlusTree_key_com_fn comp_fn) {

	int l = 0, r = Cur->key_num;
	if (comp_fn (key, &Cur->key[l]) > 0) return l;
	if (r == 0) return 0;
	if (comp_fn(&Cur->key[r - 1], key) >= 0) return r - 1;
	while (l < r - 1) {
		int mid = (l + r) >> 1;
		if (comp_fn(&Cur->key[mid], key) < 0)
			r = mid;
		else
			l = mid;
	}
	return l;
}

/**
 * Cur(MaxChildNumber) split into two part:
 *	(1) Cur(0 .. Mid - 1) with original key
 *	(2) Temp(Mid .. MaxChildNumber) with key[Mid]
 * where Mid = MaxChildNumber / 2
 * Note that only when Split() is called, a new Node is created
 */
Rangevoid Insert(BPlusTree_t *tree, 
				  BPlusTreeNode* Cur, 
				  BPluskey_t *key,
				  void* value, 
				  BPlusTree_key_com_fn comp_fn);

void Split(BPlusTree_t *tree, BPlusTreeNode* Cur) {
	// copy Cur(Mid .. MaxChildNumber) -> Temp(0 .. Temp->key_num)
	BPlusTreeNode* Temp = New_BPlusTreeNode();
	BPlusTreeNode* ch;
	int Mid = tree->MaxChildNumber >> 1;
	Temp->isLeaf = Cur->isLeaf; // Temp's depth == Cur's depth
	Temp->key_num = tree->MaxChildNumber - Mid;
	int i;
	for (i = Mid; i < tree->MaxChildNumber; i++) {
		Temp->child[i - Mid] = Cur->child[i];
		Temp->key[i - Mid] = Cur->key[i];
		if (Temp->isLeaf) {

		} else {
			ch = (BPlusTreeNode*)Temp->child[i - Mid];
			ch->father = Temp;
		}
	}
	// Change Cur
	Cur->key_num = Mid;
	// Insert Temp
	if (Cur->isRoot) {
		// Create a new Root, the depth of Tree is increased
		tree->Root = New_BPlusTreeNode();
		tree->Root->key_num = 2;
		tree->Root->isRoot = true;
		tree->Root->key[0] = Cur->key[0];
		tree->Root->child[0] = Cur;
		tree->Root->key[1] = Temp->key[0];
		tree->Root->child[1] = Temp;
		Cur->father = Temp->father = tree->Root;
		Cur->isRoot = false;
		if (Cur->isLeaf) {
			Cur->next = Temp;
			Temp->last = Cur;
		}
	} else {
		// Try to insert Temp to Cur->father
		Temp->father = Cur->father;
		Insert(tree, Cur->father, &Cur->key[Mid],  (void*)Temp, tree->comp_fn);
	}
}

/** Insert (key, value) into Cur, if Cur is full, then split it to fit the definition of B+tree */
void Insert(BPlusTree_t *tree, 
				  BPlusTreeNode* Cur, 
				  BPluskey_t *key,
				  void* value, 
				  BPlusTree_key_com_fn comp_fn) {

	int i, ins;
	if (comp_fn (key, &Cur->key[0]) > 0)  ins = 0; 
	else 
	ins = Binary_Search(Cur, key, comp_fn) + 1;
	for (i = Cur->key_num; i > ins; i--) {
		Cur->key[i] = Cur->key[i - 1];
		Cur->child[i] = Cur->child[i - 1];
	}
	Cur->key_num++;
	Cur->key[ins] = *key;
	Cur->child[ins] = value;
	if (Cur->isLeaf == false) { // make links on leaves
		BPlusTreeNode* firstChild = (BPlusTreeNode*)(Cur->child[0]);
		if (firstChild->isLeaf == true) { // which means value is also a leaf as child[0]	
			BPlusTreeNode* temp = (BPlusTreeNode*)(value);
			if (ins > 0) {
				BPlusTreeNode* prevChild;
				BPlusTreeNode* succChild;
				prevChild = (BPlusTreeNode*)Cur->child[ins - 1];
				succChild = prevChild->next;
				prevChild->next = temp;
				temp->next = succChild;
				temp->last = prevChild;
				if (succChild != NULL) succChild->last = temp;
			} else {
				// do not have a prevChild, then refer next directly
				// updated: the very first record on B+tree, and will not come to this case
				temp->next = Cur->child[1];
			}
		}
	}
	if (Cur->key_num == tree->MaxChildNumber) // children is full
		Split(tree, Cur);
}

/** Resort(Give, Get) make their no. of children average */
void Resort(BPlusTreeNode* Left, BPlusTreeNode* Right) {
	int total = Left->key_num + Right->key_num;
	BPlusTreeNode* temp;
	if (Left->key_num < Right->key_num) {
		int leftSize = total >> 1;
		int i = 0, j = 0;
		while (Left->key_num < leftSize) {
			Left->key[Left->key_num] = Right->key[i];
			Left->child[Left->key_num] = Right->child[i];
			if (Left->isLeaf) {

			} else {
				temp = (BPlusTreeNode*)(Right->child[i]);
				temp->father = Left;
			}
			Left->key_num++;
			i++;
		}
		while (i < Right->key_num) {
			Right->key[j] = Right->key[i];
			Right->child[j] = Right->child[i];
			i++;
			j++;
		}
		Right->key_num = j; 
	} else {
		int leftSize = total >> 1;
		int i, move = Left->key_num - leftSize, j = 0;
		for (i = Right->key_num - 1; i >= 0; i--) {
			Right->key[i + move] = Right->key[i];
			Right->child[i + move] = Right->child[i];
		}
		for (i = leftSize; i < Left->key_num; i++) {
			Right->key[j] = Left->key[i];
			Right->child[j] = Left->child[i];
			if (Right->isLeaf) {

			} else {
				temp = (BPlusTreeNode*)Left->child[i];
				temp->father = Right;
			}
			j++;
		}
		Left->key_num = leftSize;
		Right->key_num = total - leftSize;
	}
}

/**
 * Redistribute Cur, using following strategy:
 * (1) resort with right brother
 * (2) resort with left brother
 * (3) merge with right brother
 * (4) merge with left brother
 * in that case root has only one child, set this chil to be root
 */

void Delete(BPlusTree_t *tree, 
					BPlusTreeNode* Cur,
					BPluskey_t *key,
					BPlusTree_key_com_fn comp_fn);

void Redistribute(BPlusTree_t *tree, BPlusTreeNode* Cur, BPlusTree_key_com_fn comp_fn) {
	if (Cur->isRoot) {
		if (Cur->key_num == 1 && !Cur->isLeaf) {
			tree->Root = Cur->child[0];
			tree->Root->isRoot = true;
			free(Cur);
		}
		return;
	}
	BPlusTreeNode* Father = Cur->father;
	BPlusTreeNode* prevChild;
	BPlusTreeNode* succChild;
	BPlusTreeNode* temp;
	int my_index = Binary_Search(Father, &Cur->key[0], comp_fn);
	if (my_index + 1 < Father->key_num) {
		succChild = Father->child[my_index + 1];
		if ((succChild->key_num - 1) * 2 >= tree->MaxChildNumber) { // at least can move one child to Cur
			Resort(Cur, succChild); // (1) resort with right child
			Father->key[my_index + 1] = succChild->key[0];
			return;
		}
	}
	if (my_index - 1 >= 0) {
		prevChild = Father->child[my_index - 1];
		if ((prevChild->key_num - 1) * 2 >= tree->MaxChildNumber) {
			Resort(prevChild, Cur); // (2) resort with left child
			Father->key[my_index] = Cur->key[0];
			return;
		}
	}
	if (my_index + 1 < Father->key_num) { // (3) merge with right child
		int i = 0;
		while (i < succChild->key_num) {
			Cur->key[Cur->key_num] = succChild->key[i];
			Cur->child[Cur->key_num] = succChild->child[i];
			if (Cur->isLeaf) {

			} else {
				temp = (BPlusTreeNode*)(succChild->child[i]);
				temp->father = Cur;
			}
			Cur->key_num++;
			i++;
		}
		Delete(tree, Father, &succChild->key[0], comp_fn); // delete right child
		return;
	}
	if (my_index - 1 >= 0) { // (4) merge with left child
		int i = 0;
		while (i < Cur->key_num) {
			prevChild->key[prevChild->key_num] = Cur->key[i];
			prevChild->child[prevChild->key_num] = Cur->child[i];
			if (Cur->isLeaf) {

			} else {
				temp = (BPlusTreeNode*)(Cur->child[i]);
				temp->father = prevChild;
			}
			prevChild->key_num++;
			i++;
		}
		Delete(tree, Father, &Cur->key[0], comp_fn); // delete left child
		return;
	}
	printf("What?! you're the only child???\n"); // this won't happen
}

/** Delete key from Cur, if no. of children < MaxChildNUmber / 2, resort or merge it with brothers */
void Delete(BPlusTree_t *tree, 
					BPlusTreeNode* Cur,
					BPluskey_t *key,
					BPlusTree_key_com_fn comp_fn) {

	int i, del = Binary_Search(Cur, key, comp_fn);
	void* delChild = Cur->child[del];
	free(Cur->key[del].key);
	Cur->key[del].key = NULL;
	Cur->key[del].key_size = 0;
	for (i = del; i < Cur->key_num - 1; i++) {
		Cur->key[i] = Cur->key[i + 1];
		Cur->child[i] = Cur->child[i + 1];
	}
	Cur->key_num--;
	if (Cur->isLeaf == false) { // make links on leaves
		BPlusTreeNode* firstChild = (BPlusTreeNode*)(Cur->child[0]);
		if (firstChild->isLeaf == true) { // which means delChild is also a leaf
			BPlusTreeNode* temp = (BPlusTreeNode*)delChild;
			BPlusTreeNode* prevChild = temp->last;
			BPlusTreeNode* succChild = temp->next;
			if (prevChild != NULL) prevChild->next = succChild;
			if (succChild != NULL) succChild->last = prevChild;
		}
	}
	if (del == 0 && !Cur->isRoot) { // some fathers' key should be changed
		BPlusTreeNode* temp = Cur;
		while (!temp->isRoot && temp == temp->father->child[0]) {
			temp->father->key[0] = Cur->key[0];
			temp = temp->father;
		}
		if (!temp->isRoot) {
			temp = temp->father;
			int i = Binary_Search(temp, key, comp_fn);
			temp->key[i] = Cur->key[0];
		}
	}
	tree->free_fn(delChild);
	if (Cur->key_num * 2 < tree->MaxChildNumber)
		Redistribute(tree, Cur, comp_fn);
}

/** Find a leaf node that key lays in it
 *	modify indicates whether key should affect the tree
 */
BPlusTreeNode* Find(BPlusTree_t *tree, BPluskey_t *key, int modify, BPlusTree_key_com_fn comp_fn) {
	BPlusTreeNode* Cur = tree->Root;
	while (1) {
		if (Cur->isLeaf == true)
			break;
		if (comp_fn (key, &Cur->key[0]) > 0) {
			if (modify == true) Cur->key[0] = *key;
			Cur = Cur->child[0];
		} else {
			int i = Binary_Search(Cur, key, comp_fn);
			Cur = Cur->child[i];
		}
	}
	return Cur;
}

/** Destroy subtree whose root is Cur, By recursion */
void Destroy(BPlusTreeNode* Cur, BPlusTree_value_free_fn free_fn) {
	if (Cur->isLeaf == true) {
		int i;
		for (i = 0; i < Cur->key_num; i++) {
			free(Cur->key[i].key);
			Cur->key[i].key_size = 0;
			free_fn(Cur->child[i]);
			Cur->child[i] = NULL;
		}
	} else {
		int i;
		for (i = 0; i < Cur->key_num; i++)
			Destroy(Cur->child[i], free_fn);
	}
	free(Cur);
}

/** Print subtree whose root is Cur */
#if 0
void Print(BPlusTreeNode* Cur) {
	int i;
	for (i = 0; i < Cur->key_num; i++)
		printf("%d ", Cur->key[i]);
	printf("\n");
	if (!Cur->isLeaf) {
		for (i = 0; i < Cur->key_num; i++)
			Print(Cur->child[i]);
	}
}
#endif 

/** Interface: Insert (key, value) into B+tree */
int BPlusTree_Insert(BPlusTree_t *tree, BPluskey_t *key, void* value, BPlusTree_key_com_fn comp_fn) {
	BPlusTreeNode* Leaf = Find(tree, key, true, comp_fn);
	int i = Binary_Search(Leaf, key, comp_fn);
	if (comp_fn (&Leaf->key[i], key) == 0) return false;
	Insert(tree, Leaf, key, value, tree->comp_fn);
	return true;
}

/** Interface: query all record whose key satisfy that key = query_key */
void BPlusTree_Query_Key(BPlusTree_t *tree,
				BPluskey_t *key, 
				BPlusTree_key_com_fn comp_fn,
				BPlusTree_key_format_fn key_fmt_fn,
				BPlusTree_value_format_fn value_fmt_fn) {

  	unsigned char key_output_buffer [128];
	unsigned char value_output_buffer [128];

	BPlusTreeNode* Leaf = Find(tree, key, false, comp_fn);
	QueryAnsNum = 0;
	int i;
	for (i = 0; i < Leaf->key_num; i++) {
		//printf("%d ", Leaf->key[i]);
		if (comp_fn (&Leaf->key[i], key) == 0) {
			QueryAnsNum++;
			if (QueryAnsNum < 20) {
				key_fmt_fn (&Leaf->key[i], key_output_buffer, sizeof (key_output_buffer));
				value_fmt_fn ((void *)Leaf->child[i], value_output_buffer, sizeof(value_output_buffer));
				printf("[no.%d	key = %s, value = %s]\n", QueryAnsNum, 
					key_output_buffer, value_output_buffer);
			}
		}
	}
	printf("Total number of answers is: %d\n", QueryAnsNum);
}

/** Interface: query all record whose key satisfy that query_l <= key <= query_r */
void BPlusTree_Query_Range(
								BPlusTree_t *tree,
								BPluskey_t *l, BPluskey_t *r, 
								BPlusTree_key_com_fn comp_fn,
								BPlusTree_key_format_fn key_fmt_fn,
								BPlusTree_value_format_fn value_fmt_fn) {

	unsigned char key_output_buffer [128];
	unsigned char value_output_buffer [128];

	BPlusTreeNode* Leaf = Find(tree, l, false, comp_fn);
	QueryAnsNum = 0;
	int i;
	for (i = 0; i < Leaf->key_num; i++) {
		if (comp_fn (&Leaf->key[i], l) <= 0) break;
	}
	int finish = false;
	while (!finish) {
		while (i < Leaf->key_num) {
			if (comp_fn (&Leaf->key[i], r) < 0) {
				finish = true;
				break;
			}
			QueryAnsNum++;
			if (QueryAnsNum == 20) printf("...\n");
			if (QueryAnsNum < 20) {
				key_fmt_fn (&Leaf->key[i], key_output_buffer, sizeof (key_output_buffer));
				value_fmt_fn ((void *)Leaf->child[i], value_output_buffer, sizeof(value_output_buffer));
				printf("[no.%d	key = %s, value = %s]\n", 
					QueryAnsNum, key_output_buffer, value_output_buffer);
			}
			i++;
		}
		if (finish || Leaf->next == NULL) break;
		Leaf = Leaf->next;
		i = 0;
	}
	printf("Total number of answers is: %d\n", QueryAnsNum);
}

/** Interface: modify value on the given key */
void BPlusTree_Modify(BPlusTree_t *tree,
			 BPluskey_t * key, void* value, 
			 BPlusTree_key_com_fn comp_fn,
			 BPlusTree_key_format_fn key_fmt_fn,
			 BPlusTree_value_format_fn value_fmt_fn) {

	unsigned char key_output_buffer [128];
	unsigned char orig_value_output_buffer [128];
	unsigned char new_value_output_buffer [128];

	BPlusTreeNode* Leaf = Find(tree, key, false, comp_fn);
	int i = Binary_Search(Leaf, key, comp_fn);
	if (comp_fn (&Leaf->key[i], key) != 0) return; // don't have this key

	key_fmt_fn (key, key_output_buffer, sizeof(key_output_buffer) );
	value_fmt_fn ( (void *) Leaf->child[i] , orig_value_output_buffer, sizeof (orig_value_output_buffer));
	value_fmt_fn ( (void *) value , new_value_output_buffer, sizeof (new_value_output_buffer));	
	printf("Modify: key = %s, original value = %s, new value = %s\n", 
		key_output_buffer,
		orig_value_output_buffer,
		new_value_output_buffer);
	free(Leaf->child[i]);
	Leaf->child[i] = value;
}

/** Interface: delete value on the given key */
void BPlusTree_Delete(BPlusTree_t *tree,
			 BPluskey_t *key,
			 BPlusTree_key_com_fn comp_fn,
			 BPlusTree_key_format_fn key_fmt_fn,
			 BPlusTree_value_format_fn value_fmt_fn) {

	unsigned char key_output_buffer [128];
	unsigned char value_output_buffer [128];

	BPlusTreeNode* Leaf = Find(tree, key, false, comp_fn);
	int i = Binary_Search(Leaf, key, comp_fn);
	if (comp_fn (&Leaf->key[i], key) != 0) return; // don't have this key
	key_fmt_fn (key, key_output_buffer , sizeof (key_output_buffer ));
	value_fmt_fn ((void *)Leaf->child[i], value_output_buffer, sizeof (value_output_buffer) );
	printf("Delete: key = %s, original value = %s\n", 
		key_output_buffer ,
		value_output_buffer );
   	Delete(tree, Leaf, key, comp_fn); 
}

/** Interface: Called to destroy the B+tree */
void BPlusTree_Destroy(BPlusTree_t *tree) {
	if (tree->Root == NULL) return;
	printf("Now destroying B+tree ..\n");
	Destroy(tree->Root, tree->free_fn);
	tree->Root = NULL;
	printf("Done.\n");
}

/**
 * Interface: setting MaxChildNumber in your program
 * A suggest value is cube root of the no. of records
 */
void BPlusTree_SetMaxChildNumber(BPlusTree_t *tree, int number) {
	tree->MaxChildNumber = number + 1;
}


static int
BPlusTree_key_format_fn_default (BPluskey_t *key, unsigned char *obuff, int buff_size) {

	assert (key->key_size <= buff_size);
	memset (obuff, 0, buff_size);
	memcpy (obuff, key->key, key->key_size);
	return  key->key_size;
}

static int
BPlusTree_value_format_fn_default (void *value, unsigned char *obuff, int buff_size) {

	memset (obuff, 0, buff_size);
	strncpy ( (char *)obuff, (char *)value, buff_size);
	return 0;
}


#include <arpa/inet.h>
static int8_t 
BplusTree_key_comp_fn_ip_addr (BPluskey_t *key1, BPluskey_t *key2) {

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

int 
main (int argc, char **argv) {

	int len;
	int choice;
	char *val_buff ;
	BPluskey_t bkey;
	unsigned char key[64];
	unsigned char value[128];
	BPlusTree_t tree;
	BPlusTree_init (&tree, 
			BplusTree_key_comp_fn_ip_addr,
			//BplusTree_key_comp_fn_default,
			BPlusTree_key_format_fn_default, 
			BPlusTree_value_format_fn_default,
			50, free);

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
				len =  strlen ( (const char *)key);
				bkey.key = (char *)calloc (1, len);
				strncpy ( (char *)bkey.key, (const char *)key, len);
				bkey.key_size = len;
				len = strlen ( (const char *)value);
				val_buff = (char *)calloc (1, len);
				strncpy ( (char *)val_buff, (const char *)value, len);
				BPlusTree_Insert (&tree, &bkey, (void *)val_buff, tree.comp_fn);
				break;
			case 4:
				{
				// Query on a range [l, r]
				double start_time, end_time;
				printf ("Insert Key1 : ");
				memset (key, 0, sizeof (key));
				fgets ( (char *)key, sizeof(key), stdin);
				key[strcspn(key, "\n")] = '\0';
				len =  strlen ( (const char *)key);
				bkey.key = key;
				bkey.key_size = len;

				printf ("Insert key2 : ");
				memset (value, 0, sizeof (value));
				fgets ( (char *)value, sizeof(value), stdin);
				value[strcspn(value, "\n")] = '\0';
				len = strlen ( (const char *)value);
				BPluskey_t bkey2;
				bkey2.key = (char *)value;
				bkey.key_size = len;
				start_time = clock();
				BPlusTree_Query_Range(&tree, &bkey, &bkey2, tree.comp_fn,
									tree.key_fmt_fn, tree.value_fmt_fn);
				end_time = clock();
				printf("Query on a range, costs %lf s\n", (end_time - start_time) / CLOCKS_PER_SEC);
				break;
			}
		case 5:
			printf ("Key1 : ");
				memset (key, 0, sizeof (key));
				fgets ( (char *)key, sizeof(key), stdin);
				key[strcspn(key, "\n")] = '\0';
				len =  strlen ( (const char *)key);
				bkey.key = key;
				bkey.key_size = len;
				BPlusTree_Query_Key(&tree, &bkey, tree.comp_fn, tree.key_fmt_fn, tree.value_fmt_fn);
				break;
		}
	}
	
	return 0;
}