#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <stddef.h>

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

static array_list_t anna_alloc_tmp[ANNA_ALLOC_TYPE_COUNT];

int anna_alloc_tot=0;
int anna_alloc_count=0;
int anna_alloc_count_next_gc=1024*1024;
int anna_alloc_gc_block_counter;
int anna_alloc_run_finalizers=1;
array_list_t anna_alloc_todo = AL_STATIC;
array_list_t anna_alloc_permanent = AL_STATIC;
pthread_t anna_alloc_gc_thread;

static pthread_mutex_t anna_alloc_mutex_gc = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t anna_alloc_cond_gc = PTHREAD_COND_INITIALIZER;
static int anna_alloc_flag_gc = 0;

static pthread_mutex_t anna_alloc_mutex_work = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t anna_alloc_cond_work = PTHREAD_COND_INITIALIZER;
static int anna_alloc_flag_work = 1;

static anna_context_t *anna_alloc_gc_context;

static array_list_t anna_alloc_internal[ANNA_ALLOC_TYPE_COUNT] = {
    AL_STATIC,
    AL_STATIC,
    AL_STATIC,
    AL_STATIC,
    AL_STATIC,
    AL_STATIC,
    AL_STATIC
}
    ;

#ifdef ANNA_CHECK_GC_TIMING

static long long anna_alloc_time_collect = 0;
static long long anna_alloc_time_free = 0;
static long long anna_alloc_time_reclaim = 0;
static long long anna_alloc_time_start;

static long long anna_alloc_time()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return 1000000ll * tv.tv_sec + tv.tv_usec;
}


static void anna_alloc_timer_start()
{
    anna_alloc_time_start = anna_alloc_time();
}

#define anna_alloc_timer_stop(name) name += anna_alloc_time() - anna_alloc_time_start

#else
#define anna_alloc_timer_start()
#define anna_alloc_timer_stop(name)
#endif

void anna_alloc_mark_permanent(void *alloc)
{
    al_push(&anna_alloc_permanent, alloc);
}

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
//    anna_message(L"WEE %ls\n", o->return_type->name);
    
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
	    anna_message(L"No type specified for input argument %d of function %ls\n", 
		    i+1, o->name);
	    CRASH;
	}
#endif	
	anna_alloc_mark_type(o->input_type[i]);
    }

    if(o->code)
	anna_vm_mark_code(o);
}

void anna_alloc_mark_stack_template(anna_stack_template_t *stack)
{
    if( stack->flags & ANNA_USED)
	return;
    stack->flags |= ANNA_USED;

    if(stack->parent)
	anna_alloc_mark_stack_template(stack->parent);
    if(stack->function)
	anna_alloc_mark_function(stack->function);
    if(stack->wrapper)
	anna_alloc_mark_object(stack->wrapper);
    int i;
    for(i=0; i<stack->count; i++)
    {
	if(stack->member_declare_node[i])
	{
	    anna_alloc_mark_node((anna_node_t *)stack->member_declare_node[i]);
	}
    }
    for(i=0; i<al_get_count(&stack->expand); i++)
    {
	anna_use_t *use = al_get(&stack->expand, i);
	anna_alloc_mark_node(use->node);
	anna_alloc_mark_type(use->type);
    }
    for(i=0; i<al_get_count(&stack->import); i++)
    {
	anna_use_t *use = al_get(&stack->import, i);
	anna_alloc_mark_node(use->node);
	anna_alloc_mark_type(use->type);
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
	case ANNA_NODE_TYPE_OF:
	case ANNA_NODE_INPUT_TYPE_OF:
	case ANNA_NODE_RETURN_TYPE_OF:
	{
	    anna_node_wrapper_t *n = (anna_node_wrapper_t *)this;
	    anna_alloc_mark_node(n->payload);
	    break;
	}
	
	case ANNA_NODE_USE:
	{
	    anna_error(this, L"Unimplemented node type %d during gc. Come back tomorrow.", 
		this->node_type);
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
    *mem2 |= ANNA_USED;
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
	case ANNA_FUNCTION_TYPE:
	{
	    break;
	}
	case ANNA_BLOB:
	{
	    anna_alloc_mark_blob(obj);
	    break;
	}
	default:
	{
	    anna_error(0, L"Tried to mark unknown memory region in GC. Type: %d\n", (*((int *)obj) & ANNA_ALLOC_MASK));	    
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
	    anna_alloc_tot -= o->type->object_size;
	    anna_slab_free(obj, o->type->object_size);

	    break;
	}
	case ANNA_TYPE:
	{
	    int i;
	    anna_type_t *o = (anna_type_t *)obj;
	    
//	    anna_message(L"Discarding unused type %ls %d\n", o->name, o);
	    
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
	    
	    anna_alloc_tot -= sizeof(anna_type_t);
	    anna_slab_free(obj, sizeof(anna_type_t));
	    break;
	}
	case ANNA_ACTIVATION_FRAME:
	{
	    anna_activation_frame_t *o = (anna_activation_frame_t *)obj;
	    anna_alloc_tot -= o->function->frame_size;
//	    anna_message(L"FRAMED %ls\n", o->function->name);
	    
	    anna_slab_free(o, o->function->frame_size);
	    break;
	}
	case ANNA_FUNCTION:
	{
	    anna_function_t *o = (anna_function_t *)obj;
	    free(o->code);
	    free(o->input_type);
	    anna_alloc_tot -= sizeof(anna_function_t);
	    anna_slab_free(obj, sizeof(anna_function_t));
	    break;
	}

	case ANNA_NODE:
	{
	    anna_node_t *o = (anna_node_t *)obj;
//	    o->flags |= ANNA_NODE_FREED;
	    
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
//	    anna_alloc_count -= sizeof(anna_stack_template_t);
	    anna_slab_free(obj, sizeof(anna_stack_template_t));
	    break;
	}
	case ANNA_BLOB:
	{
//	    anna_message(L"DA BLOB\n");
	    
	    int *blob = (int *)obj;
	    size_t sz = blob[1];
	    anna_alloc_tot -= sz;
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

static void anna_gc_main(void);

static void anna_alloc_gc_start_work_thread()
{
    anna_alloc_tot += anna_alloc_count;
    anna_alloc_count = 0;
    anna_alloc_count_next_gc = maxi(anna_alloc_tot >> 2, GC_FREQ);
    
    pthread_mutex_lock(&anna_alloc_mutex_work);
    anna_alloc_flag_work = 1;
    pthread_cond_signal(&anna_alloc_cond_work);    
    pthread_mutex_unlock(&anna_alloc_mutex_work);    
}

static void anna_alloc_gc_wait_for_work_thread()
{
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    while(1)
    {
	if(anna_alloc_flag_gc == 1)
	{
	    anna_alloc_flag_gc = 0;		
	    pthread_mutex_unlock(&anna_alloc_mutex_gc);
	    break;
	}
	pthread_cond_wait(&anna_alloc_cond_gc, &anna_alloc_mutex_gc);    
    }
}

void anna_gc_init()
{
    //  anna_message(L"Init of GC.\n");
    pthread_create(
	&anna_alloc_gc_thread,
	0, &anna_gc_main,
	0);
//    anna_message(L"Main thread has returned from GC thread creation.\n");
}

void anna_gc(anna_context_t *context)
{
    anna_alloc_gc_context = context;

    pthread_mutex_lock(&anna_alloc_mutex_work);
    anna_alloc_flag_work = 0;
    pthread_mutex_unlock(&anna_alloc_mutex_work);
    
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    anna_alloc_flag_gc = 1;
    pthread_cond_signal(&anna_alloc_cond_gc);    
    pthread_mutex_unlock(&anna_alloc_mutex_gc);
    
    pthread_mutex_lock(&anna_alloc_mutex_work);
    while(1)
    {
	if(anna_alloc_flag_work == 1)
	{
	    pthread_mutex_unlock(&anna_alloc_mutex_work);
	    break;
	}
	pthread_cond_wait(&anna_alloc_cond_work, &anna_alloc_mutex_work);    
    }
}

static void anna_alloc_gc_collect()
{
    size_t j, i;
    
    anna_alloc_timer_start();
    anna_slab_free_return();
    anna_slab_reclaim();
/*
    for(j=0; j<ANNA_ALLOC_TYPE_COUNT; j++)
    {
	if(anna_alloc_stop[j])
	{
	    array_list_t *al = &anna_alloc[j];
	    if(anna_alloc_stop[j] != al_get_count(al))
	    {
		size_t sz = (al->pos - anna_alloc_stop[j]);
//		anna_message(L"Move %d elements from position %d to begining of array\n",
//			     sz, anna_alloc_stop[j]);
		memmove(
		    &al->arr[0],
		    &al->arr[anna_alloc_stop[j]],
		    sizeof(void *) * sz);
		
		al->pos = sz;
	    }
	    else
	    {
		al->pos = 0;
	    }
	}
    }
*/
    anna_alloc_timer_stop(anna_alloc_time_reclaim);
    
    anna_alloc_timer_start();
    
    anna_context_t *context = anna_alloc_gc_context;
    al_truncate(&anna_alloc_todo, 0);
	
    anna_alloc_gc_block();
//	anna_message(L"B.\n");
    
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
//    	anna_message(L"C.\n");

    anna_activation_frame_t *f = context->frame;
    while(f)
    {
	anna_alloc_unmark(f);
	f = f->dynamic_frame;
    }
//    anna_message(L"Unmarked frames.\n");
    
    anna_alloc_mark_context(context);	
    anna_reflection_mark_static();    
    anna_alloc_mark_object(null_object);
    anna_alloc_mark(anna_stack_wrap(stack_global));
//	anna_message(L"Marked stuff.\n");
    while(al_get_count(&anna_alloc_todo))
    {
	anna_object_t *obj = (anna_object_t *)al_pop(&anna_alloc_todo);
	obj->type->mark_object(obj);
    }
//	anna_message(L"Marked objects.\n");
    
    for(i=0; i<al_get_count(&anna_alloc_permanent); i++)
    {
	anna_alloc_mark(al_get(&anna_alloc_permanent, i));
    }
//    anna_message(L"Marked permanent stuff.\n");

    memcpy(&anna_alloc_tmp, &anna_alloc, sizeof(anna_alloc_tmp));
    
    for(j=0; j<ANNA_ALLOC_TYPE_COUNT; j++)
    {
	al_init(&anna_alloc[j]);
    }

    anna_alloc_timer_stop(anna_alloc_time_collect);
}

static void anna_alloc_gc_free()
{
    anna_alloc_timer_start();
    int i, j;
    int freed = 0;

    for(j=0; j<ANNA_ALLOC_TYPE_COUNT; j++)
    {
	for(i=0; i<al_get_count(&anna_alloc_internal[j]);)
	{
	    void *el = al_get_fast(&anna_alloc_internal[j], i);
	    int flags = *((int *)el);
	    if(!(flags & ANNA_USED))
	    {
		freed++;
		anna_alloc_free(el);
		al_set_fast(&anna_alloc_internal[j], i, al_get_fast(&anna_alloc_internal[j], al_get_count(&anna_alloc_internal[j])-1));
		al_truncate(&anna_alloc_internal[j], al_get_count(&anna_alloc_internal[j])-1);
	    }
	    else
	    {
		anna_alloc_unmark(el);	    
		i++;
	    }
	}
	al_resize(&anna_alloc_internal[j]);
    }

    for(j=0; j<ANNA_ALLOC_TYPE_COUNT; j++)
    {
	for(i=0; i<al_get_count(&anna_alloc_tmp[j]); i++)
	{
	    void *el = al_get_fast(&anna_alloc_tmp[j], i);
	    int flags = *((int *)el);
	    if(!(flags & ANNA_USED))
	    {
		freed++;
		anna_alloc_free(el);
	    }
	    else
	    {
		al_push(&anna_alloc_internal[j], el);
		anna_alloc_unmark(el);	    
	    }
	}
	al_destroy(&anna_alloc_tmp[j]);
    }

    anna_alloc_timer_stop(anna_alloc_time_free);

//    anna_message(L"Freed memory.\n");
    
//	anna_message(L"Reclaimed pools.\n");

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

    anna_message(L"Collected %d elements\n", start_count-end_count);
    for(i=0; i<ANNA_ALLOC_TYPE_COUNT; i++)
    {
	anna_message(L"Collected %d elements of type %ls, after gc, %d elements remain\n", s_count[i] - o_count[i] , name[i], o_count[i]);
    }

    int old_anna_alloc_tot = anna_alloc_tot;
    
#endif
    
#ifdef ANNA_CHECK_GC_LEAKS
    anna_message(
	L"Collected %d bytes. %d bytes currently in use. Next GC in %d bytes\n",
	old_anna_alloc_tot + old_anna_alloc_count - anna_alloc_tot,
	anna_alloc_tot, 
	anna_alloc_count_next_gc);
#endif
    
    anna_alloc_gc_unblock();
//    anna_message(L"GC cycle performed, %d allocations freed, %d remain\n", freed, al_get_count(&anna_alloc));
}

static void anna_gc_main()
{
    prctl(PR_SET_NAME,"anna/gc",0,0,0);
    
    while(1)
    {
	anna_alloc_gc_wait_for_work_thread();
	
	if(anna_alloc_gc_block_counter)
	{
	    anna_alloc_gc_start_work_thread();    
	    continue;
	}
	
	anna_alloc_gc_collect();
	anna_alloc_gc_start_work_thread();
	anna_alloc_gc_free();
    }    
}

void anna_gc_destroy(void)
{
#ifdef ANNA_FULL_GC_ON_SHUTDOWN
    anna_alloc_run_finalizers=0;
    anna_gc();
#endif

#ifdef ANNA_CHECK_GC_TIMING
    anna_message(
	L"The GC spend the following amount of time in these phases:\nCollecting: %lld ms (This pauses other threads)\nFree:ing %lld ms (This runs concurrently)\nReclaiming: %lld ms (This pauses other threads)\n",
	anna_alloc_time_collect/1000, anna_alloc_time_free/1000, anna_alloc_time_reclaim/1000);
    
#endif

}
