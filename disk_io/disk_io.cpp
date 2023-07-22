#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include "disk_io.h"

void
disk_io_create_disk_file (const char* path, fsize size) {

    fd_t fd;
    
    FILE *file = fopen (path, "w+");

    if (!file) assert(0);

    fd = fileno (file);

    if (ftruncate(fd, size) == -1) {
        assert(0);
    }

    fclose (file);
}

fd_t 
disk_io_open_file (const char* path) {

    FILE *file = fopen (path, "w+");

    if (!file) assert(0);

    fd_t fd = fileno (file);

    return fd;
}

void
disk_io_delete_file (const char* path) {

    assert (unlink(path) == 0);
}

void
disk_io_write_to_file (fd_t fd, int offset, char *data, int data_size) {

    if (offset >= 0 && 
        lseek(fd,  offset, SEEK_SET) == -1) {
        assert(0);
    }

    assert (write (fd, data, data_size) != -1 );
}

void
disk_io_truncate_file_size (fd_t fd, int tr_size) {

    if (ftruncate(fd, tr_size) == -1) {
        assert(0);
    }
}

void
disk_io_expand_file_size (fd_t fd, int ex_size) {

    if (lseek(fd, ex_size - 1, SEEK_SET) == -1) {
        assert(0);
    }
}

void *
disk_io_file_mmap (fd_t fd, uint64_t start_offset, uint64_t end_offset) {

    void *pptr = mmap (NULL, end_offset - start_offset, 
                                        PROT_READ | PROT_WRITE, MAP_SHARED, 
                                        fd, start_offset);

    assert (pptr);
    return pptr;
}

void
disk_file_unmap (void *pptr, uint64_t len) {

    munmap (pptr, len);
}

int 
main (int argc, char **argv) {

    const char *file_name = "db-sample.txt";

    disk_io_create_disk_file (file_name, 1024 * 1024);

    fd_t fd = disk_io_open_file (file_name);

    disk_io_write_to_file (fd, 0, "Hello World", strlen ("Hello World"));

    disk_io_truncate_file_size (fd, 1024);

    disk_io_expand_file_size (fd, 1024);

    disk_io_delete_file (file_name);

    return 0;
}