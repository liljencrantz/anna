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
#include "anna_module_data.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_util.h"
#include "anna_function.h"
#include "anna_stack.h"

#include "clib/anna_function_type.h"
#include "clib/lang/object.h"
#include "anna_type.h"
#include "anna_macro.h"
#include "anna_member.h"
#include "clib/parser.h"
#include "anna_status.h"
#include "anna_vm.h"
#include "anna_alloc.h"
#include "anna_intern.h"
#include "wutil.h"
#include "anna_attribute.h"
#include "clib/lang/string.h"
#include "anna_mid.h"
#include "anna_use.h"

#include "clib/clib.h"
#include "clib/lang/list.h"
#include "clib/lang/hash.h"

static void anna_module_load_i(anna_stack_template_t *module);
array_list_t anna_module_default_macros = AL_STATIC;

static wchar_t *anna_module_search(
    anna_stack_template_t *parent, wchar_t *name)
{
    /*
      FIXME: Hardcoded uglyness...
     */
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"lib/%ls.anna", name);
    return sb_content(&sb);
}


static anna_stack_template_t *anna_module(
    anna_stack_template_t *parent, wchar_t *name, wchar_t *filename)
{
    
    anna_object_t *obj;
    anna_stack_template_t *res;
    if(name)
    {
	obj = anna_as_obj(anna_stack_get(parent, name));
	
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
		if(res->filename && wcscmp(res->filename, filename) != 0)
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
    anna_stack_name(res, name);
    obj = anna_stack_wrap(res);
    if(name)
    {
	anna_stack_declare(
	    parent, name, obj->type, anna_from_obj(obj), ANNA_STACK_READONLY);
    }
    if(filename)
    {
	res->filename = wcsdup(filename);
    }
    else
    {
	res->filename = anna_module_search(parent, name);
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
    closedir(dir);
    sb_destroy(&fn);
}

static void anna_module_bootstrap_macro(wchar_t *name)
{
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"bootstrap/%ls.anna", name);
    wchar_t *path = sb_content(&sb);

    anna_stack_template_t *mm = anna_module(stack_global, name, path);
    sb_destroy(&sb);

    anna_module_load_i(mm);
    al_push(&anna_module_default_macros, mm);
}

void anna_module_const_int(
    anna_stack_template_t *stack,
    wchar_t *name,
    int value,
    wchar_t *documentation
    )
{
    anna_stack_declare(
	stack,
	name,
	int_type,
	anna_from_int(value),
	ANNA_STACK_READONLY);
    if(documentation)
    {
	anna_type_t *type = 
	    anna_stack_wrap(stack)->type;
	anna_member_document(
	    type,
	    anna_mid_get(name),
	    documentation);
    }
}


void anna_module_const_char(
    anna_stack_template_t *stack,
    wchar_t *name,
    wchar_t value,
    wchar_t *documentation
    )
{
    anna_stack_declare(
	stack,
	name,
	char_type,
	anna_from_char(value),
	ANNA_STACK_READONLY);
    if(documentation)
    {
	anna_type_t *type = 
	    anna_stack_wrap(stack)->type;
	anna_member_document(
	    type,
	    anna_mid_get(name),
	    documentation);
    }
}

void anna_module_const_float(
    anna_stack_template_t *stack,
    wchar_t *name,
    double value,
    wchar_t *documentation
    )
{
    anna_stack_declare(
	stack,
	name,
	float_type,
	anna_from_float(value),
	ANNA_STACK_READONLY);
    if(documentation)
    {
	anna_type_t *type = 
	    anna_stack_wrap(stack)->type;
	anna_member_document(
	    type,
	    anna_mid_get(name),
	    documentation);
    }
}

static void anna_module_bootstrap_monkeypatch(
    anna_stack_template_t *lang, wchar_t *name)
{
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"bootstrap/%ls.anna", name);
    wchar_t *path = sb_content(&sb);    
    anna_stack_template_t *int_mod = anna_module(stack_global, name, path);
    sb_destroy(&sb);

    anna_module_load_i(int_mod);
    
    int i;
    anna_type_t *int_mod_type = anna_stack_wrap(int_mod)->type;
    for(i=1; i<int_mod_type->static_member_count; i++)
    {
	anna_object_t *fun_obj = anna_as_obj(int_mod_type->static_member[i]);
	anna_function_t *fun = anna_function_unwrap(fun_obj);
	
	anna_node_t *target_node = anna_attribute_call(fun->attribute, L"target");
	anna_node_t *name_node = anna_attribute_call(fun->attribute, L"name");
	
	if((!target_node || target_node->node_type != ANNA_NODE_IDENTIFIER) ||
	   (name_node && name_node->node_type != ANNA_NODE_IDENTIFIER))
	{
	    /*
	      Skip monkeypatch elements without valid targets. These
	      are hopefully just internal helper functions.
	     */
	    continue;
	}
	
	anna_node_identifier_t *target_id = (anna_node_identifier_t *)target_node;
	if(name_node)
	{
	    anna_node_identifier_t *name_id = (anna_node_identifier_t *)name_node;	
	    fun->name = name_id->name;
	}
	
	anna_type_t * type = anna_type_unwrap(
	    anna_as_obj(
		anna_stack_get(
		    lang, target_id->name)));
	
	if(type == any_list_type)
	{
	    anna_list_add_method(fun);
	}
	else if(type == hash_type)
	{
	    anna_hash_add_method(fun);
	}
	else if(type == node_type)
	{
	    anna_node_wrapper_add_method(fun);
	}
	else if(type == string_type)
	{
	    anna_member_create_method(string_type, anna_mid_get(fun->name), fun);
	    anna_member_create_method(mutable_string_type, anna_mid_get(fun->name), fun);
	    anna_member_create_method(imutable_string_type, anna_mid_get(fun->name), fun);
	}
	else
	{
	    anna_member_create_method(type, anna_mid_get(fun->name), fun);
	}
    }
}

ANNA_VM_NATIVE(anna_system_get_argument, 1)
{
    static anna_object_t *res = 0;
    if(!res)
    {
	res = anna_list_create_imutable(string_type);
	int i;
	for(i=1; i<anna_argc; i++)
	{
	    wchar_t *data = str2wcs(anna_argv[i]);
	    anna_object_t *arg = anna_string_create(wcslen(data), data);
	    anna_list_add(res, anna_from_obj(arg));	    
	}
    }
    return anna_from_obj(res);
}

static void anna_system_load(anna_stack_template_t *stack)
{
    anna_type_t *type = anna_stack_wrap(stack)->type;
    
    anna_member_create_native_property(
	type, anna_mid_get(L"argument"),
	anna_list_type_get_imutable(string_type),
	&anna_system_get_argument,
	0,
	L"The arguments that where given to the program at launch.");
}

static void anna_module_doc_item(
    anna_type_t *type, 
    wchar_t *name1,
    wchar_t *name2,
    wchar_t *doc)
{
    static string_buffer_t *sb = 0;
    if(!sb)
    {
	sb = malloc(sizeof(string_buffer_t));
	sb_init(sb);
    }
    sb_clear(sb);
    sb_printf(sb, L"%ls%ls", name1, name2);
    
    anna_member_document(
	type,
	anna_mid_get(sb_content(sb)),
	doc);
    
}

static void anna_module_doc()
{
    wchar_t *data_numerical[][2] = {
	{L"__neg__", L"Negate the number."},
	{L"__abs__", L"The absolute value of the number."},
	{L"__sign__", L"Return the sign of the number."},
	{0, 0}
    };

    wchar_t *data_numerical_with_alias[][2] = {
	{L"__add__", L"Add two numbers together."},
	{L"__sub__", L"Subtract two numbers from each other."},
	{L"__mul__", L"Multiply two numbers with each other."},
	{L"__div__", L"Divide one number with another."},
	{L"__exp__", L"Raise one number to the power of another."},
	{L"__increaseAssign__", L"Increase a number by the specified amount."},
	{L"__decreaseAssign__", L"Decrease a number by the specified amount."},
	{L"__nextAssign__", L"Increase a number by one step."},
	{L"__prevAssign__", L"Decrease a number by one step."},
	{L"__shl__", L"Shift the bit pattern representing this number the specified number of bits left."},
	{L"__shr__", L"Shift the bit pattern representing this number the specified number of bits right."},
	{0, 0}
    };

    wchar_t *data_iter[][2] = {
	{L"__map__", L"Exacute the specified function once for each element and return a list containing the output of each function call."},
	{L"__each__", L"Execute the specified function once for each element."},
	{L"__filter__", L"Execute the specified function once for each element and return a new object of the same type, containing the elements for which the function returned non-null."},
	{L"__appendAssign__", L"Append the specified items to this collection."},
	{L"__get__Int__", L"Return the item at the specified offset."},
	{L"__get__Range__", L"Return the items in the specified range."},
	{L"__set__Int__", L"Set the item at the specified offset to the specified value."},
	{L"__set__Range__", L"Set all of the items in the specified Range to the items in the specified list."},
	{L"__find__", L"Exacute the specified function once for each element until the function returns a non-null value, then return the corresponding element. Returns null if no element matched."},
	{L"__join__", L"Join the two collections into one."},
	{0, 0}
    };
    
    anna_type_t *iter_type[] = 
	{
	    string_type, mutable_string_type, imutable_string_type, 
	    any_list_type, mutable_list_type, imutable_list_type, 
	    buffer_type, node_call_type, hash_type, range_type, 0
	};
    
    int i, j;
    
    for(i=0; data_numerical[i][0]; i++)
    {
	anna_member_document(
	    int_type,
	    anna_mid_get(data_numerical[i][0]),
	    data_numerical[i][1]);
	anna_member_document(
	    float_type,
	    anna_mid_get(data_numerical[i][0]),
	    data_numerical[i][1]);
	anna_member_document(
	    complex_type,
	    anna_mid_get(data_numerical[i][0]),
	    data_numerical[i][1]);
    }    
    
    for(i=0; data_numerical_with_alias[i][0]; i++)
    {
	anna_module_doc_item(int_type, data_numerical_with_alias[i][0], L"Int__", data_numerical_with_alias[i][1]);
	anna_module_doc_item(int_type, data_numerical_with_alias[i][0], L"", data_numerical_with_alias[i][1]);

	anna_module_doc_item(char_type, data_numerical_with_alias[i][0], L"Int__", data_numerical_with_alias[i][1]);
	anna_module_doc_item(char_type, data_numerical_with_alias[i][0], L"Char__", data_numerical_with_alias[i][1]);
	anna_module_doc_item(char_type, data_numerical_with_alias[i][0], L"", data_numerical_with_alias[i][1]);

	anna_module_doc_item(float_type, data_numerical_with_alias[i][0], L"Int__", data_numerical_with_alias[i][1]);
	anna_module_doc_item(float_type, data_numerical_with_alias[i][0], L"Float__", data_numerical_with_alias[i][1]);
	anna_module_doc_item(float_type, data_numerical_with_alias[i][0], L"IntReverse__", data_numerical_with_alias[i][1]);

	anna_module_doc_item(complex_type, data_numerical_with_alias[i][0], L"Int__", data_numerical_with_alias[i][1]);
	anna_module_doc_item(complex_type, data_numerical_with_alias[i][0], L"IntReverse__", data_numerical_with_alias[i][1]);
	anna_module_doc_item(complex_type, data_numerical_with_alias[i][0], L"Float__", data_numerical_with_alias[i][1]);
	anna_module_doc_item(complex_type, data_numerical_with_alias[i][0], L"FloatReverse__", data_numerical_with_alias[i][1]);
	anna_module_doc_item(complex_type, data_numerical_with_alias[i][0], L"Complex__", data_numerical_with_alias[i][1]);
    }

    for(i=0; data_iter[i][0]; i++)
    {
	for(j=0;iter_type[j]; j++)
	{
	    anna_member_document(
		iter_type[j],
		anna_mid_get(data_iter[i][0]),
		data_iter[i][1]);
	}
    }
}

void anna_module_init()
{
    /*
      Set up all native modules
    */
    anna_module_data_t modules[] = 
	{
	    { L"reflection", anna_reflection_create_types, anna_reflection_load },
	    { L"lang", anna_lang_create_types, anna_lang_load },
	    { L"parser", anna_parser_create_types, anna_parser_load },
	    { L"system", 0, anna_system_load },
	    { L"cio", 0, anna_cio_load },
	    { L"math", 0, anna_math_load },
	    { L"cerror", 0, anna_cerror_load },
	    { L"ctime", 0, anna_ctime_load },
	};

    anna_module_data_create(modules, stack_global);
    anna_module_doc();
    
    anna_stack_template_t *stack_macro = anna_stack_create(stack_global);
    anna_macro_init(stack_macro);
    al_push(&stack_global->expand, anna_use_create_stack(stack_macro));
    
    anna_stack_template_t *stack_lang = anna_stack_unwrap(
	anna_as_obj(
	    anna_stack_get(
		stack_global, L"lang")));
    
    anna_stack_template_t *stack_parser = anna_stack_unwrap(
	anna_as_obj(
	    anna_stack_get(
		stack_global, L"parser")));

    anna_object_t *g_obj = anna_stack_wrap(stack_global);
    anna_stack_declare(
	stack_global,
	L"global",
	g_obj->type,
	anna_from_obj(g_obj),
	ANNA_STACK_READONLY);
    anna_type_setup_interface(g_obj->type);
    
    /*
      Load a bunch of built in non-native macros and monkey patch some
      of the native types with additional non-native methods.
      
      This must be done in a specific order, since many of these
      patches rely on each other.
      
      Right now, we separate these things into many different files
      for clarity. Long term, we probably want to use as few files as
      possible in order to reduce overhead. We'll worry about that
      once the functionality is mostly set in stone.
    */
    
    anna_module_bootstrap_macro(L"ast");
    anna_module_bootstrap_macro(L"macroUtil");
    anna_module_bootstrap_macro(L"mapping");
    anna_module_bootstrap_macro(L"update");
    anna_module_bootstrap_macro(L"iter");
    anna_module_bootstrap_monkeypatch(stack_parser, L"monkeypatchNode");
    anna_module_bootstrap_macro(L"range");
    anna_module_bootstrap_monkeypatch(stack_lang, L"monkeypatchMisc");
    anna_module_bootstrap_monkeypatch(stack_lang, L"monkeypatchList");
    anna_module_bootstrap_monkeypatch(stack_lang, L"monkeypatchRange");
    anna_module_bootstrap_monkeypatch(stack_lang, L"monkeypatchString");
    anna_module_bootstrap_macro(L"switch");
    anna_module_bootstrap_macro(L"struct");
    anna_module_bootstrap_macro(L"enum");
    anna_module_bootstrap_macro(L"error");
    anna_module_bootstrap_macro(L"expandCode");

    /*
      Load all non-native libraries
    */
    anna_module_init_recursive(L"lib", stack_global);
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
		    anna_error(
			im->child[j],
			L"Invalid module. All module names must be identifiers");
		}
	    }
	    m->child[i] = anna_node_create_null(
		&module->location);	    
	}
    }
}

static void anna_module_find_import(anna_node_t *module, array_list_t *import)
{
    anna_module_find_import_internal(module, L"use", import);
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
		
	if(this2->payload->body)
	{
	    anna_node_each((anna_node_t *)this2->payload->body, &anna_module_compile, 0);
	}
	anna_vm_compile(this2->payload);
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
    if(!module_stack->filename)
    {
        return;
    }

//    debug_level=0;
    int i;

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
    al_push(&module_stack->import, L"lang");
    
    anna_module_find_expand(program, &module_stack->expand);    
    anna_module_find_import(program, &module_stack->import);
    
    for(i=0; i<al_get_count(&module_stack->expand); i++ )
    {
	wchar_t *str = al_get(&module_stack->expand, i);
	debug(D_SPAM,L"expand statement: expand(%ls)\n", str);
	
	anna_stack_template_t *mod = anna_module(stack_global, str, 0);
	anna_module_load_i(mod);
	
	if(anna_error_count || !mod)
	{
	    return;
	}
	al_set(&module_stack->expand, i, anna_use_create_stack(mod));
    }
    
    for(i=0; i<al_get_count(&anna_module_default_macros); i++ )
    {
	anna_stack_template_t *mod = al_get(&anna_module_default_macros, i);
	al_push(&module_stack->expand, anna_use_create_stack(mod));
    }
    
    /*
      Prepare the module. 
    */
    anna_node_t *node = (anna_node_t *)
	anna_node_macro_expand(
	    program,
	    module_stack);
    if(anna_error_count)
    {
	debug(D_CRITICAL,L"Found %d error(s) during macro expansion phase\n", anna_error_count);
	exit(ANNA_STATUS_MACRO_ERROR);
    }
    debug(D_SPAM,L"Macros expanded in module %ls\n", module_stack->filename);    
    
    anna_node_call_t *module_node = node_cast_call(node);

    anna_node_call_t *attribute = 0;
    for(i=0; i<module_node->child_count; i++)
    {
	if(anna_node_is_call_to(module_node->child[i], L"attribute"))
	{
	    anna_stack_wrap(module_stack)->type->attribute = node_cast_call(module_node->child[i]);
	    module_node->child[i] = anna_node_create_null(0);
	    break;
	}
    }
    
    
    anna_node_print(D_SPAM, node);
    anna_node_register_declarations(node, module_stack);
    module_stack->flags |= ANNA_STACK_NAMESPACE;
    if(anna_error_count)
    {
	debug(
	    D_CRITICAL,
	    L"Critical: Found %d error(s) during loading of module %ls\n", 
	    anna_error_count, module_stack->filename);
	exit(ANNA_STATUS_INTERFACE_ERROR);
    }
    debug(
	D_SPAM,
	L"Declarations registered in module %ls\n", 
	module_stack->filename);
    
    for(i=0; i<al_get_count(&module_stack->import); i++ )
    {
	wchar_t *str = al_get(&module_stack->import, i);
	anna_stack_template_t *mod = anna_module(stack_global, str, 0);
	anna_module_load_i(mod);
	if(anna_error_count || !mod)
	{
	    return;
	}
	al_set(&module_stack->import, i, anna_use_create_stack(mod));
    }
    al_push(&module_stack->import, anna_use_create_stack(module_stack));
    
    anna_node_set_stack(node, module_stack);
    anna_node_resolve_identifiers(node);
    
    debug(
	D_SPAM,
	L"Stack set in module %ls\n", 
	module_stack->filename);
    
    module_node = node_cast_call(node);
    debug(
	D_SPAM,
	L"Dependencies imported in module %ls\n", 
	module_stack->filename);
    
    anna_node_calculate_type_children(module_node);
    if(anna_error_count)
    {
	debug(
	    4,
	    L"Found %d error(s) during module loading\n",
	    anna_error_count);
	exit(ANNA_STATUS_TYPE_CALCULATION_ERROR);
    }
    debug(D_SPAM,L"Return types set up for module %ls\n", module_stack->filename);

    for(i=0; i<module_node->child_count; i++)
    {
	anna_node_each(
	    module_node->child[i], 
	    (anna_node_function_t)&anna_node_validate, 
	    module_stack);
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
    
    debug(D_SPAM,L"AST validated for module %ls\n", module_stack->filename);	
	
    for(i=0; i<module_node->child_count; i++)
    {
	anna_node_static_invoke(module_node->child[i], module_stack);
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
    
    debug(D_SPAM,L"Module stack object set up for %ls\n", module_stack->filename);
    anna_node_each((anna_node_t *)module_node, &anna_module_compile, 0);
    
    debug(D_SPAM,L"Module %ls is compiled\n", module_stack->filename);	
    anna_type_setup_interface(anna_stack_wrap(module_stack)->type);
}

anna_object_t *anna_module_load(wchar_t *module_name)
{
    string_buffer_t fn;
    sb_init(&fn);
    sb_printf(&fn, L"%ls.anna", module_name);    

    anna_stack_template_t *module = anna_module(
	stack_global, 0, sb_content(&fn));
    anna_module_load_i(module);
    
    sb_destroy(&fn);
    
    return anna_stack_wrap(module);
}

anna_function_t *anna_module_function(
    anna_stack_template_t *stack,
    wchar_t *name,
    int flags,
    anna_native_t native, 
    anna_type_t *return_type,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn,
    wchar_t *doc
    )
{
    anna_function_t *f = anna_native_create(
	name,
	flags, native,
	return_type, 
	argc, argv, argn,
	stack);
    
    anna_stack_declare(
	stack,
	name,
	f->wrapper->type,
	anna_from_obj(f->wrapper),
	ANNA_STACK_READONLY);
    if(doc)
    {
	anna_function_document(
	    f,
	    anna_intern_static(
		doc));
    }
    return f;
}
