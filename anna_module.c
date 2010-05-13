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
#include "anna_util.h"
#include "anna_function.h"
#include "anna_prepare.h"

static hash_table_t *anna_module_imported=0;

anna_function_t *anna_module_load(wchar_t *module_name)
{
    if(anna_module_imported == 0)
    {
	anna_module_imported = malloc(sizeof(hash_table_t));
	hash_init(anna_module_imported, &hash_wcs_func, &hash_wcs_cmp);
    }
    anna_function_t *module = (anna_function_t *)hash_get(
	anna_module_imported,
	module_name);

    if(module)
	return module;
    
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
    
    wprintf(L"Parsed module AST:\n");    
    anna_node_print(program);
        
    anna_function_t *fake_function = anna_function_create(
	anna_util_identifier_generate(L"moduleFunction", &(program->location)),
	0,
	node_cast_call(program),
	null_type, 
	0,
	0,
	0,
	stack_global, 
	0);
    
    
    anna_node_dummy_t *program_dummy = (anna_node_dummy_t *)
	anna_node_prepare(
	    program,
	    fake_function,
	    0);
    
    assert(program_dummy->node_type == ANNA_NODE_TRAMPOLINE);
    module = anna_function_unwrap(
	program_dummy->payload);
    module->name = module_name;
    
    assert(module);
    
    anna_object_t *module_object = anna_stack_wrap(module->stack_template);
    anna_stack_declare(stack_global, module_name, module_object->type, module_object);
    
    anna_prepare();
    
    if(anna_error_count)
    {
	wprintf(L"Found %d error(s) during module validation, exiting\n", anna_error_count);
	exit(1);
    }
    
    //sb_destroy(&sb);
    hash_put(anna_module_imported, module_name, module);
    return module;
    
}
