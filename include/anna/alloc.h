#ifndef ANNA_ALLOC_H
#define ANNA_ALLOC_H

#include "anna/util.h"
#include "anna/base.h"
#include "anna/node.h"
#include "anna/slab.h"

extern array_list_t anna_alloc[ANNA_ALLOC_TYPE_COUNT];
extern array_list_t anna_alloc_todo;

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
__hot void anna_gc(anna_context_t *stack);
__cold void anna_gc_destroy(void);

#define GC_FREQ (1024*1024*4)

static inline void anna_alloc_check_gc(anna_context_t *context)
{
    if(unlikely(anna_alloc_count >= anna_alloc_count_next_gc))
    {
	anna_gc(context);
    }
}

static inline __malloc anna_activation_frame_t *anna_alloc_activation_frame(size_t sz)
{
    anna_activation_frame_t *res = anna_slab_alloc(sz);
    res->flags = ANNA_ACTIVATION_FRAME;
    al_push(&anna_alloc[ANNA_ACTIVATION_FRAME], res);
    anna_alloc_count+=sz;
    return res;
}

static inline __malloc void *anna_alloc_blob(size_t sz)
{
    sz += 2*sizeof(int);
    int *res = anna_slab_alloc(sz);
    res[0] = ANNA_BLOB;
    res[1] = sz;
    al_push(&anna_alloc[ANNA_BLOB], res);
    anna_alloc_count+=sz;
    return (void *)res;
}

static inline void *anna_blob_payload(void *blob)
{
    int *res = (int *)blob;
    return (void *)&res[2];
}

static inline void *anna_blob_from_payload(void *blob)
{
    int *res = (int *)blob;
    return (void *)&res[-2];
}

static inline __malloc anna_object_t *anna_alloc_object(size_t sz)
{
    anna_object_t *res = anna_slab_alloc(sz);
    res->flags = ANNA_OBJECT;
    al_push(&anna_alloc[ANNA_OBJECT], res);
    anna_alloc_count+=sz;
    return res;
}

static inline __malloc anna_type_t *anna_alloc_type()
{
    anna_type_t *res = anna_slab_alloc(sizeof(anna_type_t));
    memset(res, 0, sizeof(anna_type_t));
    res->flags = ANNA_TYPE;
    al_push(&anna_alloc[ANNA_TYPE], res);
    anna_alloc_count+=sizeof(anna_type_t);
    return res;
}

static inline __malloc anna_function_t *anna_alloc_function()
{
    anna_function_t *res = anna_slab_alloc(sizeof(anna_function_t));
    memset(res, 0, sizeof(anna_function_t));
    res->flags = ANNA_FUNCTION;
    al_push(&anna_alloc[ANNA_FUNCTION], res);
    anna_alloc_count+=sizeof(anna_function_t);
    return res;
}

static inline __malloc anna_activation_frame_t *anna_alloc_callback_activation_frame(size_t frame_sz, size_t code_sz)
{
    size_t sz = frame_sz + sizeof(anna_function_t)+ code_sz;
    anna_activation_frame_t *res = anna_slab_alloc(sz);
    res->flags = ANNA_ACTIVATION_FRAME;
    al_push(&anna_alloc[ANNA_ACTIVATION_FRAME], res);
    anna_alloc_count+=sz;
    char *ptr = (char *)res;
    anna_function_t *fun = (anna_function_t *)(ptr + frame_sz);

    memset(fun, 0, sizeof(anna_function_t)+code_sz);
    fun->flags = ANNA_FUNCTION;
    fun->code = (char *)&fun[1];
    res->code = fun->code;
    fun->frame_size = sz;
    res->function = fun;
    return res;
}

static inline __malloc void *anna_alloc_node(size_t sz)
{
    anna_node_t *res = calloc(1, sz);
    res->flags = ANNA_NODE;
    al_push(&anna_alloc[ANNA_NODE], res);
    return res;
}

static inline __malloc  anna_stack_template_t *anna_alloc_stack_template()
{
    anna_stack_template_t *res = anna_slab_alloc(sizeof(anna_stack_template_t));
    memset(res, 0, sizeof(anna_stack_template_t));
    res->flags = ANNA_STACK_TEMPLATE;
    al_push(&anna_alloc[ANNA_STACK_TEMPLATE], res);
    anna_alloc_count+=sizeof(anna_stack_template_t);
    return res;
}

/**
   Mark an arbitrary memory allocation as used, and traverse and mark
   all allocations it references.
 */
__hot void anna_alloc_mark(void *obj);
__hot void anna_alloc_mark_entry(anna_entry_t *obj);

__hot static inline void anna_alloc_mark_object(anna_object_t *obj)
{
    if( obj->flags & ANNA_USED)
	return;
    
    obj->flags |= ANNA_USED;
#ifdef ANNA_CHECK_GC
    if(!obj->type->mark_object)
    {
	wprintf(L"No mark function in type %ls\n", obj->type->name);
	CRASH;
    }
#endif
    al_push(&anna_alloc_todo, obj);
}

__hot static inline void anna_alloc_mark_type(anna_type_t *type)
{
    if( type->flags & ANNA_USED)
	return;
    type->flags |= ANNA_USED;

    type->mark_type(type);
}

__hot void anna_alloc_mark_stack_template(anna_stack_template_t *o);
__hot void anna_alloc_mark_function(anna_function_t *o);
__hot void anna_alloc_mark_node(anna_node_t *o);


__hot void anna_alloc_gc_block(void);
__hot void anna_alloc_gc_unblock(void);

/**
   Tell Anna that the specified allocation (and all allocations it
   points to) should never ever be garbage collected, even if it
   appears to be unused. Use this function with great care, or memory
   leaks will result.
 */
__cold void anna_alloc_mark_permanent(void *alloc);

#endif
