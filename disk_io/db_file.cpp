#include "db_file.h"
#include "gluethread/glthread.h"

typedef struct block_meta_data_{

    bool is_free;
    glthread_t priority_thread_glue;
    struct block_meta_data_ *prev_block;
    struct block_meta_data_ *next_block;
} block_meta_data_t;
GLTHREAD_TO_STRUCT(glthread_to_block_meta_data, 
    block_meta_data_t, priority_thread_glue);

#define NEXT_META_BLOCK(block_meta_data_ptr)    \
    (block_meta_data_ptr->next_block)

#define NEXT_META_BLOCK_BY_SIZE(block_meta_data_ptr)    \
    (block_meta_data_t *)((char *)(block_meta_data_ptr + 1) \
        + DB_PAGE_DEF_SIZE)

#define PREV_META_BLOCK(block_meta_data_ptr)    \
    (block_meta_data_ptr->prev_block)

#define mm_bind_blocks_for_allocation(allocated_meta_block, free_meta_block)  \
    free_meta_block->prev_block = allocated_meta_block;        \
    free_meta_block->next_block = allocated_meta_block->next_block;    \
    allocated_meta_block->next_block = free_meta_block;                \
    if (free_meta_block->next_block)\
    free_meta_block->next_block->prev_block = free_meta_block

#define mm_bind_blocks_for_deallocation(freed_meta_block_down, freed_meta_block_top)    \
    freed_meta_block_down->next_block = freed_meta_block_top->next_block;               \
    if(freed_meta_block_top->next_block)                                                \
    freed_meta_block_top->next_block->prev_block = freed_meta_block_down

fd_t 
db_file_create_db_file (const char* path, int flags, uint64_t size) {}

pg_offset_t 
db_file_alloc_db_page (fd_t fd) {}

bool 
db_file_free_db_page (fd_t fd, pg_offset_t offset) {}

void 
db_file_close_db_file (fd_t fd) {}

void 
db_file_destroy_db_file (fd_t fd) {}