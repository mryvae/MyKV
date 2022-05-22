#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "pdict.h"
#include "sds.h"

static void _pdictPanic(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "\nPDICT LIBRARY PANIC: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
}

static void *_pdictAlloc(int size)
{
    void *p = zmalloc(size);
    if (p == NULL)
        _pdictPanic("Out of memory");
    return p;
}

static void _pdictFree(void *ptr) {
    zfree(ptr);
}

unsigned int pdictIntHashFunction(unsigned int key)
{
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}

unsigned int pdictGenHashFunction(const unsigned char *buf, int len) {
    unsigned int hash = 5381;

    while (len--)
        hash = ((hash << 5) + hash) + (*buf++); /* hash * 33 + c */
    return hash;
}
static unsigned int _pdictLastPower(unsigned int size)
{
    unsigned int i = 16;

    if (size >= 2147483648U)
        return 2147483648U;
    while(1) {
        if (i >= size)
            break;
        i *= 2;
    }
    return i/2;
}
static int pdictInitmeta(pdict *ht){
    if(ht->exist) return PDICT_OK;
    ht->phtmt->size = ht->size;
	if (ht->pm->is_pmem)
		pmem_persist(ht->phtmt, sizeof(pdictmeta));
	else
		pmem_msync(ht->phtmt, sizeof(pdictmeta));

    for(int i = 0;i < ht->size;i++)
        ht->bucket[i] = -1;

    if (ht->pm->is_pmem)
        pmem_persist((char*)(ht->bucket), sizeof(blockid)*ht->size);
    else
        pmem_msync((char*)(ht->bucket), sizeof(blockid)*ht->size);
    return PDICT_OK;
}
static void pdictInitBitmap(pdict *ht){
    if(!ht->exist)return;
    pdictIterator *iter = pdictGetIterator(ht);
    blockid id = pdictNext(iter);
    while (id >= 0)
    {
        if(pmemblocklegal(ht->pm,id)){
            bitmapSet(ht->pm->bm, id);
        }else{
            blockid pre = -1,cur = ht->bucket[iter->index];
            while (cur!=-1 && cur != id)
            {
                pre = cur;
                cur = ((pdictEntry *)pmemGetBlockAddr(ht->pm,cur))->next;
            }
            if(cur == id){
                if(pre == -1){
                    ht->bucket[iter->index] = iter->nextid;
                    if (ht->pm->is_pmem)
                        pmem_persist(ht->bucket + iter->index, sizeof(blockid));
                    else
                        pmem_msync(ht->bucket + iter->index, sizeof(blockid));
                }else{
                    ((pdictEntry *)pmemGetBlockAddr(ht->pm,pre))->next = iter->nextid;
                    pmemWritebackBlock(ht->pm,pre);
                }
            }
        }
        id = pdictNext(iter);
    }
    pdictReleaseIterator(iter);
}
static unsigned int _pdictStringCopyHTHashFunction(const void *key)
{
    return pdictGenHashFunction(((robj *)key)->ptr, strlen(((robj *)key)->ptr));
}
static int _pdictStringCopyHTKeyCompare(const void *key1,const void *key2)
{
    return strcmp(((robj *)key1)->ptr,key2) == 0;
}
void* pdictGetEntryKey(pdictEntry *entry){
    uint16_t keyoid = entry->keyoid;
    return (char *)entry + keyoid;
}
static inline int pdictSetEntryKey(pdict* ht, pdictEntry *entry,const void *key){
    entry->keyoid = sizeof(pdictEntry);
    entry->keysize = sdslen(((robj *)key)->ptr);
    if(entry->keysize +1 > ht->maxKeyValSize){
        _pdictPanic("key too lang!");
        return PDICT_ERR;
    }
    memcpy((char*)entry + entry->keyoid,((robj *)key)->ptr, entry->keysize);
    ((char*)entry)[entry->keyoid + entry->keysize] = '\0';
    return PDICT_OK;
}
void* pdictGetEntryVal(pdictEntry *entry){
    uint16_t valoid = entry->valueoid;
    return (char *)entry + valoid;
}
static inline int pdictSetEntryVal(pdict* ht, pdictEntry *entry,const void *val){
    entry->valueoid = sizeof(pdictEntry) + entry->keysize + 1;
    entry->valsize = sdslen(((robj *)val)->ptr);
    if(entry->keysize +1 + entry->valsize +1 > ht->maxKeyValSize){
        _pdictPanic("value too lang!");
        return PDICT_ERR;
    }
    memcpy((char*)entry + entry->valueoid,((robj *)val)->ptr, entry->valsize);
    ((char*)entry)[entry->valueoid + entry->valsize] = '\0';
    return PDICT_OK;
}
static inline blockid pdictGetEntryNext(pdictEntry *entry){
    return entry->next;
}
static inline void pdictSetEntryNext(pdictEntry *entry,blockid next){
    entry->next = next;
}
static int _pdictKeyIndex(pdict* ht, const void *key)
{
    printf("enter pdictFindBlock\n");
    unsigned int h;
    pdictEntry *he;

    /* Compute the key hash value */
    h = _pdictStringCopyHTHashFunction(key) & ht->sizemask;
    printf("bucket:%u\n",h);
    /* Search if this slot does not already contain the given key */
    he = pdictFind(ht,key);
    if(he) return -1;
    return h;
}
pdict *pdictCreate(pmem* pm)
{
    pdict *ht = _pdictAlloc(sizeof(pdict));

    ht->pm = pm;
    ht->size = _pdictLastPower(pm->bm->size);
    ht->phtmt = pm->pmemaddr;
    ht->bucket = (char*)(pm->pmemaddr) + sizeof(pdictmeta);
    ht->sizemask = ht->size -1;
    ht->used = 0;
    ht->maxKeyValSize = BLOCK_SIZE - sizeof(pdictEntry);
    ht->exist = pm->exist;
    pdictInitmeta(ht);
    pdictInitBitmap(ht);
    return ht;
}

pdictEntry *pdictFind(pdict * ht, const void * key){
    pdictEntry *he;
    blockid id;
    if((id = pdictFindBlock(ht,key)) == -1)
        return NULL;
    return pmemGetBlockAddr(ht->pm,id);
}
blockid pdictFindBlock(pdict * ht, const void * key){
    printf("enter pdictFindBlock\n");
    pdictEntry *he;
    blockid id;
    unsigned int h;

    h = _pdictStringCopyHTHashFunction(key) & ht->sizemask;
    printf("bucket:%u\n",h);
    id = ht->bucket[h];
    if(id == -1) return -1;
    he = pmemGetBlockAddr(ht->pm,id);
    while(he) {
        if (_pdictStringCopyHTKeyCompare(key, pdictGetEntryKey(he)))
            return id;
        id = pdictGetEntryNext(he);
        if(id == -1) return -1;
        he = pmemGetBlockAddr(ht->pm,id);
    }
    return -1;
}
int pdictAdd(pdict * ht, void *key, void *val){
    printf("enter pdictAdd\n");
    int index;
    pdictEntry *entry;
    blockid id;
    if((index = _pdictKeyIndex(ht,key)) == -1){
        _pdictPanic("already exist the key!");
        return PDICT_ERR;
    }
    printf("_pdictKeyIndex:%d\n",index);
    if((id = pmemAllocBlock(ht->pm)) == -1){
        _pdictPanic("pmem memory full!");
        return PDICT_ERR;
    }
    entry = pmemGetBlockAddr(ht->pm,id);
    if(pdictSetEntryKey(ht,entry,key) == PDICT_ERR){
        pmemFreeBlock(ht->pm,id);
        return PDICT_ERR;
    }
    if(pdictSetEntryVal(ht,entry,val) == PDICT_ERR){
        pmemFreeBlock(ht->pm,id);
        return PDICT_ERR;
    }
    pdictSetEntryNext(entry, -1);
    pmemWritebackBlock(ht->pm,id);
    if(ht->bucket[index] == -1){
        ht->bucket[index] = id;
        if (ht->pm->is_pmem)
            pmem_persist(ht->bucket + index, sizeof(blockid));
        else
            pmem_msync(ht->bucket + index, sizeof(blockid));
        ht->used++;
        return PDICT_OK;
    }
    blockid nextid,heid = ht->bucket[index];
    pdictEntry *he = pmemGetBlockAddr(ht->pm,heid);
    while(he) {
        nextid = pdictGetEntryNext(he);
        if(nextid == -1) break;
        heid = nextid;
        he = pmemGetBlockAddr(ht->pm,heid);
    }
    if(he){
        he->next = id;
        pmemWritebackBlock(ht->pm,heid);
        ht->used++;
        return PDICT_OK;
    }else{
        pmemFreeBlock(ht->pm,id);
        return PDICT_ERR;
    }
}

int pdictReplace(pdict * ht, void *key, void *val)
{
    blockid id;
    pdictEntry *entry;

    /* Try to add the element. If the key
     * does not exists dictAdd will suceed. */
    if (pdictAdd(ht, key, val) == PDICT_OK)
        return PDICT_OK;
    /* It already exists, get the entry */
    id = pdictFindBlock(ht,key);
    entry = pmemGetBlockAddr(ht->pm,id);
    /* Free the old value and set the new one */
    if(pdictSetEntryVal(ht,entry,val) == PDICT_ERR){
        return PDICT_ERR;
    }
    pmemWritebackBlock(ht->pm,id);
    return PDICT_OK;
}

int pdictDelete(pdict *ht, const void *key)
{
    unsigned int h;
    blockid id,previd;
    pdictEntry *he, *prevHe;

    if (ht->size == 0)
        return PDICT_ERR;
    h = _pdictStringCopyHTHashFunction(key) & ht->sizemask;
    id = ht->bucket[h];
    he = pmemGetBlockAddr(ht->pm,id);
    prevHe = NULL;
    while(he) {
        if (_pdictStringCopyHTKeyCompare(key, pdictGetEntryKey(he))) {
            /* Unlink the element from the list */
            if (prevHe){
                pdictSetEntryNext(prevHe, he->next);
                pmemWritebackBlock(ht->pm,previd);
            }
            else{
                ht->bucket[h] = he->next;
                if (ht->pm->is_pmem)
                    pmem_persist(ht->bucket + h, sizeof(blockid));
                else
                    pmem_msync(ht->bucket + h, sizeof(blockid));
            }
            pmemFreeBlock(ht->pm,id);
            ht->used--;
            return PDICT_OK;
        }
        previd = id;
        prevHe = he;
        id = he->next;
        he = pmemGetBlockAddr(ht->pm,id);
    }
    return PDICT_ERR; /* not found */
}
pdictIterator *pdictGetIterator(pdict *ht){
    pdictIterator *iter = _pdictAlloc(sizeof(*iter));

    iter->ht = ht;
    iter->index = -1;
    iter->id = -1;
    iter->nextid = -1;
    return iter;
}
blockid pdictNext(pdictIterator *iter){
    while (1) {
        if (iter->id == -1) {
            iter->index++;
            if (iter->index >= (signed)iter->ht->size)
                break;
            iter->id = iter->ht->bucket[iter->index];
        } else {
            iter->id = iter->nextid;
        }
        if (iter->id >= 0) {
            /* We need to save the 'next' here, the iterator user
             * may delete the entry we are returning. */
            iter->nextid = ((pdictEntry *)pmemGetBlockAddr(iter->ht->pm,iter->id))->next;
            return iter->id;
        }
    }
    return -1;
}
void pdictReleaseIterator(pdictIterator *iter)
{
    _pdictFree(iter);
}