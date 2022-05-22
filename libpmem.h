#ifndef _LIBPMEM_H
#define _LIBPMEM_H

#include<stddef.h>
#include<stdint.h>

void pmem_persist(void *buf, size_t len);
void pmem_msync(void *buf, size_t len);
void * pmem_map_file(const char *path, size_t len, int flags, int mode,
	    size_t *mapped_lenp, int *is_pmemp);
void *pmem_memcpy_persist(void *pmemdest, const void *src, size_t len);
#endif /* _LIBPMEM_H */