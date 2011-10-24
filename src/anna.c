#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <locale.h>
#include <sys/prctl.h>
#include <time.h>

#include "anna/common.h"
#include "anna/util.h"
#include "anna/function.h"
#include "anna/base.h"
#include "anna/module.h"
#include "anna/lib/lang/int.h"
#include "anna/lib/function_type.h"
#include "anna/type.h"
#include "anna/member.h"
#include "anna/status.h"
#include "anna/vm.h"
#include "anna/tt.h"
#include "anna/alloc.h"
#include "anna/slab.h"
#include "anna/mid.h"

#define PRCTL_MAX_LENGTH 15

/**
   The root of the namespace hierarchy
*/
anna_stack_template_t *stack_global;

/**
   Number of arguments given to the program
*/
int anna_argc;

/**
   All arguments given to the program
*/
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
    anna_stack_name(stack_global, L"global");
    stack_global->flags |= ANNA_STACK_NAMESPACE;

    anna_abides_init();
    null_object = anna_object_create_raw(anna_align(sizeof(anna_object_t)));
    anna_module_init();
    anna_type_close(stack_global->wrapper->type);
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
    
    if(strlen(name) > PRCTL_MAX_LENGTH)
    {
	name = strndup(name, PRCTL_MAX_LENGTH);
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
    anna_object_t *main_wrapper = anna_as_obj(anna_stack_get(module, L"main"));

    if(!main_wrapper)
    {
	debug(D_CRITICAL,L"No main method defined.\n");
	exit(ANNA_STATUS_MODULE_SETUP_ERROR);
    }
    anna_function_t *main_fun = anna_function_unwrap(main_wrapper);
    if(!main_fun)
    {
	debug(D_CRITICAL,L"«main» is not a function.\n");
	exit(ANNA_STATUS_MODULE_SETUP_ERROR);	
    }    
    if(main_fun->input_count)
    {
	debug(D_CRITICAL,L"«main» function must take zero arguments but takes %d.\n", main_fun->input_count);
	exit(ANNA_STATUS_MODULE_SETUP_ERROR);
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

/**
   Main program entry point.

   Save the argument list, initialize the interpreter, parse the
   specified module and run its main function.
*/

int main(int argc, char **argv)
{
    wsetlocale(LC_ALL, L"");
    tzset();
    
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
	exit(ANNA_STATUS_INTERNAL_ERROR);
    }
    
    anna_stack_template_t *module = anna_stack_unwrap(anna_module_load(module_name));
    free(module_name);
    
    if(anna_error_count)
    {
	debug(
	    D_CRITICAL,
	    L"Found %d error(s) during module loading, exiting\n", 
	    anna_error_count);
	exit(ANNA_STATUS_MODULE_SETUP_ERROR);
    }
    
    anna_alloc_gc_unblock();
    anna_main_run(module);

    anna_shutdown();
    
    return ANNA_STATUS_OK;
}
