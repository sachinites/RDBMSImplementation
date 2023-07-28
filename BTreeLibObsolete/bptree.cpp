
#undef IN_MEMORY

#include <unistd.h>
#include "bptree_mem.h"


/* Nice Animation Here : 
    https://www.cs.usfca.edu/~galles/visualization/BPlusTree.html
*/
typedef struct bptree_node_ptr_ {

     bptree_node_t *ptr;
     file_addr_t faddr;

}  bptree_node_ptr_t;


#define DEF(bptree_node_var)    bptree_node_ptr_t bptree_node_var = {NULL, 0}
#define BPTREE_NODE_ALLOC(_fd, node_var) \
    node_var.faddr = bptree_alloc_node (_fd, &node_var.ptr)
#define BPTREE_NODE_FREE(_fd, node_var) \
    bptree_free_node (_fd, node_var.faddr)
#define BPTREE_NODE_SYNC(_fd, node_var) \
    bptree_node_fsync(fd, node_var.faddr)

int 
main (int argc, char **argv) {

    db_file_create_db_file ("bptree.db", 0);
    fd_t fd = db_file_open ("bptree.db");

    DEF(root);
    DEF(root1);
    DEF(root2);
    
    BPTREE_NODE_ALLOC(fd, root);
    BPTREE_NODE_ALLOC(fd, root1);
    BPTREE_NODE_ALLOC(fd, root2);

    fdatasync (fd);

    BPTREE_NODE_FREE (fd, root);
    BPTREE_NODE_FREE (fd, root1);
    BPTREE_NODE_FREE (fd, root2);

    fdatasync (fd);
    return 0;
}