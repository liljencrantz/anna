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
#include "anna_stack.h"

#include "anna_function_type.h"
#include "anna_type_type.h"
#include "anna_object_type.h"
#include "anna_type.h"
#include "anna_macro.h"
#include "anna_member.h"
#include "anna_node_wrapper.h"
#include "anna_status.h"
#include "anna_vm.h"
#include "anna_alloc.h"
#include "anna_intern.h"
#include "anna_lang.h"
#include "wutil.h"

static void anna_module_load_i(anna_stack_template_t *module);

static anna_stack_template_t *anna_module(
    anna_stack_template_t *parent, wchar_t *name, wchar_t *filename)
{
    
    anna_object_t *obj;
    anna_stack_template_t *res;
    if(name)
    {
	obj = anna_stack_get(parent, name);

	if(obj)
	{
	    res = anna_stack_unwrap(obj);
	    if(!res)
	    {
		debug(D_CRITICAL, L"%ls is not a namespace\n", name);
		CRASH;
	    }
	    if(filename)
	    {
		if(res->filename)
		{
		    debug(D_CRITICAL, L"Multiple definitions for module %ls\n", name);
		    CRASH;		
	    }
		res->filename = wcsdup(filename);
	    }
	    return res;
	}
    }    
    
    res = anna_stack_create(parent);
    obj = anna_stack_wrap(res);
    if(name)
    {
	anna_stack_declare(
	    parent, name, obj->type, obj, ANNA_STACK_READONLY);
    }
    if(filename)
    {
	res->filename = wcsdup(filename);
    }
    return res;
}

static void anna_module_init_recursive(
    wchar_t *dname, anna_stack_template_t *parent)
{
    DIR *dir = wopendir(dname);
    struct wdirent *ent;
    string_buffer_t fn;
    sb_init(&fn);
    sb_printf(&fn, L"%ls/", dname);
    size_t len = sb_length(&fn);
    while((ent=wreaddir(dir)))
    {
	sb_truncate(&fn, len);
	sb_append(&fn, ent->d_name);
	struct stat statbuf;
	
	wchar_t *d_name = wcsdup(ent->d_name);

	if(ent->d_name[0] == L'.')
	{
	    goto CLEANUP;
	}	
	
	if(wstat(sb_content(&fn), &statbuf))
	{
	    debug(D_ERROR, L"Failed to stat file %ls\n", sb_content(&fn));
	    goto CLEANUP;
	}
	
	if(S_ISDIR(statbuf.st_mode))
	{
	    anna_module_init_recursive(sb_content(&fn), anna_module(parent, d_name, 0));
	    goto CLEANUP;
	}
	
	wchar_t *suffix = d_name + wcslen(d_name) - 5;
	if(suffix <= d_name)
	{
	    goto CLEANUP;
	}
	
	if(wcscmp(suffix, L".anna") == 0)
	{
	    *suffix=0;
	    anna_module_load_i(
		anna_module(parent, d_name,sb_content(&fn)));
	}
      CLEANUP:
	free(d_name);
    }
    sb_destroy(&fn);
}

void anna_module_init()
{
    anna_stack_template_t *stack_lang = anna_lang_load();

    anna_stack_declare(
	stack_global,
	L"lang",
	anna_stack_wrap(stack_lang)->type,
	anna_stack_wrap(stack_lang),
	ANNA_STACK_READONLY);
    
    anna_module_init_recursive(L"lib", stack_global);
    anna_stack_populate_wrapper(stack_lang);
}
	
static void anna_module_find_import_internal(
    anna_node_t *module, wchar_t *name, array_list_t *import)
{
    int i, j;
    if(module->node_type != ANNA_NODE_CALL)
    {
	anna_error(module, L"Not a valid module");
	return;
    }
    anna_node_call_t *m = (anna_node_call_t *)module;
    for(i=0; i<m->child_count; i++)
    {
	if(anna_node_is_call_to(m->child[i], name))
	{
	    anna_node_call_t *im = (anna_node_call_t *)m->child[i];
	    for(j=0; j<im->child_count; j++)
	    {
		if(im->child[j]->node_type == ANNA_NODE_IDENTIFIER)
		{
		    anna_node_identifier_t *id = (anna_node_identifier_t *)im->child[j];
		    al_push(import, id->name);
		}
		else
		{
		    anna_error(im->child[j], L"Invalid module. Must be an identifier");
		}
	    }
	    m->child[i] = anna_node_create_null(
		&module->location);	    
	}
    }
}
static void anna_module_find_import(anna_node_t *module, array_list_t *import)
{
    anna_module_find_import_internal(module, L"import", import);
}

static void anna_module_find_expand(anna_node_t *module, array_list_t *import)
{
    anna_module_find_import_internal(module, L"expand", import);
}

static void anna_module_compile(anna_node_t *this, void *aux)
{
    if(this->node_type == ANNA_NODE_CLOSURE)
    {
	anna_node_closure_t *this2 = (anna_node_closure_t *)this;	
	if(this2->payload->body && !this2->payload->code)
	{
	    anna_node_each((anna_node_t *)this2->payload->body, &anna_module_compile, 0);
	    anna_vm_compile(this2->payload);
	}
    }
    if(this->node_type == ANNA_NODE_TYPE)
    {
	anna_node_type_t *this2 = (anna_node_type_t *)this;	
	if(this2->payload->body && !(this2->payload->flags & ANNA_TYPE_COMPILED))
	{
	    this2->payload->flags |= ANNA_TYPE_COMPILED;
	    anna_node_each((anna_node_t *)this2->payload->body, &anna_module_compile, 0);
	}
    }
}

static void anna_module_load_i(anna_stack_template_t *module_stack)
{
//    debug_level=0;
    int i;
    array_list_t import = AL_STATIC;
    array_list_t expand = AL_STATIC;

    if(module_stack->flags & ANNA_STACK_LOADED)
    {
	return;
    }
    module_stack->flags |= ANNA_STACK_LOADED;
    
    debug(D_SPAM,L"Parsing file %ls...\n", module_stack->filename);    
    anna_node_t *program = anna_parse(module_stack->filename);
    
    if(!program || anna_error_count) 
    {
	debug(D_CRITICAL,L"Module %ls failed to parse correctly; exiting.\n", module_stack->filename);
	exit(ANNA_STATUS_PARSE_ERROR);
    }
    
    debug(D_SPAM,L"Parsed AST for module %ls:\n", module_stack->filename);    
    anna_node_print(D_SPAM, program);    

    /*
      Implicitly add an import
      of the lang module to the top of the ast.
    */
    al_push(&import, L"lang");
    
    anna_stack_template_t *macro_stack = anna_stack_create(stack_global);
    macro_stack->flags |= ANNA_STACK_NAMESPACE;
    anna_module_find_expand(program, &expand);    
    anna_module_find_import(program, &import);
    
    for(i=0; i<al_get_count(&expand); i++ )
    {
	wchar_t *str = al_get(&expand, i);
	anna_stack_template_t *mod = anna_module(stack_global, str, 0);
	if(anna_error_count || !mod)
	{
	    return;
	}
	al_push(&macro_stack->expand, mod);
    }
    
    /*
      Prepare the module. 
    */
    anna_node_t *node = (anna_node_t *)
	anna_node_macro_expand(
	    program,
	    macro_stack);
    
    if(anna_error_count)
    {
	debug(D_CRITICAL,L"Found %d error(s) during macro expansion phase\n", anna_error_count);
	exit(ANNA_STATUS_MACRO_ERROR);
    }
    debug(D_SPAM,L"Macros expanded in module %ls\n", module_stack->filename);    
    
    al_destroy(&expand);
    
    anna_node_print(0, node);
    anna_node_register_declarations(module_stack, node);
    module_stack->flags |= ANNA_STACK_NAMESPACE;
    if(anna_error_count)
    {
	debug(
	    4,
	    L"Critical: Found %d error(s) during loading of module %ls\n", 
	    anna_error_count, module_stack->filename);
	exit(ANNA_STATUS_INTERFACE_ERROR);
    }
    debug(
	D_SPAM,
	L"Declarations registered in module %ls\n", 
	module_stack->filename);
    
    for(i=0; i<al_get_count(&import); i++ )
    {
	wchar_t *str = al_get(&import, i);
	anna_stack_template_t *mod = anna_module(stack_global, str, 0);
	if(anna_error_count || !mod)
	{
	    return;
	}
	al_set(&import, i, mod);
    }
    al_push(&import, module_stack);
    memcpy(&module_stack->import, &import, sizeof(array_list_t));
    
    anna_node_call_t *ggg = node_cast_call(node);
    
    anna_node_calculate_type_children(ggg, module_stack);
    if(anna_error_count)
    {
	debug(
	    4,
	    L"Found %d error(s) during module loading\n",
	    anna_error_count);
	exit(ANNA_STATUS_TYPE_CALCULATION_ERROR);
    }
    debug(D_SPAM,L"Return types set up for module %ls\n", module_stack->filename);	

    for(i=0; i<ggg->child_count; i++)
    {
	anna_node_each(ggg->child[i], (anna_node_function_t)&anna_node_validate, module_stack);
    }
    if(anna_error_count)
    {
	debug(
	    D_CRITICAL,
	    L"Found %d error(s) during module loading\n",
		anna_error_count);
	exit(
	    ANNA_STATUS_VALIDATION_ERROR);
    }
    
//	anna_node_each((anna_node_t *)ggg, &anna_module_prepare_body, module_stack);
    
    debug(D_SPAM,L"AST validated for module %ls\n", module_stack->filename);	
	
/*
	anna_node_find(node, ANNA_NODE_CLOSURE, &al);	
	for(i=0; i<al_get_count(&al); i++)
	{
	    anna_function_t *f = ((anna_node_closure_t *)al_get(&al, i))->payload;
	    anna_function_setup_type(f, module_stack);
	}
	debug(D_SPAM,L"%d function types set up\n", al_get_count(&al));	
*/
	
    for(i=0; i<ggg->child_count; i++)
    {
	anna_node_static_invoke(ggg->child[i], module_stack);
	if(anna_error_count)
	{
	    debug(
		D_CRITICAL,
		L"Found %d error(s) during module loading\n",
		anna_error_count);
	    exit(
		ANNA_STATUS_MODULE_SETUP_ERROR);
	}
    }
    
    anna_stack_populate_wrapper(module_stack);
    
    debug(D_SPAM,L"Module stack object set up for %ls\n", module_stack->filename);	
    
    anna_node_each((anna_node_t *)ggg, &anna_module_compile, 0);
    
    debug(D_SPAM,L"Module %ls is compiled\n", module_stack->filename);	
}

anna_object_t *anna_module_load(wchar_t *module_name)
{
    string_buffer_t fn;
    sb_init(&fn);
    sb_printf(&fn, L"%ls.anna", module_name);
    
    anna_alloc_gc_block();

    anna_stack_template_t *module = anna_module(
	stack_global, 0, sb_content(&fn));
    anna_module_load_i(module);
    
    sb_destroy(&fn);
    anna_alloc_gc_unblock();

    return anna_stack_wrap(module);
}
