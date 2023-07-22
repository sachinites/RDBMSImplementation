#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "disk_io/pager.h"
#include "mem_allocator/mem_allocator.h"

typedef struct stud_ {

    char name[32];
} stud_t;

int 
main (int argc, char **argv) {

    /* Create a DB file on disk*/
    db_file_create_db_file ("sample1.db", 0);
    /* Open the DB file*/
    fd_t fd = db_file_open ("sample1.db");
    /* Allocate the DB page to use*/
    pg_no_t db_pg1 = db_file_alloc_db_page (fd);
    assert (db_pg1 != INVALID_DB_PG_NO);

    /* mmap the DB page into process's VM*/
    void *db_pg1_ptr = db_file_mmap_db_page (fd, db_pg1);
    /* initialize the allocator to request memory for objects from
        DB page*/
    allocator_init (db_pg1_ptr, DB_PAGE_DEF_SIZE);

    /* Lets allocate memort for student object and initialize it*/
    stud_t *stud1 = (stud_t *)allocator_alloc_mem (db_pg1_ptr, sizeof(stud_t));
    strncpy (stud1->name, "Abhishek", sizeof(stud1->name));

    /* We re done, destory the mapping */
    db_file_munmap_db_page (db_pg1_ptr );

    /*close the DB file and exit the application */
    close (fd);
    return 0;
}