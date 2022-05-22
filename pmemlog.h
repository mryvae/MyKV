#ifndef __LOG_H
#define __LOG_H

#define LOG_OK 0
#define LOG_ERR 1
#define LOG_POOL_SIZE 1024*1024*128
#include "libpmem.h"
#include "sds.h"
typedef struct logmeta {
    size_t cur;
    size_t size;
} logmeta;
typedef struct log {
    logmeta* lgmt;
    void *pmemaddr;
    size_t mapped_len;
    int is_pmem;
    sds buf;
} log;
log * logCreate(const char* path);
int logInitmeta(log *lg);
int logappendPut(log *lg,void *key,void *val);
int logappendDel(log *lg,void *key);
int logCheckPoint(log *lg);
int logWalk(log *lg,int (*process_chunk)(const void *buf, size_t len, void *arg));
int logUsed(log *lg);
#endif