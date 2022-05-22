#define _GNU_SOURCE
#include "libpmem.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

static void *map_file(const char *path, size_t size)
{
    int fd = open(path, O_RDWR | O_DIRECT | O_SYNC | O_CREAT, S_IREAD | S_IWRITE);

    void *ret = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    close(fd);

    return ret;
}
void pmem_persist(void *buf, size_t len)
{
    void *ptr = (void *)((unsigned long)buf & ~(64 - 1));
    for (; ptr < buf + len; ptr += 64)
        __asm__ volatile("clwb %0" : "+m" (ptr));
    __asm__ volatile("sfence":::"memory");
}
void pmem_msync(void *buf, size_t len){
    void *ptr = (void *)((unsigned long)buf & ~(64 - 1));
    for (; ptr < buf + len; ptr += 64)
        __asm__ volatile("clwb %0" : "+m" (ptr));
    __asm__ volatile("sfence":::"memory");
}
void * pmem_map_file(const char *path, size_t len, int flags, int mode,
	    size_t *mapped_lenp, int *is_pmemp)
{
    void *ret = map_file(path,len);
    if (mapped_lenp != NULL)
        *mapped_lenp = len;

	if (is_pmemp != NULL)
		*is_pmemp = 1;
    return ret;
}
void *pmem_memcpy_persist(void *pmemdest, const void *src, size_t len){
    memcpy(pmemdest,src,len);
    pmem_persist(pmemdest,len);
    return pmemdest;
}