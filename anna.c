#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <locale.h>
#include <sys/prctl.h>

#include "common.h"
#include "util.h"
#include "anna_function.h"
#include "anna.h"
#include "anna_module.h"
#include "anna_int.h"
#include "anna_function_type.h"
#include "anna_type.h"
#include "anna_member.h"
#include "anna_status.h"
#include "anna_vm.h"
#include "anna_tt.h"
#include "anna_alloc.h"
#include "anna_slab.h"
#include "anna_mid.h"

anna_stack_template_t *stack_global;
int anna_argc;
char **anna_argv;

/**
   Init the interpreter. 
 */
static void anna_init()
{
    anna_int_init();
    anna_type_init();
    
    anna_mid_init();
    anna_slab_init();
    anna_vm_init();

    stack_global = anna_stack_create(0);
    stack_global->flags |= ANNA_STACK_NAMESPACE;

    anna_abides_init();
    null_object = anna_object_create_raw(anna_align(sizeof(anna_object_t)));
    anna_module_init();
}

/**
   Set the name of the application
 */
static void anna_set_program_name(char *arg)
{    
    char *name = strrchr(arg, '/');
    if(!name)
	name = arg;
    else
	name++;
    
    if(strlen(name) > 15)
    {
	name = strdup(name);
	name[16] = 0;
	prctl(PR_SET_NAME,name,0,0,0);
	free(name);
    }
    else
    {
	prctl(PR_SET_NAME,name,0,0,0);
    }    
}

/**
   Perform shutdown operations
 */
static void anna_shutdown()
{
    #ifdef ANNA_FULL_GC_ON_SHUTDOWN
    anna_gc_destroy();
    anna_vm_destroy();
    anna_mid_destroy();
    hash_foreach(&anna_type_for_function_identifier, fun_key_free);
    hash_destroy(&anna_type_for_function_identifier);
#endif    
}

/**
   Run the function named main in the specified module.
 */
static void anna_main_run(anna_stack_template_t *module)
{
    anna_object_t *main_wrapper = anna_stack_get(module, L"main");

    if(!main_wrapper)
    {
	debug(D_CRITICAL,L"No main method defined\n");
	exit(1);	
    }
    
    debug(D_SPAM,L"Program fully loaded and ready to be executed\n");    
    anna_vm_run(main_wrapper, 0, 0);
}

/**
  Figure out the name of the module we want to run based on the
  command line arguments
 */
static wchar_t *anna_module_name_extract(int argc, char **argv)
{
    if(argc < 2)
    {
	debug(D_CRITICAL,L"Error: Expected at least one argument, a name of a file to run.\n");
	exit(ANNA_STATUS_ARGUMENT_ERROR);
    }

    wchar_t *module_name = str2wcs(argv[1]);
    size_t mlen = wcslen(module_name);
    if(mlen > 5 && wcscmp(&module_name[mlen-5], L".anna") == 0)
    {
	module_name[mlen-5] = 0;
    }
    
    return module_name;
}


int main(int argc, char **argv)
{
    wsetlocale(LC_ALL, L"");

    anna_argc= argc;
    anna_argv = argv;
    
    wchar_t *module_name = anna_module_name_extract(argc, argv);
    
    anna_set_program_name(argv[1]);
    
    debug(D_SPAM,L"Initializing interpreter...\n");    
    anna_alloc_gc_block();
    anna_init();
    
    if(anna_error_count)
    {
	debug(
	    D_CRITICAL,
	    L"Found %d error(s) during initialization, exiting\n", 
	    anna_error_count);
	exit(1);
    }
    
    anna_stack_template_t *module = anna_stack_unwrap(anna_module_load(module_name));
    free(module_name);

    anna_alloc_gc_unblock();
    anna_main_run(module);
    anna_shutdown();
    
    return 0;
}
