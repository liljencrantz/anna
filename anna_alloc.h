#ifndef ANNA_ALLOC_H
#define ANNA_ALLOC_H

#include "anna.h"

array_list_t anna_alloc;

static inline anna_vmstack_t *anna_alloc_vmstack(size_t sz)
{
    anna_vmstack_t *res = calloc(1, sz);
    res->flags = ANNA_VMSTACK;
    al_push(&anna_alloc, res);
    return res;
}

static inline anna_object_t *anna_alloc_object(size_t sz)
{
    anna_object_t *res = calloc(1, sz);
    res->flags = ANNA_OBJECT;
    al_push(&anna_alloc, res);
    return res;
}

static inline anna_type_t *anna_alloc_type()
{
    anna_type_t *res = calloc(1, sizeof(anna_type_t));
    res->flags = ANNA_TYPE;
    al_push(&anna_alloc, res);
    return res;
}

static inline anna_function_t *anna_alloc_function()
{
    anna_function_t *res = calloc(1, sizeof(anna_function_t));
    res->flags = ANNA_FUNCTION;
    al_push(&anna_alloc, res);
    return res;
}

static inline anna_node_t *anna_alloc_node(size_t sz)
{
    anna_node_t *res = calloc(1, sz);
    res->flags = ANNA_NODE;
    al_push(&anna_alloc, res);
    return res;
}

void anna_gc();

#endif
