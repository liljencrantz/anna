#include "anna_micro_gc.c"

typedef struct gc_

static array_list_t blocks;
static void *current_block;
static 
void amgc_init()
{
    al_init(allocs);
}

void *amgc_alloc(size_t s);
static void amgc_keep_item(amgc_context_t *context, void *ptr);
void amgc_run_gc();

