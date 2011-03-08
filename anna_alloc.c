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

array_list_t anna_alloc = AL_STATIC;

static void anna_alloc_unmark(void *obj)
{
    *((int *)obj) &= (~ANNA_USED);
}

static void anna_alloc_mark_object(anna_object_t *obj)
{
    size_t i;
    anna_type_t *t = obj->type;
    for(i=0; i<t->member_count; i++)
    {
	
    }
    
}


static void anna_alloc_mark(void *obj)
{
    *((int *)obj) |= ANNA_USED;
    switch(*((int *)obj) & ANNA_ALLOC_MASK)
    {
	case ANNA_OBJECT:
	{
	    anna_alloc_mark_object((anna_object_t *)obj);
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
	for(obj = &stack->base[0]; obj < stack->top; obj++)
	{
	    anna_alloc_mark_object(*obj);
	}
	
    }
    


}


