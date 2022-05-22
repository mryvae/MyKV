#include "lru.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "zmalloc.h"

static void _lruFree(void *ptr) {
    zfree(ptr);
}
static void _lruPanic(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "\nLRU LIBRARY PANIC: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
}
static void *_lruAlloc(int size)
{
    void *c = zmalloc(size);
    if (c == NULL)
        _lruPanic("Out of memory");
    return c;
}

lru* lrunewlen(dictType *type,unsigned int size){
    if(size<LRU_MIN_SIZE)
        size=LRU_MIN_SIZE;
    lru* lr = _lruAlloc(sizeof(*lr));
    lr->size=size;
    lr->list=listCreate();
    lr->dict=dictCreate(type,NULL);
    return lr;
}

int lruget(lru* lr,void* key,int ahead){
    dictEntry *entry;
    entry=dictFind(lr->dict,key);
    if(!entry)return LRU_ERR;
    //printf("Find in lru\n");
    if(ahead){
        if(entry->val){
            listAheadNode(lr->list,entry->val);
        }else{
            printf("entry->val is NULL\n");
        }
    }
    return LRU_OK;
}
int lruput(lru* lr,void* key){
    if(lruget(lr,key,1)==LRU_OK)return LRU_OK;
    if(lr->list->len < lr->size){
        listNode *node=listAddNodeMidpoint(lr->list, key);
        if(!node)return LRU_ERR;
        if(dictAdd(lr->dict,key,node)==DICT_ERR)return LRU_ERR;
    }else{
        if(dictDelete(lr->dict,lr->list->tail->value)==DICT_ERR)
            return LRU_ERR;
        listDelNode(lr->list, lr->list->tail);
        listNode *node=listAddNodeMidpoint(lr->list, key);
        if(!node)return LRU_ERR;
        if(dictAdd(lr->dict,key,node)==DICT_ERR)return LRU_ERR;
    }
    return LRU_OK;
}