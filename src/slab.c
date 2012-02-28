#define SLAB_SZ 4096

slab_t **slab_list;
static array_list_t *slab_alloc;

void anna_slab_init()
{
    slab_list = calloc(SLAB_MAX,sizeof(slab_t *));
    slab_alloc = calloc(SLAB_MAX,sizeof(array_list_t));
}

static int anna_ptr_in_chunk(size_t sz, char *chunk, void *ptr)
{
    return (chunk < (char *)ptr) && ((chunk + sz*SLAB_SZ + sizeof(size_t)) > (char *)ptr);
}

static size_t *anna_slab_counter(size_t sz, void *slab)
{
    int i;
    for(i=0; i<al_get_count(&slab_alloc[sz]); i++)
    {
	char *chunk = al_get(&slab_alloc[sz], i);
	if(anna_ptr_in_chunk(sz, chunk, slab))
	{
	    return (size_t *)chunk;
	}
    }
    CRASH;
    return 0;
}

static void anna_slab_remove_chunk_from_pool(
    size_t sz, char *chunk)
{
    slab_t *slab = slab_list[sz];
    while(anna_ptr_in_chunk(sz, chunk, slab))
    {
	slab = slab->next;
    }
    slab_list[sz] = slab;
    slab_t *prev = slab;
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
//    anna_message(L"WOOT RECLAIM SZ %d, %d chunks\n", sz, al_get_count(&slab_alloc[sz]));
    slab_t *slab = slab_list[sz];

    while(slab)
    {
	size_t *chunk_sz = anna_slab_counter(sz, slab);
	(*chunk_sz)++;
	slab = slab->next;
    }

    for(i=0; i<al_get_count(&slab_alloc[sz]);)
    {
	size_t *chunk = al_get(&slab_alloc[sz], i);
//	anna_message(L"AAAA %d %d\n", *chunk, SLAB_SZ);
	
	if(*chunk == SLAB_SZ)
	{
	    //anna_message(L"YAY, FREEING AN ENTIRE CHUNK OF SIZE %d, WOOT!!!!\n", sz);

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

void anna_slab_reclaim()
{    
    int i;
    for(i=0; i<4; i++)
    {
	static size_t tjoho = 0;
	anna_slab_reclaim_sz(tjoho);
	tjoho = (tjoho+4) % SLAB_MAX;
    }
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
