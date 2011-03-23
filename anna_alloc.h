#ifndef ANNA_ALLOC_H
#define ANNA_ALLOC_H

#include "util.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_slab.h"

extern array_list_t anna_alloc;
extern int anna_alloc_count;
extern int anna_alloc_obj_count;
void anna_gc(void);
void anna_gc_destroy(void);


#define GC_FREQ 16000

static inline __malloc anna_vmstack_t *anna_alloc_vmstack(size_t sz)
{
    anna_alloc_count++;
    if(++anna_alloc_count%GC_FREQ == 0)
	anna_gc();
    anna_object_t *res = anna_slab_alloc(sz);
//    anna_vmstack_t *res = malloc(sz);
    res->flags = ANNA_VMSTACK;
    al_push(&anna_alloc, res);
    
    return res;
}

static inline __malloc anna_object_t *anna_alloc_object(size_t sz)
{
    if(++anna_alloc_count%GC_FREQ == 0)
	anna_gc();
//    anna_object_t *res = malloc(sz);
    anna_object_t *res = anna_slab_alloc(sz);
    res->flags = ANNA_OBJECT;
    al_push(&anna_alloc, res);
    return res;
}

static inline __malloc anna_type_t *anna_alloc_type()
{
    if(++anna_alloc_count%GC_FREQ == 0)
	anna_gc();
    anna_type_t *res = calloc(1, sizeof(anna_type_t));
    res->flags = ANNA_TYPE;
    al_push(&anna_alloc, res);
    return res;
}

static inline __malloc anna_function_t *anna_alloc_function()
{
    if(++anna_alloc_count%GC_FREQ == 0)
	anna_gc();
    anna_function_t *res = calloc(1, sizeof(anna_function_t));
    res->flags = ANNA_FUNCTION;
    al_push(&anna_alloc, res);
    return res;
}

static inline __malloc anna_node_t *anna_alloc_node(size_t sz)
{
    if(++anna_alloc_count%GC_FREQ == 0)
	anna_gc();
    anna_node_t *res = calloc(1, sz);
    res->flags = ANNA_NODE;
    al_push(&anna_alloc, res);
    return res;
}

static inline __malloc  anna_stack_template_t *anna_alloc_stack_template()
{
    if(++anna_alloc_count%GC_FREQ == 0)
	anna_gc();
    anna_stack_template_t *res = calloc(1, sizeof(anna_stack_template_t));
    res->flags = ANNA_STACK_TEMPLATE;
    al_push(&anna_alloc, res);
    return res;
}

void anna_alloc_mark_object(anna_object_t *obj);

void anna_alloc_gc_block(void);
void anna_alloc_gc_unblock(void);


#endif
