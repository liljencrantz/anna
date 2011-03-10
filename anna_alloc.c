#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "anna.h"
#include "anna_alloc.h"
#include "anna_vm.h"
#include "anna_function.h"

array_list_t anna_alloc = AL_STATIC;

static void anna_alloc_unmark(void *obj)
{
    *((int *)obj) &= (~ANNA_USED);
}

__pure static inline int anna_object_member_is_blob(anna_type_t *type, size_t off)
{
    return type->member_blob[off];
    
}

__pure static inline int anna_type_member_is_blob(anna_type_t *type, size_t off)
{
    return type->static_member_blob[off];
}

static void anna_alloc_mark_object(anna_object_t *obj);
static void anna_alloc_mark_type(anna_type_t *type);
static void anna_alloc_mark(void *obj);

static void anna_alloc_mark_function(anna_function_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;
}

static void anna_alloc_mark_stack_template(anna_stack_template_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;
}

static void anna_alloc_mark_node(anna_node_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;
}

static void anna_alloc_mark_type(anna_type_t *type)
{
    if( type->flags & ANNA_USED)
	return;
    type->flags |= ANNA_USED;
    size_t i;

    for(i=0; i<type->static_member_count; i++)
    {
	if(!anna_type_member_is_blob(type, i)){
	    anna_alloc_mark_object(type->static_member[i]);
	}	
    }    
}

static void anna_alloc_mark_object(anna_object_t *obj)
{
    if( obj->flags & ANNA_USED)
	return;
    obj->flags |= ANNA_USED;
    size_t i;
    anna_type_t *t = obj->type;
    for(i=0; i<t->member_count; i++)
    {
	if(!anna_object_member_is_blob(obj, i)){
	    anna_alloc_mark_object(obj->member[i]);
	}
    }
    anna_alloc_mark_type(obj->type);
    anna_function_t *f = anna_function_unwrap(obj);
    if(f){
	anna_alloc_mark_function(f);
    }
    anna_type_t *wt = anna_type_unwrap(obj);
    if(wt){
	anna_alloc_mark_type(wt);
    }
}

static void anna_alloc_mark_vmstack(anna_vmstack_t *stack)
{
    anna_object_t **obj;
    for(obj = &stack->base[0]; obj < stack->top; obj++)
    {
	anna_alloc_mark_object(*obj);
    }
}

static void anna_alloc_mark(void *obj)
{
    switch(*((int *)obj) & ANNA_ALLOC_MASK)
    {
	case ANNA_OBJECT:
	{
	    anna_alloc_mark_object((anna_object_t *)obj);
	    break;
	}
	case ANNA_TYPE:
	{
	    anna_alloc_mark_type((anna_type_t *)obj);
	    break;
	}
	case ANNA_VMSTACK:
	{
	    anna_alloc_mark_vmstack((anna_vmstack_t *)obj);
	    break;
	}
	case ANNA_FUNCTION:
	{
	    anna_alloc_mark_function((anna_function_t *)obj);
	    break;
	}
	case ANNA_NODE:
	{
	    anna_alloc_mark_node((anna_node_t *)obj);
	    break;
	}
	case ANNA_STACK_TEMPLATE:
	{
	    anna_alloc_mark_stack_template((anna_stack_template_t *)obj);
	    break;
	}
    }
}

void anna_gc()
{
    size_t i;
    anna_object_t **obj;
    
    for(i=0; i<al_get_count(&anna_alloc); i++)
    {
	anna_alloc_unmark(al_get(&anna_alloc, i));
    }    
    
    for(i=0; i<anna_vm_stack_frame_count(); i++)
    {
	anna_vmstack_t *stack = anna_vm_stack_get(i);
	anna_alloc_mark_vmstack(stack);	
    }

    for(i=0; i<al_get_count(&anna_alloc); i++)
    {
	void *el = al_get(&anna_alloc, i);
	if(!(*((int *)el) & ANNA_USED))
	{
	    free(el);
	}
    }    
        
}


