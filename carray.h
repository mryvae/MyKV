#ifndef __CARRAY_H
#define __CARRAY_H

#define CARRAY_OK 0
#define CARRAY_ERR 1
#define CARRAY_FULL 2
#define CARRAY_EMPTY 3
#define CARRAY_MIN_SIZE 1024
#define CARRAY_MAX_SIZE 1024*256
typedef struct carray {
    void ** array;
    unsigned int size;
    unsigned int used;
    unsigned int head;
    unsigned int tail;
} carray;
carray* carraynewlen(unsigned int size);
int carrayAdd(carray* ca,void * iterm);
void * carrayDelete(carray* ca);
int carrayResize(carray* ca);
int carrayExpand(carray *ca, unsigned int size);
#endif