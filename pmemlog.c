#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include "zmalloc.h"
#include "pmemlog.h"

static void _logFree(void *ptr) {
    zfree(ptr);
}
static void _logPanic(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "\nLOG LIBRARY PANIC: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
}
static void *_logAlloc(int size)
{
    void *c = zmalloc(size);
    if (c == NULL)
        _logPanic("Out of memory");
    return c;
}
static int logappend(log *lg,const void *buf, size_t count){
    if(lg->lgmt->cur + count > lg->lgmt->size) return -1;
	if (lg->is_pmem) {
		pmem_memcpy_persist(lg->pmemaddr+lg->lgmt->cur, buf, count);
        logupdatecur(lg,count);
	} else {
		memcpy(lg->pmemaddr+lg->lgmt->cur, buf, count);
		pmem_msync(lg->pmemaddr+lg->lgmt->cur, count);
        logupdatecur(lg,count);
	}
    return count;
}
log * logCreate(const char* path){
    log *lg = _logAlloc(sizeof(*lg));

    int logexist = 0;
    if (access(path, F_OK) == 0)
        logexist = 1;

	if ((lg->pmemaddr = pmem_map_file(path, LOG_POOL_SIZE,
			0,
			0666, &(lg->mapped_len), &(lg->is_pmem))) == NULL) {
		_logPanic("Create PMEMLOG failed!");
	}
    lg->lgmt=lg->pmemaddr;
    lg->buf = sdsempty();

    if(!logexist)
        logInitmeta(lg);

    return lg;
}
int logupdatecur(log *lg,size_t len){
    if(lg->lgmt->cur + len > lg->lgmt->size)return LOG_ERR;
    lg->lgmt->cur = lg->lgmt->cur + len;

	if (lg->is_pmem)
		pmem_persist(&(lg->lgmt->cur), sizeof(size_t));
	else
		pmem_msync(&(lg->lgmt->cur), sizeof(size_t));
    return LOG_OK;
}
int logInitmeta(log *lg){
    lg->lgmt->cur=sizeof(logmeta);
    lg->lgmt->size=lg->mapped_len;
	if (lg->is_pmem)
		pmem_persist(lg->lgmt, sizeof(logmeta));
	else
		pmem_msync(lg->lgmt, sizeof(logmeta));
    return LOG_OK;
}

int logappendPut(log *lg,void *key,void *val){
    lg->buf=sdscpy(lg->buf,"");

    lg->buf=sdscatlen(lg->buf,"P(",2);
    lg->buf=sdscatlen(lg->buf,((robj *)key)->ptr,sdslen(((robj *)key)->ptr));
    lg->buf=sdscatlen(lg->buf,",",1);
    lg->buf=sdscatlen(lg->buf,((robj *)val)->ptr,sdslen(((robj *)val)->ptr));
    lg->buf=sdscatlen(lg->buf,")\n",2);

    if (logappend(lg, lg->buf, sdslen(lg->buf)) < 0) {
        _logPanic("pmemlog append error!");
        return LOG_ERR;
	}
    return LOG_OK;
}
int logappendDel(log *lg,void *key){
    lg->buf=sdscpy(lg->buf,"");

    lg->buf=sdscatlen(lg->buf,"D(",2);
    lg->buf=sdscatlen(lg->buf,((robj *)key)->ptr,sdslen(((robj *)key)->ptr));
    lg->buf=sdscatlen(lg->buf,")\n",2);

    if (logappend(lg, lg->buf, sdslen(lg->buf)) < 0) {
        _logPanic("pmemlog append error!");
        return LOG_ERR;
	}
    return LOG_OK;
}
int logCheckPoint(log *lg){
    logInitmeta(lg);
    return LOG_OK;
}

int logWalk(log *lg,int (*process_chunk)(const void *buf, size_t len, void *arg)){
    process_chunk(lg->pmemaddr+sizeof(logmeta),lg->lgmt->cur-sizeof(logmeta), NULL);
    return LOG_OK;
}

int logUsed(log *lg){
    int used;
    used = lg->lgmt->cur * 100 / lg->lgmt->size;
    return used;
}