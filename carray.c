#include "carray.h"
#include "zmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

static void _carrayFree(void *ptr) {
    zfree(ptr);
}
static void _carrayPanic(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "\nCARRAY LIBRARY PANIC: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
}
static void *_carrayAlloc(int size)
{
    void *c = zmalloc(size);
    if (c == NULL)
        _carrayPanic("Out of memory");
    return c;
}
carray* carraynewlen(unsigned int size){
    if(size<CARRAY_MIN_SIZE)
        size=CARRAY_MIN_SIZE;
    carray* ca = _carrayAlloc(sizeof(*ca));
    ca->array=zmalloc(size*sizeof(void*));
    ca->size=size;
    ca->used=0;
    ca->head=-1;
    ca->tail=0;
    return ca;
}

int carrayAdd(carray* ca,void * iterm){
    if(ca->used >= ca->size)return CARRAY_FULL;
    ca->array[ca->tail]=iterm;
    ca->tail=(ca->tail+1)%ca->size;
    ca->used++;
    return CARRAY_OK;
}
void * carrayDelete(carray* ca){
    if(ca->used==0)return NULL;
    void * item;
    ca->head=(ca->head+1)%ca->size;
    item=ca->array[ca->head];
    ca->used--;
    return item;
}
int carrayResize(carray* ca)
{
    int minimal = ca->size;
    if(minimal>=CARRAY_MAX_SIZE)
        return CARRAY_ERR;

    if (minimal < CARRAY_MIN_SIZE)
        minimal = CARRAY_MIN_SIZE;

    minimal=minimal*2;

    if(minimal > CARRAY_MAX_SIZE)
        minimal=CARRAY_MAX_SIZE;

    return carrayExpand(ca, minimal);
}
int carrayExpand(carray *ca, unsigned int size)
{
    carray n;
    unsigned int realsize = (size/1024)*1024;

    if (ca->used > size)
        return CARRAY_ERR;

    n.size = realsize;
    n.head = -1;
    n.tail = 0;
    n.used = 0;
    n.array = _carrayAlloc(realsize*sizeof(void*));

    void * item=carrayDelete(ca);
    while (item)
    {
        carrayAdd(&n,item);
        item=carrayDelete(ca);
    }
    _carrayFree(ca->array);

    *ca = n;
    return CARRAY_OK;
}