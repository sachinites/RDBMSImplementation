#include <unistd.h>

#include <assert.h>
#include <sys/stat.h>
#include "pager.h"

/* Offset to page no & Vice-versa conversion functions*/
pg_offset_t
db_page_get_offset (pg_no_t pg_no) {

    pg_offset_t offset = 0;

    offset += sizeof (db_file_hdr_t);

    if (pg_no == 0) {

        return offset;
    }

    offset +=  (pg_no  * DB_PAGE_DEF_SIZE);
    return offset;
}

pg_no_t
db_get_page_from_offset (pg_offset_t offset) {

    pg_no_t pg_no;

    offset -= sizeof (db_file_hdr_t);

    assert (offset >= DB_PAGE_DEF_SIZE);

    pg_no = offset / DB_PAGE_DEF_SIZE;

    assert (pg_no <= DB_FILE_DEF_PAGE_CNT);

    return pg_no;
}

static void 
db_file_init_hdr (fd_t fd) {

    int i;

    db_file_hdr_t *db_hdr = (db_file_hdr_t *)disk_io_file_mmap 
                                                            (fd, 0, sizeof (db_file_hdr_t));

    for (i = 0 ; i < free_pg_bitmap_size; i++) {
        db_hdr->allocated_pg_bitmap[i] = 0;
    }

    disk_file_unmap ( (void *)db_hdr, sizeof (db_file_hdr_t));
}

void
db_file_create_db_file (const char* path, uint64_t size) {

    fd_t fd;

    disk_io_create_disk_file (path, 
        sizeof (db_file_hdr_t) + 
        (DB_FILE_DEF_PAGE_CNT *  DB_PAGE_DEF_SIZE));

    fd = db_file_open  (path);

    /* initialize the Hdr of the DB file*/
    db_file_init_hdr (fd);

    close (fd);
}

fd_t 
db_file_open (const char* path) {

    return disk_io_open_file (path);
}

pg_no_t
db_file_alloc_db_page (fd_t fd) {

    int i;
    pg_no_t pos = 0;
    uint64_t var;
    uint64_t msb = 1 << 63;

    db_file_hdr_t *db_hdr = (db_file_hdr_t *)disk_io_file_mmap 
                                                            (fd, 0, sizeof (db_file_hdr_t));
    
    for (i = 0;  i < free_pg_bitmap_size; i++) {

        if (db_hdr->allocated_pg_bitmap[i] == UINT64_MAX) {
            
            pos += 64;
            continue;
        }

        var = db_hdr->allocated_pg_bitmap[i];
            
        /* Find the Ist zeroth bit */
        while (var & msb) {
            var = var << 1;
            pos++;
        }
        
        db_hdr->allocated_pg_bitmap[i] |= (1 << (63 - (pos % 64) ));
        disk_file_unmap ( (void *)db_hdr, sizeof (db_file_hdr_t));
        return pos;
    }

    disk_file_unmap ( (void *)db_hdr, sizeof (db_file_hdr_t));
    return INVALID_DB_PG_NO;
}

bool 
db_file_free_db_page (fd_t fd, pg_no_t pg_no) {

    db_file_hdr_t *db_hdr = (db_file_hdr_t *)disk_io_file_mmap 
                                                            (fd, 0, sizeof (db_file_hdr_t));

    int block_no = pg_no / 64;
    int bit_no = 63 - (pg_no % 64);

    db_hdr->allocated_pg_bitmap[block_no] &= ~(1 << bit_no);
    disk_file_unmap ( (void *)db_hdr, sizeof (db_file_hdr_t));
    return true;
}

void *
db_file_mmap_db_page (fd_t fd, pg_no_t pg_no) {

    pg_offset_t offset = db_page_get_offset (pg_no);
    return disk_io_file_mmap (fd, offset, offset + DB_PAGE_DEF_SIZE );
}

void 
db_file_munmap_db_page (void *addr) {

    disk_file_unmap (addr, DB_PAGE_DEF_SIZE);
}

pg_no_t
db_get_container_page_from_offset (uint64_t db_file_offset) {


    if (db_file_offset < sizeof (db_file_hdr_t)) return INVALID_DB_PG_NO;
    pg_no_t pg_no = (db_file_offset / DB_PAGE_DEF_SIZE ) - 1;
    if ((db_file_offset % DB_PAGE_DEF_SIZE) >= sizeof (db_file_hdr_t)) pg_no += 1;
    return pg_no;
}


void 
db_file_print_stats (fd_t fd) {

    int i, j = 0;
    uint64_t var;
    uint64_t msb = 1 << 63;
    pg_no_t pg_no = 0;

    db_file_hdr_t *db_hdr = (db_file_hdr_t *)disk_io_file_mmap 
                                                            (fd, 0, sizeof (db_file_hdr_t));

    for (i = 0 ; i < free_pg_bitmap_size; i++) {

        var = db_hdr->allocated_pg_bitmap[i];
        
        for (j = 0 ; j < 64; j++) {
            
            if (var & msb) {
                printf ("Page no %llu in use\n", pg_no);
            }
            var = var << 1;
            pg_no++;
        }
    }

    disk_file_unmap ( (void *)db_hdr, sizeof (db_file_hdr_t));
}