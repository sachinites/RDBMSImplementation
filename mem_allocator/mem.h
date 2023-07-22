#ifndef __MEM_H__
#define __MEM_H__

#include <stdint.h>
#include "../disk_io/disk_io.h"

uint64_t
uapi_mem_alloc (fd_t fd, uint32_t req_size, void **ptr);

void 
uapi_mem_free (fd_t fd, uint64_t disk_addr) ;

void *
uapi_get_vm_addr (fd_t fd, uint64_t disk_addr);

#endif 