#ifndef __DISK_FILE_MGMT__
#define __DISK_FILE_MGMT__

#define DB_PAGE_DEF_SIZE    1024
#define DB_FILE_DEF_PAGE_CNT    64

#include "disk_io.h"

typedef uint64_t pg_offset_t;
typedef uint64_t pg_no_t;

#define INVALID_DB_PG_NO    UINT64_MAX

#define free_pg_bitmap_size \
    (DB_FILE_DEF_PAGE_CNT / 64)

#pragma pack (push,1)

typedef struct db_file_hdr_ {

    /* Most Significant bit is 0*/
    uint64_t allocated_pg_bitmap[free_pg_bitmap_size];

} db_file_hdr_t;

#pragma pack(pop)


void
db_file_create_db_file (const char* path,  uint64_t size);

fd_t 
db_file_open (const char* path);

pg_no_t
db_file_alloc_db_page (fd_t fd);

bool 
db_file_free_db_page (fd_t fd, pg_no_t pg_no);

void *
db_file_mmap_db_page (fd_t fd, pg_no_t pg_no);

void 
db_file_munmap_db_page (void *addr);

void 
db_file_print_stats (fd_t fd);

pg_offset_t
db_page_get_offset (pg_no_t pg_no) ;

pg_no_t
db_get_page_from_offset (pg_offset_t offset);

pg_no_t
db_get_container_page_from_offset (uint64_t db_file_offset);

#endif 