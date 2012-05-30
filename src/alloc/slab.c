#define SLAB_SZ 4096

slab_t **slab_list;
slab_t **slab_list_free;
slab_t **slab_list_tail;
static array_list_t *slab_alloc;

void anna_slab_init()
{
    /*
      Allocate at least one page for each allocation, to decrease odds
      of cache line bouncing.
      
      (list_free and list_tail are used by the gc thread while slab_list and
      slab_alloc are used by work thread)
     */
    slab_list = calloc(1, maxi(4096, SLAB_MAX * sizeof(slab_t *)));
    slab_list_free = calloc(1, maxi(4096, SLAB_MAX * sizeof(slab_t *)));
    slab_list_tail = calloc(1, maxi(4096, SLAB_MAX * sizeof(slab_t *)));
    slab_alloc = calloc(1, maxi(4096,SLAB_MAX*sizeof(array_list_t)));
}

static int anna_ptr_in_chunk(size_t sz, char *chunk, void *ptr)
{
    return (chunk < (char *)ptr) && ((chunk + sz*SLAB_SZ + sizeof(size_t)) > (char *)ptr);
}

static int cmpptr(const void *p1, const void *p2)
{
    ptrdiff_t diff = *(void **)p1-*(void **)p2;
    return diff == 0 ? 0 : (diff > 0 ? 1 : -1);
}

static size_t *anna_slab_counter(size_t sz, void *slab)
{
    int idx = al_bsearch(&slab_alloc[sz], slab, cmpptr)-1;
    void *ptr = al_get(&slab_alloc[sz], idx);
    return ptr;
}

/*
  Remove all slabs in slab_list[sz] that are part of the specified chunk
 */
static void anna_slab_remove_chunk_from_pool(
    size_t sz, char *chunk)
{
    slab_t *slab = slab_list[sz];
    /*
      First, make the slab variable point to the first slab that
      should not be removed.
     */
    while(slab && anna_ptr_in_chunk(sz, chunk, slab))
    {
	slab = slab->next;
    }
    /*
      Make slab_list point to the first slab that should not be removed
     */
    slab_list[sz] = slab;
    slab_t *prev = slab;

    /*
      Loop while slab is not null:
      
      1. Make prev the old value for slab.
      2. Make slab into the next slab that should not be removed
      3. Make prev->next point to slab

    */
    while(slab)
    {
	slab = slab->next;
	while(slab && anna_ptr_in_chunk(sz, chunk, slab))
	{
	    slab = slab->next;
	}
	prev->next = slab;
	prev = slab;
    }
}

static void anna_slab_reclaim_sz(size_t sz)
{
    int i;
    slab_t *slab = slab_list[sz];
    
    al_sort(&slab_alloc[sz], &cmpptr);
    
    while(slab)
    {
	size_t *chunk_sz = anna_slab_counter(sz, slab);	
	(*chunk_sz)++;
	slab = slab->next;
    }

    slab = slab_list[sz];
    
    for(i=0; i<al_get_count(&slab_alloc[sz]);)
    {
	size_t *chunk = al_get(&slab_alloc[sz], i);
	
	if(*chunk == SLAB_SZ)
	{
	    anna_slab_remove_chunk_from_pool(sz, (char *)chunk);
	    free(chunk);
	    al_set_fast(&slab_alloc[sz], i, al_get_fast(&slab_alloc[sz], al_get_count(&slab_alloc[sz])-1));
	    al_truncate(&slab_alloc[sz], al_get_count(&slab_alloc[sz])-1);
	}
	else
	{
	    *chunk = 0;
	    i++;
	}
    }
}

void anna_slab_free_return()
{
    int i;
    
    for(i=0; i<SLAB_MAX; i++)
    {
	if(slab_list_free[i])
	{
	    slab_list_tail[i]->next = slab_list[i];
	    slab_list[i] = slab_list_free[i];
	    slab_list_free[i] = slab_list_tail[i] = 0;
	}
    }
}

void anna_slab_reclaim()
{    
    if(SLAB_MAX == 0)
    {
	return;
    }
    
    static size_t sz = 0;
    anna_slab_reclaim_sz(sz);
    sz = (sz+4) % SLAB_MAX;
}

void anna_slab_alloc_batch(size_t sz)
{
//    anna_message(L"Allocate object batch of size %d. We have %d allocated items.\n", sz, anna_slab_alloc_count);

    char * mem = malloc(sz*SLAB_SZ + sizeof(double));
    al_push(&slab_alloc[sz], mem);
    *((size_t *)mem) = 0;
    mem += sizeof(double);

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
	    anna_message(L"%d free allocations of size %d use %d kB\n", count, i, count*i/1024);
	}
    }
    anna_message(L"In total, %d kB has been allocated but is unused\n", tot/1024);
}