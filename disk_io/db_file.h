#ifndef __DISK_FILE_MGMT__
#define __DISK_FILE_MGMT__

#define DB_PAGE_DEF_SIZE    1024
#define DB_FILE_DEF_PAGE_CNT    1024

#include "disk_io.h"

typedef uint64_t pg_offset_t;

fd_t 
db_file_create_db_file (const char* path, int flags, uint64_t size);

pg_offset_t 
db_file_alloc_db_page (fd_t fd);

bool 
db_file_free_db_page (fd_t fd, pg_offset_t offset);

void 
db_file_close_db_file (fd_t fd);

void 
db_file_destroy_db_file (fd_t fd);

#endif 