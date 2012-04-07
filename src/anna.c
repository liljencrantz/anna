#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <locale.h>
#include <sys/prctl.h>
#include <time.h>
#include <getopt.h>

#include "anna/common.h"
#include "anna/util.h"
#include "anna/function.h"
#include "anna/base.h"
#include "anna/module.h"
#include "anna/lib/lang/int.h"
#include "anna/function_type.h"
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
   Number of arguments given to the program (excluding those given to the interpreter)
*/
int anna_argc;

/**
   All arguments given to the program (excluding those given to the interpreter)
*/
char **anna_argv;

/**
   Name of the file to run
*/
static wchar_t *anna_module_name;

static char *anna_program_name = 0;

void anna_type_mark_info(anna_type_t *type);

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
    anna_gc_init();
    
    stack_global = anna_stack_create(0);
    anna_stack_name(stack_global, L"global");
    stack_global->flags |= ANNA_STACK_NAMESPACE;
    
    anna_abides_init();
    null_object = anna_object_create_raw(anna_align(sizeof(anna_object_t)));
    anna_module_init();
    anna_type_close(stack_global->wrapper->type);
//    anna_type_mark_info(stack_global->wrapper->type);
    anna_stack_document(
	stack_global,
	L"The global module is the root of the entire anna namespace.");
    anna_stack_document(
	stack_global,
	L"All top level modules are descendants of the global module.");
}

/**
   Set the name of the application
*/
static void anna_set_program_name(char *arg)
{    
    if(!arg)
	return;
    
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

static void anna_print_help()
{
    anna_message(
	L"Usage: anna [option] file [arg..]\n"
	L"Run the interpreter for the Anna programming language\n\n"
	L"  -h, --help     Print this help message and exit\n"
	L"  -v, --verbose  Increase verbosity level\n"
	L"  -V, --version  Print version number and exit\n"
        L"  file           File name of the program (optionally omitting the .anna suffix)\n"
	L"  arg ...        Arguments passed to program\n");
}

static void anna_print_version()
{
    anna_message(L"0.0.0 (unreleased)\n");
}

/**
   Parse command line options. 
 */
static void anna_opt_parse(int argc, char **argv)
{
    int c;
    
    while(1) 
    {
	int option_index = 0;
	static struct option long_options[] = 
	    {
		{"help",    0, 0, 'h'},
		{"verbose", 0, 0, 'v'},
		{"version", 0, 0, 'V'},
		{0,         0, 0, 0  }
	    };
	
	c = getopt_long(
	    argc, argv, "+hvVa",
	    long_options, &option_index);
	if(c == -1)
	{
	    break;
	}

	switch(c) 
	{
	    case 'h':
	    {
		anna_print_help();
		exit(0);
		break;
	    }
	    
	    case 'v':
	    {
		debug_level--;
		break;
	    }
	    
	    case 'V':
	    {
		anna_print_version();
		exit(0);
		break;
	    }
	    
	    case '?':
	    {
		anna_print_help();
		exit(ANNA_STATUS_ARGUMENT_ERROR);
		break;
	    }
	}
    }
    
    if(optind == argc) 
    {
	anna_module_name = wcsdup(ANNA_LIB_DIR L"/repl");
	/*
	string_buffer_t sb;
	sb_init(&sb);
	ab_printf(L"%ls/%ls", ANNA_LIB_
	debug(999, L"AAA %ls\n", ANNA_LIB_DIR);
	*/
//	debug(D_CRITICAL, L"No program to run.\n");
//	exit(ANNA_STATUS_ARGUMENT_ERROR);
    }
    else
    {
	anna_module_name = str2wcs(argv[optind]);
	anna_program_name = argv[optind];
	size_t mlen = wcslen(anna_module_name);
	if(mlen > 5 && wcscmp(&anna_module_name[mlen-5], L".anna") == 0)
	{
	    anna_module_name[mlen-5] = 0;
	}
    }
    
    anna_argc = argc-optind;
    anna_argv = &argv[optind];
    
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
    hash_foreach(&anna_type_get_function_identifier, fun_key_free);
    hash_destroy(&anna_type_get_function_identifier);
#endif
}

/**
   Run the function named main in the specified module.
*/
static void anna_main_run(anna_stack_template_t *module)
{
    debug(D_INFO,L"Finished parsing program, staring main eval loop.\n");    
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
   Main program entry point.

   Save the argument list, initialize the interpreter, parse the
   specified module and run its main function.
*/

int main(int argc, char **argv)
{
    wsetlocale(LC_ALL, L"");
    tzset();
    
    anna_opt_parse(argc, argv);

    anna_set_program_name(anna_program_name);
    
    debug(D_INFO,L"Initializing interpreter...\n");    
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
    
    anna_stack_template_t *module = anna_stack_unwrap(anna_module_load(anna_module_name));
    free(anna_module_name);
    
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
    
    debug(D_INFO,L"Program ended. Exiting.\n");    
    return ANNA_STATUS_OK;
}

