#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif
#include <sys/time.h>
#include <stddef.h>

#include "anna/fallback.h"
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

#include "src/alloc/slab.c"

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
static pthread_cond_t anna_alloc_cond_work = PTHREAD_COND_INITIALIZER;
int anna_alloc_flag_gc = 0;

static array_list_t anna_alloc_alloc = AL_STATIC;

/*
  The number of currently executing worker threads
*/
static int anna_alloc_work_count = 0;
static int anna_alloc_work_count_tot = 0;

static array_list_t anna_alloc_gc_context = AL_STATIC;

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


#include "src/alloc/mark.c"
#include "src/alloc/free.c"

void anna_alloc_gc_block()
{
    anna_alloc_gc_block_counter++;
}

void anna_alloc_gc_unblock()
{
    anna_alloc_gc_block_counter--;
}

static void *anna_gc_main(void *);

void anna_alloc_init_thread()
{
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    anna_alloc_t *alloc = calloc(sizeof(anna_alloc_t), 1);
    alloc->idx = al_get_count(&anna_alloc_alloc);
    pthread_setspecific(anna_alloc_key, alloc);

    al_push(&anna_alloc_alloc, alloc);
//    anna_alloc_work_count_tot++;
    pthread_mutex_unlock(&anna_alloc_mutex_gc);
//    anna_message(L"Worker thread %d initialized and ready to run\n", anna_alloc_data()->idx);
}

void anna_alloc_add_thread()
{
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    anna_alloc_work_count_tot++;
    anna_alloc_work_count++;
//    anna_message(L"Create thread - we now have %d work threads running.\n", anna_alloc_work_count_tot);
    pthread_mutex_unlock(&anna_alloc_mutex_gc);
}

void anna_alloc_destroy_main_thread()
{
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    anna_alloc_work_count--;
    anna_alloc_work_count_tot--;

    anna_alloc_t *alloc = anna_alloc_data();
    if(al_get_count(&anna_alloc_alloc)>1)
    {
	anna_alloc_t *other_thread_alloc = al_get_fast(
	    &anna_alloc_alloc,
	    al_get_count(&anna_alloc_alloc)-1);
	other_thread_alloc->idx = alloc->idx;
	al_set_fast(
	    &anna_alloc_alloc, 
	    alloc->idx, 
	    other_thread_alloc);
    }
    al_truncate(
	&anna_alloc_alloc,
	al_get_count(&anna_alloc_alloc)-1);
//    anna_message(L"Destroy thread %d - we have %d work threads running.\n", alloc->idx, anna_alloc_work_count_tot);

    if(anna_alloc_flag_gc && anna_alloc_work_count==0)
    {
	if(pthread_cond_signal(&anna_alloc_cond_gc))
	{
	    anna_message(L"Failed to signal GC thread\n");
	    CRASH;
	}	
//	anna_message(L"Worker %d: Send wake up signal to GC thread\n", anna_alloc_data()->idx);
    }
      
    pthread_mutex_unlock(&anna_alloc_mutex_gc);
}


void anna_alloc_destroy_thread()
{
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    anna_alloc_work_count--;
    anna_alloc_work_count_tot--;
    
    anna_alloc_t *alloc = anna_alloc_data();
    if(al_get_count(&anna_alloc_alloc)>1)
    {
	anna_alloc_t *other_thread_alloc = al_get_fast(
	    &anna_alloc_alloc,
	    al_get_count(&anna_alloc_alloc)-1);
	other_thread_alloc->idx = alloc->idx;
	al_set_fast(
	    &anna_alloc_alloc, 
	    alloc->idx, 
	    other_thread_alloc);
    }
    al_truncate(
	&anna_alloc_alloc,
	al_get_count(&anna_alloc_alloc)-1);
//    anna_message(L"Destroy thread %d - we have %d work threads running.\n", alloc->idx, anna_alloc_work_count_tot);

    if(anna_alloc_flag_gc && anna_alloc_work_count==0)
    {
	if(pthread_cond_signal(&anna_alloc_cond_gc))
	{
	    anna_message(L"Failed to signal GC thread\n");
	    CRASH;
	}	
//	anna_message(L"Worker %d: Send wake up signal to GC thread\n", anna_alloc_data()->idx);
    }
        
    pthread_mutex_unlock(&anna_alloc_mutex_gc);    
}

void anna_gc_init()
{
    pthread_key_create(&anna_alloc_key, &free);
    anna_alloc_init_thread();
    anna_alloc_work_count_tot++;
    anna_alloc_work_count++;
    
    //  anna_message(L"Init of GC.\n");
    pthread_create(
	&anna_alloc_gc_thread,
	0, &anna_gc_main,
	0);
//    anna_message(L"Main thread has returned from GC thread creation.\n");
}

void anna_alloc_pause_worker(anna_context_t *context, pthread_cond_t *cond, pthread_mutex_t *mutex)
{
//    anna_message(L"Check GC status in paused thread %d\n", anna_alloc_data()->idx);
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    if(anna_alloc_flag_gc)
    {
	pthread_mutex_unlock(&anna_alloc_mutex_gc); 
//	anna_message(L"Paused thread %d starting GC\n", anna_alloc_data()->idx);
	anna_gc(context);
//	anna_message(L"Paused thread %d going back to waiting\n", anna_alloc_data()->idx);

	pthread_mutex_lock(&anna_alloc_mutex_gc);
	anna_alloc_data()->context = context;
	anna_alloc_data()->cond = cond;
	anna_alloc_data()->mutex = mutex;
	anna_alloc_data()->sleep = 1;
	pthread_mutex_unlock(&anna_alloc_mutex_gc);
    }
    else
    {
	anna_alloc_data()->context = context;
	anna_alloc_data()->cond = cond;
	anna_alloc_data()->mutex = mutex;
	anna_alloc_data()->sleep = 1;
	pthread_mutex_unlock(&anna_alloc_mutex_gc);
    }
}

void anna_alloc_unpause_worker()
{
//    anna_message(L"Unpause thread\n");
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    anna_alloc_data()->sleep = 0;
    pthread_mutex_unlock(&anna_alloc_mutex_gc);
}

void anna_gc(anna_context_t *context)
{
    if(anna_alloc_gc_block_counter)
    {
	return;
    }
    int i;
    
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    
    anna_alloc_data()->work = 0;

    /*
      Find all threads that are waiting for something non-gc related and wake them up.
    */
    for(i=0; i<al_get_count(&anna_alloc_alloc); i++)
    {
	anna_alloc_t *alloc = (anna_alloc_t *)al_get_fast(&anna_alloc_alloc, i);
	if(alloc->sleep)
	{
//	    anna_message(L"Wake up thread %d!\n", alloc->idx);
	    pthread_mutex_unlock(&anna_alloc_mutex_gc);
	    if(alloc->sleep)
	    {
		alloc->sleep = 0;
		pthread_cond_signal(alloc->cond);
	    }
	    pthread_mutex_lock(&anna_alloc_mutex_gc);
//	    anna_message(L"Thread %d sent signal!\n", alloc->idx);    
	}
    }

    anna_alloc_work_count--;
/*    anna_message(
	L"Worker %d: Time for some GC (%d threads still running)\n",
	anna_alloc_data()->idx, anna_alloc_work_count);    */
    int do_signal = (anna_alloc_work_count == 0);
  

    anna_alloc_flag_gc = 1;
    al_push(&anna_alloc_gc_context, context);

    /*
      If we're the last work thread to report in, wake up the GC thread
    */
    if(do_signal)
    {
	if(pthread_cond_signal(&anna_alloc_cond_gc))
	{
	    anna_message(L"Failed to signal GC thread\n");
	    CRASH;
	}	
//	anna_message(L"Worker %d: Send wake up signal to GC thread\n", anna_alloc_data()->idx);
    }

    /*
      Wait for GC thread to finish
    */
    while(1)
    {
	if(anna_alloc_data()->work)
	{
	    break;
	}
	pthread_cond_wait(&anna_alloc_cond_work, &anna_alloc_mutex_gc);    
//	anna_message(L"Worker %d: Woke up! %d\n", anna_alloc_data()->idx, anna_alloc_data());
    }
//    anna_alloc_work_count++;
    pthread_mutex_unlock(&anna_alloc_mutex_gc);
    
/*    anna_message(
	L"Worker %d: Leaving to do some proper work %d/%d\n",
	anna_alloc_data()->idx,
	anna_alloc_data()->count,
	anna_alloc_data()->count_next_gc
	);*/
}

static void anna_alloc_gc_wait_for_work_thread()
{
//    anna_message(L"GC: Done with this GC pass, time to wait for workers to finish\n");
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    while(1)
    {
	if(anna_alloc_work_count == 0 && anna_alloc_flag_gc == 1)
	{
	    anna_alloc_flag_gc = 0;
	    pthread_mutex_unlock(&anna_alloc_mutex_gc);
//	    anna_message(L"GC: Start work\n");
	    break;
	}
	else
	{
//	    anna_message(L"GC: Woken up while there are still threads running\n");
	}
	
	pthread_cond_wait(&anna_alloc_cond_gc, &anna_alloc_mutex_gc);    
    }
}

static void anna_alloc_gc_start_work_thread()
{
//    anna_message(L"GC: Start worker threads!\n");
    int i;
    
    pthread_mutex_lock(&anna_alloc_mutex_gc);
    al_truncate(&anna_alloc_gc_context, 0);
    for(i=0; i<al_get_count(&anna_alloc_alloc); i++)
    {
	anna_alloc_t *alloc = (anna_alloc_t *)al_get_fast(&anna_alloc_alloc, i);
//	anna_message(L"GC: Wake thread %d @ %d!\n", alloc->idx, alloc);
	alloc->count = 0;
	alloc->count_next_gc = GC_FREQ;
	alloc->work = 1;
    }
    anna_alloc_work_count = anna_alloc_work_count_tot;	    
    pthread_cond_broadcast(&anna_alloc_cond_work);    
    pthread_mutex_unlock(&anna_alloc_mutex_gc);
}

static void anna_alloc_gc_collect()
{
    size_t j, i;
    
    anna_alloc_timer_start();
    anna_slab_free_return();
    anna_slab_reclaim();
    anna_alloc_timer_stop(anna_alloc_time_reclaim);
    
    anna_alloc_timer_start();
    
    al_truncate(&anna_alloc_todo, 0);
	
    anna_alloc_gc_block();
    
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
//    anna_message(L"Unmark %d contexts from %d threads\n", al_get_count(&anna_alloc_gc_context), anna_alloc_work_count_tot);
    assert(al_get_count(&anna_alloc_gc_context) == anna_alloc_work_count_tot);
    for(i=0; i<al_get_count(&anna_alloc_gc_context); i++)
    {
	anna_context_t *context = al_get_fast(&anna_alloc_gc_context, i);

	anna_activation_frame_t *f = context->frame;
	while(f)
	{
	    anna_alloc_unmark(f);
	    f = f->dynamic_frame;
	}
//    anna_message(L"Unmarked frames.\n");	
	anna_alloc_mark_context(context);
    }

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
	anna_alloc_mark(al_get_fast(&anna_alloc_permanent, i));
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

static void *anna_gc_main(void *aux)
{
    prctl(PR_SET_NAME,"anna/gc",0,0,0);
    
    while(1)
    {
	anna_alloc_gc_wait_for_work_thread();
//	anna_message(L"GC: Time for GC! %d/%d worker threads running. %d contexts pushed.\n", anna_alloc_work_count, anna_alloc_work_count_tot, al_get_count(&anna_alloc_gc_context));

	if(anna_alloc_gc_block_counter)
	{
	    anna_alloc_gc_start_work_thread();    
	    continue;
	}
	
	anna_alloc_gc_collect();
	anna_alloc_gc_start_work_thread();
	anna_alloc_gc_free();
    }    
    return 0;
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
