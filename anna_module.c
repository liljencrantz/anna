#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna.h"
#include "anna_module.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_util.h"
#include "anna_function.h"
#include "anna_prepare.h"
#include "anna_stack.h"

static hash_table_t *anna_module_imported=0;
static array_list_t anna_module_unprepared = {0,0,0};
/*
static array_list_t anna_function_unprepared = {0,0,0};

void anna_function_prepare_enque()
{
    
}
*/

static void anna_module_calculate_type(anna_node_t *n, void *aux)
{
    anna_node_calculate_type(n, (anna_stack_frame_t *)aux);
}

anna_stack_frame_t *anna_module_load(wchar_t *module_name)
{
    static int recursion_level=0;
    
    if(anna_module_imported == 0)
    {
	anna_module_imported = malloc(sizeof(hash_table_t));
	hash_init(anna_module_imported, &hash_wcs_func, &hash_wcs_cmp);
    }
    anna_stack_frame_t *module = (anna_stack_frame_t *)hash_get(
	anna_module_imported,
	module_name);
    
    if(module)
	return module;

    recursion_level++;
        
    string_buffer_t sb;
    sb_init(&sb);
    sb_append(&sb, module_name);
    sb_append(&sb, L".anna");
    wchar_t *filename = sb_content(&sb);
    
    wprintf(L"Parsing file %ls...\n", filename);    
    anna_node_t *program = anna_parse(filename);
    
    if(!program || anna_error_count) 
    {
	wprintf(L"Module failed to parse correctly; exiting.\n");
	exit(1);
    }

    /*
      Unless we're loading the lang module, implicitly add an import
      of the lang module to the top of the ast.
     */
    if(wcscmp(module_name, L"lang") != 0)
    {
/*
	anna_node_call_t *imp = anna_node_create_call(
	    0,
	    (anna_node_t *)anna_node_create_identifier(0, L"import"),
	    0,
	    0);
	anna_node_call_add_child(
	    imp,
	    (anna_node_t *)anna_node_create_identifier(0, L"lang"));

	anna_node_call_t *definition = (anna_node_call_t *)program;
	anna_node_call_prepend_child(
	    definition,
	    (anna_node_t *)imp);
*/	
    }

    wprintf(L"Parsed module AST:\n");    
    anna_node_print(program);
    
    /*
      Prepare the module. 
    */
    anna_node_t *node = (anna_node_t *)
	anna_node_macro_expand(
	    program,
	    stack_global);
    if(anna_error_count)
    {
	wprintf(L"Found %d error(s) during module loading\n", anna_error_count);
	exit(1);
    }
    wprintf(L"Macros expanded\n");    
    
    
    anna_node_print(node);
        
    anna_stack_frame_t *module_stack = anna_node_register_declarations(node, 0);
    module_stack->parent = stack_global;
    if(anna_error_count)
    {
	wprintf(L"Found %d error(s) during module loading\n", anna_error_count);
	exit(1);
    }
    wprintf(L"Declarations registered\n");
    
    hash_put(
	anna_module_imported,
	module_name,
	module_stack);
    
    anna_object_t *module_object = anna_stack_wrap(module_stack);
    anna_stack_declare(stack_global, module_name, module_object->type, module_object, 0);
/*
    al_push(&anna_module_unprepared, module_stack);
    al_push(&anna_module_unprepared, );
*/    
    
    if(recursion_level == 1)
    {
	int i;
	array_list_t al = AL_STATIC;
	anna_node_call_t *ggg = node;
	
	for(i=0; i<ggg->child_count; i++)
	{
	    anna_node_each(ggg->child[i], &anna_module_calculate_type, module_stack);
	    if(anna_error_count)
	    {
		wprintf(L"Found %d error(s) during module loading\n", anna_error_count);
		exit(1);
	    }
	}

	anna_node_each(ggg, &anna_node_prepare_body, module_stack);
	

	wprintf(L"Node return types set up\n");	
/*
	anna_node_find(node, ANNA_NODE_CLOSURE, &al);	
	for(i=0; i<al_get_count(&al); i++)
	{
	    anna_function_t *f = ((anna_node_closure_t *)al_get(&al, i))->payload;
	    anna_function_setup_type(f, module_stack);
	}
	wprintf(L"%d function types set up\n", al_get_count(&al));	
*/

	
	for(i=0; i<ggg->child_count; i++)
	{
	    anna_node_invoke(ggg->child[i], module_stack);
	    if(anna_error_count)
	    {
		wprintf(L"Found %d error(s) during module loading\n", anna_error_count);
		exit(1);
	    }
	}
	
	wprintf(L"Declarations assigned\n");    
	
	anna_node_print(program);
    }
        
    recursion_level--;
    return module_stack;
}
