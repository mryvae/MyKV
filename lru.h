#ifndef __LRU_H
#define __LRU_H
#include "adlist.h"
#include "dict.h"
#define LRU_MIN_SIZE 1024
#define LRU_OK 0
#define LRU_ERR 1
typedef struct lru {
    unsigned int size;
    list *list;
    dict *dict;
} lru;
lru* lrunewlen(dictType *type,unsigned int size);
int lruget(lru* lr,void* key,int ahead);
int lruput(lru* lr,void* key);
#endif