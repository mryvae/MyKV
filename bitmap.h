#ifndef _BITMAP_H
#define _BITMAP_H
#define BITMAP_OK 0
#define BITMAP_ERR 1
#define BITMAP_MIN_SIZE ((size_t)(1024 * 1024 * 1024 / 512)) // 1G/512B
#include<stddef.h>
typedef struct bitmap {
    size_t size;
    int *map;
    size_t first_avail;
    int full;
} bitmap;
bitmap* bitmapnewlen(size_t size);
int bitmapSet(bitmap* bm, size_t i);
int bitmapClear(bitmap* bm, size_t i);
#endif /* _BITMAP_H */