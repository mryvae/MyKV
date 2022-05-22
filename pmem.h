#ifndef _PMEM_H
#define _PMEM_H
#define PMEM_OK 0
#define PMEM_ERR 1
#define PMEM_MIN_SIZE ((size_t)(1024 * 1024 * 1024)) //1G
#define BLOCK_SIZE ((size_t)(512))
#include<stddef.h>
#include<stdint.h>
#include "bitmap.h"
typedef long long blockid;
typedef struct pmem {
    void *pmemaddr;
    void *blockaddr;
    size_t mapped_len;
    int is_pmem;
    int exist;
    bitmap *bm;
} pmem;

pmem* pmemnewlen(char* path,size_t size);
blockid pmemAllocBlock(pmem* pm);
int pmemFreeBlock(pmem* pm,blockid id);
void* pmemGetBlockAddr(pmem* pm,blockid id);
uint16_t pmemblockCalculateCheckSum(pmem* pm,blockid id);
uint16_t pmemblockGetCheckSum(pmem* pm,blockid id);
int pmemblocklegal(pmem* pm,blockid id);
int pmemWritebackBlock(pmem* pm,blockid id);
#endif /* _PMEM_H */