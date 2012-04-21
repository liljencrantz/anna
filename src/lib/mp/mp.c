#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#include "anna/common.h"
#include "anna/util.h"
#include "anna/base.h"
#include "anna/function.h"
#include "anna/function_type.h"
#include "anna/mid.h"
#include "anna/type_data.h"
#include "anna/module.h"
#include "anna/lib/clib.h"
#include "anna/vm.h"
#include "anna/intern.h"
#include "anna/type.h"
#include "anna/member.h"

anna_type_t *channel_type, *task_type;

static array_list_t anna_mp_thread = AL_STATIC;
static pthread_mutex_t anna_mp_thread_mutex = PTHREAD_MUTEX_INITIALIZER;

const static anna_type_data_t anna_mp_type_data[] = 
{
    { &channel_type, L"Channel" },
}
    ;

static void *anna_mp_run_main(void *aux)
{
    anna_entry_t **arg = (anna_entry_t **)aux;

    anna_object_t *fun = (anna_object_t *)arg[0];
    
    anna_entry_t *argv[] = 
	{
	    arg[1]
	}
    ;
    free(arg);
    anna_alloc_init_thread();
    anna_vm_run(fun, 1, argv);
    return 0;
}
typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}
    anna_channel_sync_t;

ANNA_VM_NATIVE(anna_mp_run, 1)
{
    if(param[0] == null_entry)
    {
	return null_entry;
    }

    /* Create client and server channels */
    anna_object_t *cchan = anna_object_create(channel_type);
    anna_object_t *schan = anna_object_create(channel_type);

    anna_entry_t **csink = anna_alloc_blob(sizeof(anna_entry_t *));
    anna_entry_t **ssink = anna_alloc_blob(sizeof(anna_entry_t *));
    anna_channel_sync_t *cond = anna_alloc_blob(sizeof(anna_channel_sync_t));
    pthread_mutex_init(&cond->mutex, 0);
    pthread_cond_init(&cond->cond, 0);
    
    *csink = 0;
    *ssink = 0;

    anna_entry_set(cchan, ANNA_MID_CHANNEL_READ, (anna_entry_t *)csink);
    anna_entry_set(cchan, ANNA_MID_CHANNEL_WRITE, (anna_entry_t *)ssink);
    anna_entry_set(cchan, ANNA_MID_CHANNEL_SYNC, (anna_entry_t *)cond);

    anna_entry_set(schan, ANNA_MID_CHANNEL_READ, (anna_entry_t *)ssink);
    anna_entry_set(schan, ANNA_MID_CHANNEL_WRITE, (anna_entry_t *)csink);
    anna_entry_set(schan, ANNA_MID_CHANNEL_SYNC, (anna_entry_t *)cond);
    
    pthread_t *thread = malloc(sizeof(pthread_t));
    
    anna_entry_t **arg = malloc(sizeof(anna_entry_t *)*2);
    arg[0] = param[0];
    arg[1] = (anna_entry_t *)cchan;
    pthread_create(
	thread,
	0, &anna_mp_run_main,
	arg);
    
    pthread_mutex_lock(&anna_mp_thread_mutex);
    al_push(&anna_mp_thread, thread);
    pthread_mutex_unlock(&anna_mp_thread_mutex);

    return anna_from_obj(schan);
}

void anna_mp_join()
{
    pthread_mutex_lock(&anna_mp_thread_mutex);
    while(al_get_count(&anna_mp_thread))
    {
	pthread_t *thread = (pthread_t *)al_pop(&anna_mp_thread);
	pthread_mutex_unlock(&anna_mp_thread_mutex);    
	pthread_join(*thread, 0);
	free(thread);
	pthread_mutex_lock(&anna_mp_thread_mutex);
    }
    pthread_mutex_unlock(&anna_mp_thread_mutex);    
}

ANNA_VM_NATIVE(anna_mp_channel_init, 1)
{
    return null_entry;
}

ANNA_VM_NATIVE(anna_mp_channel_read, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *chan = anna_as_obj(param[0]);

    anna_channel_sync_t *cond = (anna_channel_sync_t *)anna_entry_get(chan, ANNA_MID_CHANNEL_SYNC);
    anna_entry_t **data = (anna_entry_t **)anna_entry_get(chan, ANNA_MID_CHANNEL_READ);

//    anna_message(L"Client before lock\n");
    pthread_mutex_lock(&cond->mutex);
//    anna_message(L"Client after lock\n");
    while(!*data)
    {
	pthread_cond_wait(&cond->cond, &cond->mutex);	
//	anna_message(L"Client after wait\n");
    }
    anna_entry_t *res = *data;
    *data = 0;
    pthread_cond_signal(&cond->cond);
    pthread_mutex_unlock(&cond->mutex);    
//   anna_message(L"Client after unlock\n");

    return res;
}

ANNA_VM_NATIVE(anna_mp_channel_write, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);

    anna_object_t *chan = anna_as_obj(param[0]);

    anna_channel_sync_t *cond = (anna_channel_sync_t *)anna_entry_get(chan, ANNA_MID_CHANNEL_SYNC);
    anna_entry_t **data = (anna_entry_t **)anna_entry_get(chan, ANNA_MID_CHANNEL_WRITE);
//    anna_message(L"Server before lock\n");
    
    pthread_mutex_lock(&cond->mutex);
    while(*data)
    {
	pthread_cond_wait(&cond->cond, &cond->mutex);	
    }
//    anna_message(L"Server after lock\n");
    *data = param[1];
    pthread_cond_signal(&cond->cond);
//    anna_message(L"Server after signal\n");
    pthread_mutex_unlock(&cond->mutex);    
//    anna_message(L"Server after unlock\n");
    return param[0];
}

void anna_mp_create_types(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_mp_type_data, stack_lang);    
}

void anna_mp_load(anna_stack_template_t *stack)
{
    static wchar_t *ta_argn[]={L"channel"};
    anna_type_t *ta_argv[]={channel_type};

    anna_member_create(
	channel_type,
	ANNA_MID_CHANNEL_SYNC,
	ANNA_MEMBER_ALLOC,
	null_type);

    anna_member_create(
	channel_type,
	ANNA_MID_CHANNEL_READ,
	ANNA_MEMBER_ALLOC,
	null_type);

    anna_member_create(
	channel_type,
	ANNA_MID_CHANNEL_WRITE,
	ANNA_MEMBER_ALLOC,
	null_type);

    anna_type_t *write_argv[] = 
	{
	    channel_type,
	    object_type
	}
    ;

    wchar_t *write_argn[]=
	{
	    L"this", L"value"
	}
    ;


    anna_member_create_native_method(
	channel_type,
	anna_mid_get(L"__init__"), 0,
	&anna_mp_channel_init, 
	channel_type, 
	1,
	write_argv, write_argn, 0, L"Channel objects can not be manually created, they are a by-product of calling mp.run. Calling Channel will always return null.");


    anna_member_create_native_method(
	channel_type,
	anna_mid_get(L"read"), 0,
	&anna_mp_channel_read, 
	object_type, 
	1,
	write_argv, write_argn, 0, L"Read the next message from the channel");


    anna_member_create_native_method(
	channel_type,
	anna_mid_get(L"write"), 0,
	&anna_mp_channel_write, 
	channel_type, 
	2,
	write_argv, write_argn, 0, L"Write a message to the channel");


    task_type = anna_type_get_function(
	object_type,
	1, ta_argv, ta_argn, 0,
	0);

    static wchar_t *co_argn[]={L"routine"};
    anna_type_t *co_argv[]={task_type};
    
    anna_module_function(
	stack,
	L"run", 0,
	&anna_mp_run, 
	channel_type, 
	1, co_argv, co_argn, 0,
	L"Create a new instance of the specified coroutine.");

    anna_type_data_register(
	anna_mp_type_data, stack);
}
