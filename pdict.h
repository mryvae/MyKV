#ifndef __PDICT_H
#define __PDICT_H

#define PDICT_OK 0
#define PDICT_ERR 1
#include "pmem.h"
typedef struct pdictEntry {
    uint16_t keyoid;
    uint16_t keysize;
    uint16_t valueoid;
    uint16_t valsize;
    blockid next;
} pdictEntry;
typedef struct pdictmeta { // size of pdictmeta <= BLOCKSIZE
	size_t size;
} pdictmeta;
typedef struct pdict {
    pmem* pm;
    pdictmeta* phtmt;
    blockid* bucket;
    unsigned int size;
    unsigned int sizemask;
    unsigned int used;
    unsigned int maxKeyValSize;
    int exist;
} pdict;
typedef struct pdictIterator {
    pdict *ht;
    int index;
    blockid id,nextid;
} pdictIterator;
pdict *pdictCreate(pmem* pm);
void* pdictGetEntryKey(pdictEntry *entry);
void* pdictGetEntryVal(pdictEntry *entry);
int pdictAdd(pdict * ht, void * key, void *val);
int pdictReplace(pdict * ht, void *key, void *val);
int pdictDelete(pdict * ht, const void * key);
pdictEntry *pdictFind(pdict * ht, const void * key);
blockid pdictFindBlock(pdict * ht, const void * key);
pdictIterator *pdictGetIterator(pdict *ht);
blockid pdictNext(pdictIterator *iter);
void pdictReleaseIterator(pdictIterator *iter);
#endif