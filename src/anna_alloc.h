#ifndef ANNA_ALLOC_H
#define ANNA_ALLOC_H

#include "util.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_slab.h"

extern array_list_t anna_alloc;
/*
  The total amount of memory allocated at the last time the garbage
  collector was run
 */
extern int anna_alloc_tot;
/*
  The total amount of memory allocated after the last time the garbage
  collector was run
 */
extern int anna_alloc_count;
extern int anna_alloc_obj_count;
extern int anna_alloc_count_next_gc;
void anna_gc(anna_vmstack_t *stack);
void anna_gc_destroy(void);

#define GC_FREQ (1024*1024*4)

static inline void anna_alloc_check_gc(anna_vmstack_t *stack)
{
    if(unlikely(anna_alloc_count >= anna_alloc_count_next_gc))
    {
	anna_gc(stack);
    }
}

static inline __malloc anna_vmstack_t *anna_alloc_vmstack(size_t sz)
{
    anna_vmstack_t *res = anna_slab_alloc(sz);
//    anna_vmstack_t *res = malloc(sz);
    
    res->flags = ANNA_VMSTACK;
    al_push(&anna_alloc, res);
    anna_alloc_count+=sz;
    return res;
}

static inline __malloc void *anna_alloc_blob(size_t sz)
{
    sz += 2*sizeof(int);
    int *res = anna_slab_alloc(sz);
//    wprintf(L"Alloc %d (%d)=> %d (%d)\n", sz, sz+ sizeof(long long), res, (long)res & 7);
    res[0] = ANNA_BLOB;
    res[1] = sz;
    al_push(&anna_alloc, res);
    anna_alloc_count+=sz;
    return (void *)&res[2];
}

static inline __malloc anna_object_t *anna_alloc_object(size_t sz)
{
    anna_object_t *res = anna_slab_alloc(sz);
    res->flags = ANNA_OBJECT;
    al_push(&anna_alloc, res);
    anna_alloc_count+=sz;
    return res;
}

static inline __malloc anna_type_t *anna_alloc_type()
{
    anna_type_t *res = anna_slab_alloc(sizeof(anna_type_t));
//    wprintf(L"Alloc type @ %d\n", res);
    memset(res, 0, sizeof(anna_type_t));
    res->flags = ANNA_TYPE;
    al_push(&anna_alloc, res);
    return res;
}

static inline __malloc anna_function_t *anna_alloc_function()
{
    anna_function_t *res = anna_slab_alloc(sizeof(anna_function_t));
    memset(res, 0, sizeof(anna_function_t));
    res->flags = ANNA_FUNCTION;
    al_push(&anna_alloc, res);
    return res;
}

static inline __malloc void *anna_alloc_node(size_t sz)
{
    anna_node_t *res = calloc(1, sz);
    res->flags = ANNA_NODE;
    al_push(&anna_alloc, res);
    return res;
}

static inline __malloc  anna_stack_template_t *anna_alloc_stack_template()
{
    anna_stack_template_t *res = anna_slab_alloc(sizeof(anna_stack_template_t));
    memset(res, 0, sizeof(anna_stack_template_t));
    res->flags = ANNA_STACK_TEMPLATE;
    al_push(&anna_alloc, res);
    return res;
}

/**
   Mark an arbitrary memory allocation as used, and traverse and mark
   al allocations it references.
 */
void anna_alloc_mark(void *obj);
void anna_alloc_mark_entry(anna_entry_t *obj);
void anna_alloc_mark_object(anna_object_t *obj);
void anna_alloc_mark_type(anna_type_t *obj);
void anna_alloc_mark_stack_template(anna_stack_template_t *o);

void anna_alloc_gc_block(void);
void anna_alloc_gc_unblock(void);

#endif