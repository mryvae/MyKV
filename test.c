#include "pmem.h"
#include "pdict.h"
#include "bitmap.h"
#include "sds.h"
#include "pmemlog.h"
#include<stdio.h>
static robj *createObject(int type, void *ptr) {
    robj *o;
    o = zmalloc(sizeof(*o));
    o->type = type;
    o->ptr = ptr;
    o->refcount = 1;
    o->dirty=0;
    return o;
}

static robj *createStringObject(char *ptr, size_t len) {
    return createObject(1,sdsnewlen(ptr,len));
}
static void testbitmap(){
    bitmap* bm=bitmapnewlen(5);
    bitmapSet(bm,0);
    printf("%ld\n",bm->first_avail);
    bitmapSet(bm,2);
    bitmapSet(bm,3);
    printf("%ld\n",bm->first_avail);
    bitmapSet(bm,1);
    printf("%ld\n",bm->first_avail);
    bitmapClear(bm,2);
    printf("%ld\n",bm->first_avail);
    bitmapSet(bm,2);
    bitmapSet(bm,4);
    if(bm->full){
        printf("full!\n");
    }
}
static int printit(const void *buf, size_t len, void *arg)
{
	fwrite(buf, len, 1, stdout);
	return 0;
}
static void testlog(){
    log * lg=logCreate("/mnt/pmem-mry/log");
    robj *key1=createStringObject("mry",3);
    robj *val1=createStringObject("abc",3);
    robj *key2=createStringObject("mry1",4);
    robj *val2=createStringObject("abc1",4);
    logappendPut(lg,key1,val1);
    logappendPut(lg,key2,val2);
    logappendPut(lg,key1,val1);
    logappendPut(lg,key2,val2);
    logWalk(lg,printit);
    logCheckPoint(lg);
    logappendPut(lg,key1,val1);
    logappendPut(lg,key1,val1);
    logWalk(lg,printit);
}
static void testpmem(){
    pmem* pm=pmemnewlen("/mnt/pmem-mry/kv",PMEM_MIN_SIZE);
    printf("pmemaddr:%p,blockaddr%p\n",pm->pmemaddr,pm->blockaddr);
    blockid id = pmemAllocBlock(pm);
    printf("first blockid:%ld\n",id);
    pmemFreeBlock(pm,id);
    id = pmemAllocBlock(pm);
    printf("second blockid:%ld\n",id);
    char *blockaddr = pmemGetBlockAddr(pm,id);
    pmem_memcpy_persist(blockaddr, "mry1235", 7);
    if(pmemblocklegal(pm,id)){
        printf("legal\n");
    }else{
        printf("not legal\n");
    }
    pmemWritebackBlock(pm,id);
    if(pmemblocklegal(pm,id)){
        printf("legal\n");
    }else{
        printf("not legal\n");
    }
}
static void traversepdict(pdict *ht){
    pdictIterator *iter = pdictGetIterator(ht);
    pdictEntry * entry = pmemGetBlockAddr(ht->pm,pdictNext(iter));
    while (entry)
    {
        printf("------------%s,%s\n",pdictGetEntryKey(entry),pdictGetEntryVal(entry));
        entry = pmemGetBlockAddr(ht->pm,pdictNext(iter));
    }
    pdictReleaseIterator(iter);
}
static void testpdict(){
    pmem* pm=pmemnewlen("/mnt/pmem-mry/kv",PMEM_MIN_SIZE);
    pdict *ht = pdictCreate(pm);
    robj *key1=createStringObject("mry",3);
    robj *val1=createStringObject("abc",3);
    robj *key2=createStringObject("mry1",4);
    robj *val2=createStringObject("abc1",4);
    robj *key3=createStringObject("mry2",4);
    robj *val3=createStringObject("abc2",4);
    robj *key4=createStringObject("11121123",8);
    robj *val4=createStringObject("11121333",8);
    pdictAdd(ht,key1,val1);
    pdictAdd(ht,key2,val2);
    pdictAdd(ht,key3,val3);
    pdictAdd(ht,key4,val4);
    pdictAdd(ht,key1,val1);
    pdictAdd(ht,key2,val2);
    pdictAdd(ht,key3,val3);
    pdictAdd(ht,key4,val4);
    traversepdict(ht);
}
static void testpdict1(){
    pmem* pm=pmemnewlen("/mnt/pmem-mry/kv",PMEM_MIN_SIZE);
    pdict *ht = pdictCreate(pm);
    robj *key1=createStringObject("mry",3);
    robj *val1=createStringObject("abc",3);
    robj *key2=createStringObject("mry1",4);
    robj *val2=createStringObject("abc1",4);
    robj *key3=createStringObject("mry2",4);
    robj *val3=createStringObject("abc2",4);
    robj *key4=createStringObject("11121123",8);
    robj *val4=createStringObject("11121333",8);
    pdictAdd(ht,key3,val3);
    pdictReplace(ht,key3,key4);
    pdictDelete(ht,key4);
    traversepdict(ht);
}
int main(){
    //testbitmap();
    //testlog();
    //testpmem();
    //testpdict();
    testpdict1();
    return 0;
}