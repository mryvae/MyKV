#include "bitmap.h"
#include "zmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#define MASK 0x1F //16进制下的31

static void _bitmapPanic(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "\nBITMAP LIBRARY PANIC: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
}
static void *_bitmapAlloc(int size)
{
    void *c = zmalloc(size);
    if (c == NULL)
        _bitmapPanic("Out of memory");
    return c;
}
static inline int bitmapTest(bitmap* bm, size_t i) {
    return bm->map[i>>5] & (1 << (i & MASK));
}
bitmap* bitmapnewlen(size_t size){
    if(size<BITMAP_MIN_SIZE)
       size=BITMAP_MIN_SIZE;
    bitmap* bm = _bitmapAlloc(sizeof(*bm));
    bm->size=size;
    bm->map=_bitmapAlloc((1+size/sizeof(int))*sizeof(int));
    bm->first_avail=0;
    bm->full=0;
    return bm;
}
int bitmapSet(bitmap* bm, size_t i) {
    if(i >= bm->size)return BITMAP_ERR;
    bm->map[i>>5] |= (1 << (i & MASK));
    if(bm->first_avail!=i)return BITMAP_OK;
    bm->first_avail=bm->size;
    while (i < bm->size)
    {
        if(bitmapTest(bm,i)==0){
            bm->first_avail=i;
            break;
        }
        i++;
    }
    if(bm->first_avail==bm->size){
        bm->full = 1;
    }
    return BITMAP_OK;
}

int bitmapClear(bitmap* bm, size_t i) {
    if(i >= bm->size)return BITMAP_ERR;
    bm->map[i>>5] &= ~(1 << (i & MASK));
    if(i < bm->first_avail){
        bm->first_avail = i;
    }
    return BITMAP_OK;
}