#ifndef ANNA_SLAB_H
#define ANNA_SLAB_H

#include "anna.h"

#define SLAB_MAX 128

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
    if(unlikely(sz >= SLAB_MAX))
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
    if(unlikely(sz >= SLAB_MAX))
    {
	free(ptr);
	return;
    }
    slab_t *s = (slab_t *)ptr;
    s->next = slab_list[sz];
    slab_list[sz] = s;
}

#endif
