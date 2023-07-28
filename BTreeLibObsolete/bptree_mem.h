#ifndef __BPTREE_MEM__
#define __BPTREE_MEM__

#include <stdbool.h>
#include "../mem_allocator/mem.h"

/* Secondary storage file descriptor */

#ifndef IN_MEMORY
#define DISK_MEMORY
#endif 

#define BTREE_FANOUT    3

typedef struct bptree_node_ {

    bool is_leaf;
    #ifdef IN_MEMORY
    struct btree_node_ *child[(BTREE_FANOUT * 2) ];
    #else 
    file_addr_t child[(BTREE_FANOUT * 2)];
    #endif
    unsigned char **key[(BTREE_FANOUT * 2) - 1];

} bptree_node_t;

#ifdef IN_MEMORY

bptree_node_t *bptree_alloc_node ();
void bptree_free_node (bptree_node_t *node);

#else 

file_addr_t
bptree_alloc_node (fd_t fd, bptree_node_t **ptr);

void 
bptree_free_node (fd_t fd, file_addr_t faddr);

void 
bptree_node_fsync(fd_t fd, file_addr_t faddr);

#endif 


#endif