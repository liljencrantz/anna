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

#include "anna_int.h"
#include "anna_float.h"
#include "anna_string.h"
#include "anna_char.h"
#include "anna_list.h"
#include "anna_function_type.h"
#include "anna_type_type.h"
#include "anna_type.h"
#include "anna_macro.h"
#include "anna_member.h"
#include "anna_node_wrapper.h"




static hash_table_t *anna_module_imported=0;
//static array_list_t anna_module_unprepared = {0,0,0};

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

static void anna_module_find_imports_internal(anna_node_t *module, wchar_t *name, array_list_t *import)
{
    int i;
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
	    int j;
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
static void anna_module_find_imports(anna_node_t *module, array_list_t *import)
{
    anna_module_find_imports_internal(module, L"import", import);
}

static void anna_module_find_import_macros(anna_node_t *module, array_list_t *import)
{
    anna_module_find_imports_internal(module, L"importMacro", import);
}

static void anna_module_prepare_body(
    anna_node_t *this, void *unused)
{
    anna_node_prepare_body(this);
}

static int hash_null_func( void *data )
{
    return 0;
}

static int hash_null_cmp( void *a, 
		   void *b )
{
    return 1;
}


static void anna_null_type_create()
{
    int i;
  
    wchar_t *member_name = L"!null_member";
    anna_member_t *null_member;  
    null_member = malloc(sizeof(anna_member_t)+(sizeof(wchar_t*)*(1+wcslen(member_name))));
    //wprintf(L"Null member is %d\n", null_member);

    null_member->type = null_type;
    null_member->offset=0;
    null_member->is_static=1;
    wcscpy(null_member->name, member_name);

    /*  
	anna_native_method_create(list_type, -1, L"__getInt__", 0, (anna_native_t)&anna_list_getitem, object_type, 2, i_argv, i_argn);
    */
    anna_type_t *argv[]={null_type};
    wchar_t *argn[]={L"this"};
    anna_type_static_member_allocate(null_type);

    null_type->static_member[0] = 
	anna_function_wrap(
	    anna_native_create(
		L"!nullFunction", 0, 
		(anna_native_t)&anna_i_null_function, 
		null_type, 1, argv, argn,
		0));
  
    anna_object_t *null_function;  
    null_function = null_type->static_member[0];
    hash_init(&null_type->name_identifier, &hash_null_func, &hash_null_cmp);
    hash_put(&null_type->name_identifier, L"!null_member", null_member);
  
    for(i=0; i<64;i++) {
	null_type->mid_identifier[i] = null_member;
    }
    assert(*anna_static_member_addr_get_mid(null_type, 5) == null_function);    
}

static void anna_module_load_lang()
{

    anna_stack_frame_t *stack_lang = anna_stack_create(4096, stack_global);
    
    /*
      Create lowest level stuff. Bits of magic, be careful with
      ordering here. A lot of intricate dependencies going on between
      the various calls...
    */
    
    type_type = 
	anna_type_native_create(
	    L"Type", 
	    stack_lang);
    object_type = anna_type_native_create(L"Object" ,stack_lang);
    null_type = anna_type_native_create(L"Null", stack_lang);
    int_type = 
	anna_type_native_create(
	    L"Int", 
	    stack_lang);

    list_type = 
	anna_type_native_create(
	    L"List", 
	    stack_lang);

    string_type = 
	anna_type_native_create(
	    L"String", 
	    stack_lang);
    
    float_type = anna_type_native_create(L"Float", stack_lang);
    char_type = anna_type_native_create(L"Char", stack_lang);
    member_type = 
	anna_type_native_create(
	    L"Member",
	    stack_lang);
    
    
    anna_type_type_create(stack_lang);    
    anna_list_type_create(stack_lang);
    anna_type_type_create2(stack_lang);    
    anna_null_type_create();    
    anna_int_type_create(stack_lang);
    anna_string_type_create(stack_lang);
    anna_node_create_wrapper_types(stack_lang);
    anna_member_types_create(stack_lang);
    
    anna_stack_declare(stack_lang, L"Type", type_type, anna_type_wrap(type_type), 0); 
    anna_stack_declare(stack_lang, L"Int", type_type, anna_type_wrap(int_type), 0);       anna_stack_declare(stack_lang, L"Object", type_type, anna_type_wrap(object_type), 0); 
    anna_stack_declare(stack_lang, L"Null", type_type, anna_type_wrap(null_type), 0); 

    anna_stack_declare(stack_lang, L"List", type_type, anna_type_wrap(list_type), 0); 
    anna_stack_declare(stack_lang, L"String", type_type, anna_type_wrap(string_type), 0); 
    anna_stack_declare(stack_lang, L"Float", type_type, anna_type_wrap(float_type), 0); 
    anna_stack_declare(stack_lang, L"Char", type_type, anna_type_wrap(char_type), 0);

    anna_char_type_create(stack_lang);
    anna_float_type_create(stack_lang);

    anna_function_implementation_init(stack_lang);
    anna_macro_init(stack_global);

    hash_put(
	anna_module_imported,
	L"lang",
	stack_lang);
    anna_stack_declare(
	stack_global,
	L"lang",
	anna_stack_wrap(stack_lang)->type,
	anna_stack_wrap(stack_lang),
	0
	);
    
}


anna_object_t *anna_module_load(wchar_t *module_name)
{
    static int recursion_level=0;
    int i;
    array_list_t import = AL_STATIC;
    array_list_t mimport = AL_STATIC;
        
    if(anna_module_imported == 0)
    {
	anna_module_imported = malloc(sizeof(hash_table_t));
	hash_init(anna_module_imported, &hash_wcs_func, &hash_wcs_cmp);
    }
    anna_stack_frame_t *module = (anna_stack_frame_t *)hash_get(
	anna_module_imported,
	module_name);
    
    if(module)
	return anna_stack_wrap(module);

    anna_stack_frame_t *module_stack;

    if(wcscmp(module_name, L"lang") == 0)
    {
	anna_module_load_lang();
	return anna_module_load(L"lang");	
    }
	
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
	wprintf(L"Module %ls failed to parse correctly; exiting.\n", module_name);
	exit(1);
    }

    wprintf(L"Parsed AST for module %ls:\n", module_name);    
//    anna_node_print(program);    

    /*
      Implicitly add an import
      of the lang module to the top of the ast.
    */
    al_push(&import, L"lang");
    
    anna_module_find_import_macros(program, &mimport);    
    anna_module_find_imports(program, &import);    
    

    for(i=0; i<al_get_count(&mimport); i++ )
    {
	wchar_t *str = al_get(&mimport, i);
	anna_object_t *mod = anna_module_load(str);
	if(anna_error_count || !mod)
	{
	    return 0;
	}
	al_set(&mimport, i, anna_stack_unwrap(mod));
    }

    anna_stack_frame_t *stack_macro = anna_stack_clone(stack_global);
    memcpy(&stack_macro->import, &mimport, sizeof(array_list_t));

    /*
      Prepare the module. 
    */
    anna_node_t *node = (anna_node_t *)
	anna_node_macro_expand(
	    program,
	    stack_macro);
    if(anna_error_count)
    {
	wprintf(L"Found %d error(s) during module loading\n", anna_error_count);
	exit(1);
    }
    wprintf(L"Macros expanded in module %ls\n", module_name);    
        
    //  anna_node_print(node);
        
    module_stack = anna_node_register_declarations(node, 0);
    module_stack->parent = stack_global;
    if(anna_error_count)
    {
	wprintf(
	    L"Found %d error(s) during loading of module %ls\n", 
	    anna_error_count, module_name);
	exit(1);
    }
    wprintf(
	L"Declarations registered in module %ls\n", 
	module_name);
    
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
    
    for(i=0; i<al_get_count(&import); i++ )
    {
	wchar_t *str = al_get(&import, i);
	anna_object_t *mod = anna_module_load(str);
	if(anna_error_count || !mod)
	{
	    return 0;
	}
	al_set(&import, i, anna_stack_unwrap(mod));
    }
    al_push(&import, module_stack);
    memcpy(&module_stack->import, &import, sizeof(array_list_t));
    
//    if(recursion_level == 1)
//    {
	anna_node_call_t *ggg = node_cast_call(node);
	
	for(i=0; i<ggg->child_count; i++)
	{
	    anna_node_each(ggg->child[i], &anna_module_calculate_type, module_stack);
	    if(anna_error_count)
	    {
		wprintf(
		    L"Found %d error(s) during module loading\n",
		    anna_error_count);
		exit(1);
	    }
	}
	
	anna_node_each((anna_node_t *)ggg, &anna_module_prepare_body, module_stack);	
	
	wprintf(L"Return types set up for module %ls\n", module_name);	
	
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
		wprintf(
		    L"Found %d error(s) during module loading\n",
		    anna_error_count);
		exit(1);
	    }
	}
	
	anna_stack_populate_wrapper(module_stack);

//	wprintf(L"Declarations assigned\n");
//	anna_node_print(program);
	//  }
    
    recursion_level--;
    return anna_stack_wrap(module_stack);
}

