#include <assert.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include "mem.h"
#include "mem_allocator.h"
#include "../disk_io/pager.h"
#include "../gluethread/glthread.h"
#include "../c-hashtable/hashtable.h"
#include "../c-hashtable/hashtable_itr.h"

extern glthread_t free_block_list_head;

#define HASH_PRIME_CONST    5381

hashtable_t *pg_mapped_addr_to_pg_no_ht;
hashtable_t *pg_pgno_to_mapped_addr_ht;

static unsigned int
hashfromkey(void *key) {

    unsigned char *str = (unsigned char *)key;
    unsigned int hash = HASH_PRIME_CONST;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static int
equalkeys(void *k1, void *k2)
{
    char *ky1 = (char *)k1;
    char *ky2 = (char *)k2;
    int len1 = strlen(ky1);
    int len2 = strlen(ky2);
    if (len1 != len2) return len1 - len2;
    return (0 == memcmp(k1,k2, len1));
}

void  mem_mgr_init () ;
void  mem_mgr_destroy () ;

void  mem_mgr_init ()  {

    pg_mapped_addr_to_pg_no_ht = create_hashtable(sizeof (uintptr_t), hashfromkey, equalkeys);
    pg_pgno_to_mapped_addr_ht = create_hashtable(sizeof (uintptr_t), hashfromkey, equalkeys);
}

void mem_mgr_destroy () {

}

static void 
pg_mapped_addr_to_pg_no_ht_insert (void *mapped_addr, pg_no_t pg_no) {

    unsigned char *key = (unsigned char *) calloc (1, sizeof (uintptr_t));
    memcpy ( (char *)key,  (char *)&mapped_addr,  sizeof(uintptr_t));
    assert (!hashtable_insert (pg_mapped_addr_to_pg_no_ht, (void *)key, (void *)&pg_no));
}

static void 
pg_pgno_to_mapped_addr_ht_insert (pg_no_t pg_no, void *mapped_addr) {

    unsigned char *key = (unsigned char *) calloc (1, sizeof (uintptr_t));
    memcpy ( (char *)key,  (char *)&pg_no,  sizeof(uintptr_t));
    assert (!hashtable_insert (pg_pgno_to_mapped_addr_ht, (void *)key, (void *)&mapped_addr));
}

uint64_t
uapi_mem_alloc (fd_t fd, uint32_t req_size, void **ptr) {

    glthread_t *curr;
    block_meta_data_t *block_meta_data;

    curr = glthread_get_next (&free_block_list_head);
    
    if (!curr) {
        /* Reserve a new DB page in DB file on disk*/
        pg_no_t db_pg = db_file_alloc_db_page (fd);
        assert (db_pg != INVALID_DB_PG_NO);

        /* mmap the DB page into process's VM*/
        void *db_pg_ptr = db_file_mmap_db_page (fd, db_pg);

        /* Update Hashtables*/
        pg_mapped_addr_to_pg_no_ht_insert (db_pg_ptr, db_pg);
        pg_pgno_to_mapped_addr_ht_insert (db_pg, db_pg_ptr);

        /* intialize the DB page loaded in memory for memory allocation
            by the application*/
        allocator_init (db_pg_ptr, DB_PAGE_DEF_SIZE);

        curr = glthread_get_next (&free_block_list_head);
    }

    if (!curr) {

        /* Even a new DB page could not satisfy request, appln request cannot be 
            satisfied, consider increase the size of macro DB_PAGE_DEF_SIZE to higher value*/
            assert (0);
    }

    block_meta_data = pq_glue_to_block_meta_data (curr);

    *ptr = allocator_alloc_mem ((void *)block_meta_data->base_address, req_size);
    assert(*ptr);

    pg_no_t pg_no = (pg_no_t)hashtable_search (pg_mapped_addr_to_pg_no_ht, ptr);

    pg_offset_t pg_offset = db_page_get_offset (pg_no);

    return pg_offset + (uint64_t) ((char *) *ptr - (char *)block_meta_data->base_address);
}

void 
uapi_mem_free (fd_t fd, uint64_t disk_addr) {

    pg_no_t pg_no = db_get_container_page_from_offset (disk_addr);

    assert (pg_no != INVALID_DB_PG_NO);

    pg_offset_t pg_offset =  db_page_get_offset (pg_no);

    uint64_t offset_within_page = disk_addr - pg_offset;
 
    /*Check of the disk page is mapped in main memory*/
    void *page_mapped_addr = 
        (void *)hashtable_search (pg_pgno_to_mapped_addr_ht, (void *)&pg_no);

    if (!page_mapped_addr) {
        /* DB Page is not present in memory, load it*/
        page_mapped_addr = db_file_mmap_db_page (fd, pg_no);
    }

    /* Disk page is loaded in memory, release the object from main memory loaded page*/
    void *object_addr = (void *)(char *)page_mapped_addr + offset_within_page;

    allocator_free_mem(page_mapped_addr, object_addr);

    if (allocator_is_vm_page_empty(page_mapped_addr)) {

        allocator_deinit(page_mapped_addr);
        db_file_munmap_db_page(page_mapped_addr);
        db_file_free_db_page(fd, pg_no);
        hashtable_remove(pg_mapped_addr_to_pg_no_ht, &page_mapped_addr);
        hashtable_remove(pg_pgno_to_mapped_addr_ht, &pg_no);
    }
}

void *
uapi_get_vm_addr (fd_t fd, uint64_t disk_addr) {

    pg_no_t pg_no = db_get_container_page_from_offset (disk_addr);

    assert (pg_no != INVALID_DB_PG_NO);

    pg_offset_t pg_offset =  db_page_get_offset (pg_no);

    uint64_t offset_within_page = disk_addr - pg_offset;
 
    /*Check of the disk page is mapped in main memory*/
    void *page_mapped_addr = 
        (void *)hashtable_search (pg_pgno_to_mapped_addr_ht, (void *)&pg_no);

    if (!page_mapped_addr) {
        /* DB Page is not present in memory, load it*/
        page_mapped_addr = db_file_mmap_db_page (fd, pg_no);
    }

    /* Disk page is loaded in memory, release the object from main memory loaded page*/
    void *object_addr = (void *)(char *)page_mapped_addr + offset_within_page;

    return object_addr;
}