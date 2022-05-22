#include "pmem.h"
#include "zmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "libpmem.h"

static void _pmemPanic(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "\nPMEM LIBRARY PANIC: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
}
static void *_pmemAlloc(int size)
{
    void *c = zmalloc(size);
    if (c == NULL)
        _pmemPanic("Out of memory");
    return c;
}
static inline uint16_t check_sum(const uint16_t *buffer, int size)
{
    unsigned long cksum = 0;
    while(size>1){
        cksum += *buffer++;
        size -= sizeof(uint16_t);
    }
    if(size)
        cksum += *((unsigned char *)buffer);

    cksum = (cksum>>16) + (cksum&0xffff);
    cksum += (cksum>>16);

    return (uint16_t)(~cksum);
}
uint16_t pmemblockCalculateCheckSum(pmem* pm,blockid id){
    return check_sum(pmemGetBlockAddr(pm,id),BLOCK_SIZE-sizeof(uint16_t));
}
uint16_t pmemblockGetCheckSum(pmem* pm,blockid id){
    return ((uint16_t*)pmemGetBlockAddr(pm,id) - 1)[0];
}
int pmemblocklegal(pmem* pm,blockid id){
    return pmemblockCalculateCheckSum(pm,id) == pmemblockGetCheckSum(pm,id);
}
pmem* pmemnewlen(char* path,size_t size){
    if(size<PMEM_MIN_SIZE)
        size=PMEM_MIN_SIZE;
    pmem* pm = _pmemAlloc(sizeof(*pm));
    pm->exist = 0;
    if (access(path, F_OK) == 0)
        pm->exist = 1;
	if ((pm->pmemaddr = pmem_map_file(path, size,
			0,
			0666, &(pm->mapped_len), &(pm->is_pmem))) == NULL) {
		_pmemPanic("Create PMEM failed!");
	}
    pm->blockaddr = (char*)pm->pmemaddr + BLOCK_SIZE + (size/BLOCK_SIZE)*sizeof(blockid);
    pm->bm=bitmapnewlen((size - (BLOCK_SIZE + (size / BLOCK_SIZE)*sizeof(blockid)))/BLOCK_SIZE);
    return pm;
}
blockid pmemAllocBlock(pmem* pm){
    if(pm->bm->full)return -1;
    blockid avail = pm->bm->first_avail;
    bitmapSet(pm->bm,pm->bm->first_avail);
    return avail;
}
int pmemFreeBlock(pmem* pm,blockid id){
    if(id>=0 && bitmapClear(pm->bm,id)==BITMAP_OK)
        return PMEM_OK;
    return PMEM_ERR;
}
int pmemWritebackBlock(pmem* pm,blockid id){
    if(id < 0 || id >= pm->bm->size)
        return PMEM_ERR;
    ((uint16_t*)pmemGetBlockAddr(pm,id) - 1)[0] = pmemblockCalculateCheckSum(pm,id);
    if (pm->is_pmem)
		pmem_persist((void *)((char*)(pm->blockaddr)+id*BLOCK_SIZE), BLOCK_SIZE);
	else
		pmem_msync((void *)((char*)(pm->blockaddr)+id*BLOCK_SIZE), BLOCK_SIZE);

    return PMEM_OK;
}
void* pmemGetBlockAddr(pmem* pm,blockid id){
    if(id < 0 || id >= pm->bm->size)
        return NULL;
    return (char*)(pm->blockaddr) + id * BLOCK_SIZE + sizeof(uint16_t);
}
