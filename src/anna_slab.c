#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna_slab.h"

#define SLAB_SZ 128

slab_t **slab_list;

void anna_slab_init()
{
    slab_list = calloc(SLAB_MAX * sizeof(slab_t *), 1);
}

void anna_slab_alloc_batch(size_t sz)
{
//    wprintf(L"Allocate object batch of size %d. We have %d allocated items.\n", sz, anna_slab_alloc_count);

    char * mem = malloc(sz*SLAB_SZ);
    int i;
    slab_t *s;
    slab_t *prev=0;
    for(i=0; i<SLAB_SZ; i++)
    {
	s = (slab_t *)(&mem[i*sz]);
	s->next = prev;
	prev = s;
    }
    slab_list[sz] = s;    
}
