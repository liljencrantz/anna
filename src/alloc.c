#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/common.h"
#include "anna/base.h"
#include "anna/alloc.h"
#include "anna/vm.h"
#include "anna/function.h"
#include "anna/function_type.h"
#include "anna/member.h"
#include "anna/lib/lang/int.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/hash.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/parser.h"
#include "anna/mid.h"
#include "anna/lib/reflection.h"
#include "anna/type.h"
#include "anna/use.h"
#include "anna/slab.h"

#include "src/slab.c"

array_list_t anna_alloc[ANNA_ALLOC_TYPE_COUNT] = {
    AL_STATIC,
    AL_STATIC,
    AL_STATIC,
    AL_STATIC,
    AL_STATIC,
    AL_STATIC,
    AL_STATIC
}
    ;

int anna_alloc_tot=0;
int anna_alloc_count=0;
int anna_alloc_count_next_gc=1024*1024;
int anna_alloc_gc_block_counter;
int anna_alloc_run_finalizers=1;
array_list_t anna_alloc_todo = AL_STATIC;

static void anna_alloc_unmark(void *obj)
{
    *((int *)obj) &= (~ANNA_USED);
}

void anna_alloc_mark_function(anna_function_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;

    if(!o->wrapper)
	return;
        
    anna_function_type_t *ft = anna_function_type_unwrap(o->wrapper->type);
    int i;
    for(i=0; i<o->input_count; i++)
    {
	if(o->input_default[i])
	{
	    anna_alloc_mark_node(o->input_default[i]);
	    anna_alloc_mark_node(ft->input_default[i]);
	}
    }
    
    anna_alloc_mark_node((anna_node_t *)o->attribute);
//    wprintf(L"WEE %ls\n", o->return_type->name);
    
    anna_alloc_mark_type(o->return_type);
    anna_alloc_mark_object(o->wrapper);
    if(o->this)
	anna_alloc_mark_object(o->this);
    if(o->stack_template)
	anna_alloc_mark_stack_template(o->stack_template);

    for(i=0; i<o->input_count; i++)
    {
#ifdef ANNA_CHECK_GC
	if(!o->input_type[i])
	{
	    wprintf(L"No type specified for input argument %d of function %ls\n", 
		    i+1, o->name);
	    CRASH;
	}
#endif	
	anna_alloc_mark_type(o->input_type[i]);
    }

    if(o->code)
	anna_vm_mark_code(o);
}

void anna_alloc_mark_stack_template(anna_stack_template_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;
    
    if(o->parent)
	anna_alloc_mark_stack_template(o->parent);
    if(o->function)
	anna_alloc_mark_function(o->function);
    if(o->wrapper)
	anna_alloc_mark_object(o->wrapper);
    int i;
    for(i=0; i<o->count; i++)
    {
	if(o->member_declare_node[i])
	{
	    anna_alloc_mark_node((anna_node_t *)o->member_declare_node[i]);
	}
    }
}

void anna_alloc_mark_node(anna_node_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;

    anna_node_t *this = o;

    if(o->wrapper)
	anna_alloc_mark_object(o->wrapper);
    
    switch(this->node_type)
    {
	
	case ANNA_NODE_NULL:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_INTERNAL_IDENTIFIER:
	{
	    break;
	}
	
	case ANNA_NODE_RETURN:
	case ANNA_NODE_BREAK:
	case ANNA_NODE_CONTINUE:
	case ANNA_NODE_USE:
	case ANNA_NODE_TYPE_OF:
	case ANNA_NODE_INPUT_TYPE_OF:
	case ANNA_NODE_RETURN_TYPE_OF:
	{
	    anna_error(this, L"Unimplemented node type during gc. Come back tomorrow.");
	    CRASH;
	    break;
	}		
	
	case ANNA_NODE_CAST:
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	case ANNA_NODE_SPECIALIZE:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    int i;
#ifdef ANNA_CHECK_GC
	    if(!n->function)
	    {
		anna_error(n, L"Critical: Invalid AST node");
		anna_node_print(5, n);		
		CRASH;
	    }
#endif
	    anna_alloc_mark_node(n->function);
	    for(i=0; i<n->child_count; i++)
	    {
		anna_alloc_mark_node(n->child[i]);
	    }
	    
	    break;
	}
	
	
	case ANNA_NODE_MEMBER_CALL:
	case ANNA_NODE_STATIC_MEMBER_CALL:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    int i;
		
	    anna_alloc_mark_node(n->object);
	    for(i=0; i<n->child_count; i++)
	    {
		anna_alloc_mark_node(n->child[i]);
	    }
	    
	    break;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;	    
	    anna_alloc_mark_function(c->payload);
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    anna_alloc_mark_type(f);
	    break;
	}

	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *c = (anna_node_assign_t *)this;
	    anna_alloc_mark_node(c->value);
	    break;
	}


	case ANNA_NODE_MEMBER_BIND:
	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_STATIC_MEMBER_GET:
	{
	    anna_node_member_access_t *c = (anna_node_member_access_t *)this;
	    anna_alloc_mark_node(c->object);
	    break;
	}

	case ANNA_NODE_MEMBER_SET:
	case ANNA_NODE_STATIC_MEMBER_SET:
	{
	    anna_node_member_access_t *g = (anna_node_member_access_t *)this;
	    anna_alloc_mark_node(g->object);
	    anna_alloc_mark_node(g->value);
	    break;
	}
	
	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	{
	    anna_node_declare_t *d = (anna_node_declare_t *)this;
	    anna_alloc_mark_node(d->value);
	    anna_alloc_mark_node(d->type);
	    break;
	}
	
	case ANNA_NODE_MAPPING:
	case ANNA_NODE_OR:
	case ANNA_NODE_AND:
	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_alloc_mark_node(d->arg1);
	    anna_alloc_mark_node(d->arg2);
	    break;   
	}

	case ANNA_NODE_IF:
	{
	    anna_node_if_t *d = (anna_node_if_t *)this;
	    anna_alloc_mark_node(d->cond);
	    anna_alloc_mark_node((anna_node_t *)d->block1);
	    anna_alloc_mark_node((anna_node_t *)d->block2);
	    break;
	}	
	
	case ANNA_NODE_DUMMY:
	{
	    anna_node_dummy_t *d = (anna_node_dummy_t *)this;
	    anna_alloc_mark_object(d->payload);
	    break;   
	}

	default:
	{
	    anna_error(
		this,
		L"Don't know how to handle node of type %d during garbage collection", this->node_type);	    
	    CRASH;
	    break;
	}
    }
}


static void anna_alloc_mark_blob(void *mem)
{
    int *mem2 = (int *)mem;
    mem2[-2] |= ANNA_USED;
}

void anna_alloc_mark_entry(anna_entry_t *e)
{
    if(!e)
	return;
    
    if(!anna_is_obj(e))
    {
	if(anna_is_float(e))
	{
	    anna_alloc_mark_blob(anna_as_float_payload(e));
	    return;
	}
	return;
    }
    anna_object_t *obj = anna_as_obj_fast(e);
    anna_alloc_mark_object(obj);
}

static void anna_alloc_mark_activation_frame(anna_activation_frame_t *frame)
{
    if( frame->flags & ANNA_USED)
	return;
    frame->flags |= ANNA_USED;    

    int i;
    
    for(i=0; i<frame->function->variable_count; i++)
    {	
	anna_alloc_mark_entry(frame->slot[i]);
    }
    if(frame->dynamic_frame)
	anna_alloc_mark_activation_frame(frame->dynamic_frame);
    anna_alloc_mark_function(frame->function);
}

static void anna_alloc_mark_context(anna_context_t *context)
{
    anna_entry_t **obj;
    for(obj = &context->stack[0]; obj < context->top; obj++)
    {	
	anna_alloc_mark_entry(*obj);
    }
    anna_alloc_mark_activation_frame(context->frame);
}

void anna_alloc_mark(void *obj)
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
	case ANNA_ACTIVATION_FRAME:
	{
	    anna_alloc_mark_activation_frame((anna_activation_frame_t *)obj);
	    break;
	}
	default:
	{
	    CRASH;
	}
    }
}

static void free_val(void *key, void *value)
{
    free(value);
}

static void anna_alloc_free(void *obj)
{
    
    switch(*((int *)obj) & ANNA_ALLOC_MASK)
    {
	case ANNA_OBJECT:
	{
	    anna_object_t *o = (anna_object_t *)obj;
	    int i;
	    for(i=0; i<o->type->finalizer_count; i++)
	    {
		o->type->finalizer[i](o);
	    }
	    anna_alloc_count -= o->type->object_size;
	    anna_slab_free(obj, o->type->object_size);

	    break;
	}
	case ANNA_TYPE:
	{
	    int i;
	    anna_type_t *o = (anna_type_t *)obj;
	    
//	    wprintf(L"Discarding unused type %ls %d\n", o->name, o);
	    
	    if(obj != null_type)
	    {
		for(i=0; i<anna_type_get_member_count(o); i++)
		{
		    anna_slab_free(
			anna_type_get_member_idx(o, i),
			sizeof(anna_member_t));
		}
	    }
	    
	    free(o->static_member);
	    free(o->mid_identifier);
	    
	    anna_alloc_count -= sizeof(anna_type_t);
	    anna_slab_free(obj, sizeof(anna_type_t));
	    break;
	}
	case ANNA_ACTIVATION_FRAME:
	{
	    anna_activation_frame_t *o = (anna_activation_frame_t *)obj;
	    anna_alloc_count -= o->function->frame_size;
//	    wprintf(L"FRAMED %ls\n", o->function->name);
	    
	    anna_slab_free(o, o->function->frame_size);
	    break;
	}
	case ANNA_FUNCTION:
	{
	    anna_function_t *o = (anna_function_t *)obj;
	    free(o->code);
	    free(o->input_type);
	    anna_alloc_count -= sizeof(anna_function_t);
	    anna_slab_free(obj, sizeof(anna_function_t));
	    break;
	}

	case ANNA_NODE:
	{
	    anna_node_t *o = (anna_node_t *)obj;
	    
	    switch(o->node_type)
	    {
		case ANNA_NODE_CALL:
		case ANNA_NODE_CONSTRUCT:
		case ANNA_NODE_MEMBER_CALL:
		case ANNA_NODE_STATIC_MEMBER_CALL:
	        case ANNA_NODE_SPECIALIZE:
		case ANNA_NODE_CAST:
		{
		    anna_node_call_t *n = (anna_node_call_t *)o;
		    free(n->child);
		    break;
		}
		case ANNA_NODE_INT_LITERAL:
		{
		    anna_node_int_literal_t *n = (anna_node_int_literal_t *)o;
		    mpz_clear(n->payload);
		    break;
		}
		case ANNA_NODE_STRING_LITERAL:
		{
		    anna_node_string_literal_t *n = (anna_node_string_literal_t *)o;
		    if(n->free)
		    {
		        free(n->payload);
		    }
		    break;
		}
	    }
	    free(obj);
	    break;
	}

	case ANNA_STACK_TEMPLATE:
	{
	    int i;
	    anna_stack_template_t *o = (anna_stack_template_t *)obj;
	    free(o->member_declare_node);
	    free(o->member_flags);
	    for(i=0; i<al_get_count(&o->import); i++)
	    {
		free(al_get(&o->import, i));
	    }
	    al_destroy(&o->import);
	    for(i=0; i<al_get_count(&o->expand); i++)
	    {
		free(al_get(&o->expand, i));
	    }
	    al_destroy(&o->expand);
	    hash_foreach(&o->member_string_identifier, free_val);
	    hash_destroy(&o->member_string_identifier);
	    anna_alloc_count -= sizeof(anna_stack_template_t);
	    anna_slab_free(obj, sizeof(anna_stack_template_t));
	    break;
	}
	case ANNA_BLOB:
	{
//	    wprintf(L"DA BLOB\n");
	    
	    int *blob = (int *)obj;
	    size_t sz = blob[1];
	    anna_alloc_count -= sz;
	    anna_slab_free(obj, sz);
	    break;
	}
	default:
	{
	    break;
	}
    }
}

void anna_alloc_gc_block()
{
    anna_alloc_gc_block_counter++;
}

void anna_alloc_gc_unblock()
{
    anna_alloc_gc_block_counter--;
}

void anna_gc(anna_context_t *context)
{
    al_truncate(&anna_alloc_todo, 0);
    
    if(anna_alloc_gc_block_counter)
    {
	anna_alloc_tot += anna_alloc_count;
	anna_alloc_count = 0;
	return;
    }
    
    anna_alloc_gc_block();
    size_t j, i;
    
#ifdef ANNA_CHECK_GC_LEAKS
    int old_anna_alloc_count = anna_alloc_count;
    int s_count[]={0,0,0,0,0,0,0,0,0,0,0,0,0};
    size_t start_count = 0;
    
    for(j=0; j<ANNA_ALLOC_TYPE_COUNT; j++)
    {
	s_count[j] = al_get_count(&anna_alloc[j]);
	start_count += al_get_count(&anna_alloc[j]);
    }
    

#endif
    
    anna_activation_frame_t *f = context->frame;
    while(f)
    {
	anna_alloc_unmark(f);
	f = f->dynamic_frame;
    }
    
    anna_alloc_mark_context(context);	
    anna_type_mark_static();    
    anna_reflection_mark_static();    
    anna_list_mark_static();    
    anna_hash_mark_static();    
    anna_alloc_mark_object(null_object);
    anna_alloc_mark(anna_stack_wrap(stack_global));
    while(al_get_count(&anna_alloc_todo))
    {
	anna_object_t *obj = (anna_object_t *)al_pop(&anna_alloc_todo);
	obj->type->mark_object(obj);
    }
    
    int freed = 0;
    
    for(j=0; j<ANNA_ALLOC_TYPE_COUNT; j++)
    {
	for(i=0; i<al_get_count(&anna_alloc[j]); i++)
	{
	    void *el = al_get_fast(&anna_alloc[j], i);
	    int flags = *((int *)el);
	    if(!(flags & ANNA_USED))
	    {
		freed++;
		anna_alloc_free(el);
		al_set_fast(&anna_alloc[j], i, al_get_fast(&anna_alloc[j], al_get_count(&anna_alloc[j])-1));
		al_truncate(&anna_alloc[j], al_get_count(&anna_alloc[j])-1);
		i--;
	    }
	    else
	    {
		anna_alloc_unmark(el);	    
	    }
	}
	al_resize(&anna_alloc[j]);
    }

/*
	stack_ptr = stack;
	while(stack_ptr)
	{
	    wprintf(L"\n\n\nFASDFADS %ls\n", stack_ptr->function ? stack_ptr->function->name : L"ANON");
	anna_entry_t **obj;
	//wprintf(L"FASFAS %d %d\n", stack_ptr->top - &stack_ptr->base[0], stack_ptr->function->input_count);

	for(obj = &stack_ptr->base[0]; obj < stack_ptr->top; obj++)
	{	
	    wprintf(L"Pos %d\n", obj - stack_ptr->base);
	    if(!*obj)
	    {
		wprintf(L"Pos %d is C null pointer\n");		
	    }
	    else if(anna_is_obj(*obj))
	    {
		anna_object_print(anna_as_obj(*obj));
	    }
	    else
	    {
		wprintf(L"Pos %d is not an object\n", obj - stack_ptr->base);
	    }
	    fflush(stdout);	    
	}
	stack_ptr = stack_ptr->caller;
	
	}
*/	
    anna_slab_reclaim();
//    wprintf(L"GC\n");

#ifdef ANNA_CHECK_GC_LEAKS
    size_t end_count = 0;
    int o_count[]={0,0,0,0,0,0,0,0,0,0,0,0,0};
    for(j=0; j<ANNA_ALLOC_TYPE_COUNT; j++)
    {
	o_count[j] = al_get_count(&anna_alloc[j]);
	end_count += al_get_count(&anna_alloc[j]);
    }
    
    wchar_t *name[]=
	{
	    L"object", L"activation frame", L"type", L"stack template", 
	    L"AST node", L"execution context", L"function", L"blob"
	}
    ;

    wprintf(L"Collected %d elements\n", start_count-end_count);
    for(i=0; i<ANNA_ALLOC_TYPE_COUNT; i++)
    {
	wprintf(L"Collected %d elements of type %ls, after gc, %d elements remain\n", s_count[i] - o_count[i] , name[i], o_count[i]);
    }

    int old_anna_alloc_tot = anna_alloc_tot;
    
#endif
    
    anna_alloc_tot += anna_alloc_count;
    anna_alloc_count = 0;
    anna_alloc_count_next_gc = maxi(anna_alloc_tot >> 1, GC_FREQ);
    
#ifdef ANNA_CHECK_GC_LEAKS
    wprintf(
	L"Collected %d bytes. %d bytes currently in use. Next GC in %d bytes\n",
	old_anna_alloc_tot + old_anna_alloc_count - anna_alloc_tot,
	anna_alloc_tot, 
	anna_alloc_count_next_gc);
#endif
    
//    wprintf(L"GC cycle performed, %d allocations freed, %d remain\n", freed, al_get_count(&anna_alloc));
    anna_alloc_gc_unblock();
//    wprintf(L"After GC. We have %d allocated objects\n", al_get_count(&anna_alloc));
//    anna_slab_print();
    
}

#ifdef ANNA_FULL_GC_ON_SHUTDOWN
void anna_gc_destroy(void)
{
    anna_alloc_run_finalizers=0;
    anna_gc();
}
#endif
