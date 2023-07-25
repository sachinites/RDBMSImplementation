#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <assert.h>
#include "bptree_mem.h"

#ifdef IN_MEMORY

bptree_node_t *bptree_alloc_node () {

    return (bptree_node_t *)calloc (1, sizeof (bptree_node_t));
}

void bptree_free_node (bptree_node_t *node) {

    free(node);
}

#else 

static fd_t fd;

file_addr_t
bptree_alloc_node (fd_t fd, bptree_node_t **ptr) {

    file_addr_t faddr =  (file_addr_t) uapi_mem_alloc (fd, sizeof (bptree_node_t ), (void **)ptr);
    #if 0
    assert (!fsync_range (fd, FSYNC_SYNC | FSYNC_DATA | FSYNC_WAIT, 
        faddr - sizeof (block_meta_data_t),
        sizeof (block_meta_data_t) + 
        sizeof (bptree_node_t) + 
        sizeof (block_meta_data_t)));
    #endif 
    return faddr;
}

void 
bptree_free_node (fd_t fd, file_addr_t faddr) {

    uint32_t mem_freed = uapi_mem_free (fd, faddr);
    #if 0
    assert (!fsync_range (fd, FSYNC_SYNC | FSYNC_DATA | FSYNC_WAIT, 
        /* We are optimistic here, covering worst case scenario that
        freeing the memory could also merge the upper and lower free 
        already freed blocks */
        faddr - mem_freed - sizeof (block_meta_data_t), 
        mem_freed +  sizeof (block_meta_data_t)));
    #endif 
}

void 
bptree_node_fsync(fd_t fd, file_addr_t faddr) {

    #if 0
    assert (!fsync_range (fd, FSYNC_SYNC | FSYNC_DATA | FSYNC_WAIT, faddr , sizeof (bptree_node_t)));
    #endif
}

#endif 