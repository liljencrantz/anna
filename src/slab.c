#define SLAB_SZ 512

slab_t **slab_list;

void anna_slab_init()
{
    slab_list = calloc(SLAB_MAX,sizeof(slab_t *));
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

void anna_slab_print()
{
    size_t tot=0;
    int i;
    for(i=0;i<SLAB_MAX;i++)
    {
	if(slab_list[i])
	{
	    int count = 0;
	    slab_t *t = slab_list[i];
	    while(t)
	    {
		count++;
		t = t->next;
	    }
	    tot += count*i;
	    wprintf(L"%d free allocations of size %d use %d kB\n", count, i, count*i/1024);
	}
    }
    wprintf(L"In total, %d kB has been allocated but is unused\n", tot/1024);
}
