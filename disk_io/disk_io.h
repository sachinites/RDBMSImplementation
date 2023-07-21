#ifndef __DISK_IO__
#define __DISK_IO__

#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>

// refer : https://www.codequoi.com/en/handling-a-file-by-its-descriptor-in-c/

typedef int fd_t;
typedef uint64_t fsize;

/* API to create a disk file*/
fd_t
disk_io_create_disk_file (const char* path, int flags);

bool 
disk_io_set_file_size (fd_t fd, fsize new_size);

bool 
disk_io_close_file (fd_t fd);

bool 
disk_io_delete_file (fd_t fd);

bool 
disk_io_write_to_file (fd_t fd, int offset, char *data, int data_size);

bool 
disk_io_truncate_file_size (fd_t fd, int tr_size);

bool
disk_io_expand_file_size (fd_t fd, int ex_size);


#endif 