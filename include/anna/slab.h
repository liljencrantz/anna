#ifndef ANNA_SLAB_H
#define ANNA_SLAB_H

#include <assert.h>

#include "anna/base.h"

/*
  The maximum size of memory allocations where we will use the Anna
  «slab memory allocator». For larger sizes, simply fall back to
  malloc/free.
*/
#define SLAB_MAX 64

struct slab
{
    struct slab *next;
}
    ;

typedef struct slab slab_t;

extern slab_t **slab_list;

void anna_slab_alloc_batch(size_t sz);
void anna_slab_init(void);

static inline __malloc void *anna_slab_alloc(size_t sz)
{
#if 0
    size_t *foo = malloc(sz + 2*sizeof(size_t));
    *foo = sz;
    foo+=2;

    return foo;
#endif

    if(sz >= SLAB_MAX)
    {
	return malloc(sz);
    }
    
    if(unlikely(!slab_list[sz]))
    {
	anna_slab_alloc_batch(sz);
    }
    slab_t *res = slab_list[sz];
    slab_list[sz] = res->next;
    return (void *)res;

}

static inline void anna_slab_free(void *ptr, size_t sz)
{
#if 0
    size_t *foo = ptr;
    foo-=2;
    if(*foo != sz)
    {
	wprintf(L"Oops %d vs %d\n", sz, *foo);
	CRASH;
    }
    
    free(foo);
    return;
#endif

    if(sz >= SLAB_MAX)
    {
	free(ptr);
	return;
    }
    slab_t *s = (slab_t *)ptr;
    s->next = slab_list[sz];
    slab_list[sz] = s;
}

/**
   Print some statistics about slab allocations
*/
void anna_slab_print(void);

void anna_slab_reclaim(void);


#endif
