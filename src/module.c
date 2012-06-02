#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <dlfcn.h>

#include "anna/fallback.h"
#include "anna/common.h"
#include "anna/util.h"
#include "anna/base.h"
#include "anna/parse.h"
#include "anna/module.h"
#include "anna/module_data.h"
#include "anna/node.h"
#include "anna/node_create.h"
#include "anna/util.h"
#include "anna/function.h"
#include "anna/stack.h"

#include "anna/function_type.h"
#include "anna/type.h"
#include "anna/macro.h"
#include "anna/member.h"
#include "anna/lib/parser.h"
#include "anna/status.h"
#include "anna/vm.h"
#include "anna/alloc.h"
#include "anna/intern.h"
#include "anna/wutil.h"
#include "anna/attribute.h"
#include "anna/mid.h"
#include "anna/use.h"
#include "anna/lib/clib.h"

#include "anna/lib/lang/object.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/hash.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/lang/pair.h"

static void anna_module_load_i(anna_stack_template_t *module);

array_list_t anna_module_default_macros = AL_STATIC;
array_list_t anna_module_in_transit = AL_STATIC;

static wchar_t *anna_module_bootstrap_directory()
{
    wchar_t *env = wgetenv(L"ANNA_BOOTSTRAP_DIRECTORY");
    return env ? env : ANNA_BOOTSTRAP_DIRECTORY;
}

static void anna_module_path(array_list_t *list)
{
    wchar_t *env = wgetenv(L"ANNA_PATH");
    if(env)
    {
	wchar_t *base = env;
	wchar_t *ptr = env;
	while(*ptr)
	{
	    if(*ptr == L':')
	    {
		*ptr = 0;
		if(wcslen(base))
		{
		    al_push(list, anna_intern(base));
		}
		base = ptr+1;
	    }
	    ptr++;
	}
	if(wcslen(base))
	{
	    al_push(list, anna_intern(base));
	}
    }
    
    al_push(list, L"./lib");
    al_push(list, ANNA_LIB_DIR);
}

static wchar_t *anna_module_search_suffix(wchar_t *path)
{
    int i;
    static string_buffer_t *fn = 0;
    struct stat buf;
    if(!fn)
    {
	fn = malloc(sizeof(string_buffer_t));
	sb_init(fn);
    }
    
    static const wchar_t *suff[] = {L"", L".anna", L".so"};
    
    for(i=0; i<sizeof(suff)/sizeof(suff[0]); i++)
    {
	sb_clear(fn);
	sb_printf(fn, L"%ls%ls", path, suff[i]);    
	if(!wstat(sb_content(fn), &buf))
	{
	    return sb_content(fn);
	}
    }
    return 0;
}


static wchar_t *anna_module_search(
    anna_stack_template_t *parent, wchar_t *name)
{
    string_buffer_t sb;
    sb_init(&sb);
    int i;
    array_list_t path = AL_STATIC;
    anna_module_path(&path);
    wchar_t *res = 0;
    for(i=0; i<al_get_count(&path); i++)
    {
	sb_clear(&sb);
	sb_printf(&sb, L"%ls/%ls", al_get(&path, i), name);
	if((res = anna_module_search_suffix(sb_content(&sb))))
	{
	    break;
	}
    }
    al_destroy(&path);
    sb_destroy(&sb);
    return res;
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
		    debug(D_CRITICAL, L"Multiple locations for module %ls: %ls and %ls\n", name, res->filename, filename);
		    CRASH;		
		}
		res->filename = wrealpath(filename,0);
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
	res->filename = wrealpath(filename,0);
    }
    else
    {
	wchar_t *p = anna_module_search(parent, name);
	res->filename = p?wrealpath(p, 0):0;
    }

    if(!res->filename)
    {
	anna_message(L"Oops %ls\n", res->name);
	
	return 0;
    }

    
    return res;
}

static anna_stack_template_t *anna_module_recursive(
    anna_stack_template_t *parent, array_list_t *path, int offset)
{
    wchar_t *el = (wchar_t *)al_get(path, offset);
    
    anna_stack_template_t *mod = anna_module(parent, el, 0);
    if(!mod)
    {
	return 0;
    }
    anna_module_load_i(mod);
    if(al_get_count(path) > (offset+1))
    {
	return anna_module_recursive(mod, path, offset+1);
    }
    return mod;
}


void anna_module_check(
    anna_stack_template_t *parent, wchar_t *name)
{
    static hash_table_t *cache= 0;
    
    if(!cache)
    {
	cache = malloc(sizeof(hash_table_t));
	hash_init(cache, hash_wcs_func, hash_wcs_cmp);
    }
    long checked = (long)hash_get(cache, name);
    if(checked)
    {
	return;
    }
    hash_put(cache, name, (void *)1);
    
    wchar_t *path = anna_module_search(parent, name);
    if(path)
    {
	anna_module_load_i(
	    anna_module(parent, name, 0));
    }
}

static void anna_module_init_recursive(
    wchar_t *dname, anna_stack_template_t *parent)
{
    DIR *dir = wopendir(dname);
    if(!dir)
    {
	return;
    }

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
	    if(anna_stack_get(parent, d_name))
	    {
		goto CLEANUP;
	    }

	    anna_module_init_recursive(sb_content(&fn), anna_module(parent, d_name, 0));
	    goto CLEANUP;
	}
	
	wchar_t *suffix = wcsrchr(d_name, L'.');
	if(!suffix)
	{
	    goto CLEANUP;
	}

	if((wcscmp(suffix, L".anna") == 0) || (wcscmp(suffix, L".so") == 0))
	{
	    *suffix=0;
	    
	    if(anna_stack_get(parent, d_name))
	    {
		goto CLEANUP;
	    }
	
	    anna_module_load_i(
		anna_module(parent, d_name, sb_content(&fn)));
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
    sb_printf(&sb, L"%ls/%ls.anna", anna_module_bootstrap_directory(), name);
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

void anna_module_const(
    anna_stack_template_t *stack,
    wchar_t *name,
    anna_type_t *type,
    anna_entry_t *value,
    wchar_t *documentation
    )
{
    anna_stack_declare(
	stack,
	name,
	type,
	value,
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
    sb_printf(&sb, L"%ls/%ls.anna", anna_module_bootstrap_directory(), name);
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
	if(!fun)
	{
	    anna_message(L"Invalid monkeypatch in file %ls\n", name);
	    continue;
	}
	
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
	
	anna_type_t *type = anna_type_unwrap(
	    anna_as_obj(
		anna_stack_get(lang, target_id->name)));
	anna_stack_template_t *module = 0;
	if(!type)
	{
	    module = 
		anna_stack_unwrap(
		    anna_as_obj(
			anna_stack_get(lang, target_id->name)));
	    assert(module);
	}
	
	if(type == any_list_type)
	{
	    anna_list_add_method(fun);
	}
	else if(type == hash_type)
	{
	    anna_hash_add_method(fun);
	}
	else if(type == pair_type)
	{
	    anna_pair_add_method(fun);
	}
	else if(type == node_type)
	{
	    anna_node_wrapper_add_method(fun);
	}
	else
	{
	    if(type)
	    {
		anna_member_create_method(type, anna_mid_get(fun->name), fun);
	    }
	    else
	    {
		anna_stack_declare(
		    module,
		    fun->name,
		    anna_function_wrap(fun)->type,
		    anna_from_obj(anna_function_wrap(fun)),
		    ANNA_STACK_READONLY);
	    }
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
	for(i=0; i<anna_argc; i++)
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
    anna_stack_document(stack, L"The system module contains information and functionality on the currently running system.");
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

static void anna_module_load_native(
    anna_stack_template_t *stack)
{
    string_buffer_t sb;
    sb_init(&sb);
    
    void * lib_handle;
    void (*load)(anna_stack_template_t *stack);
    void (*create)(anna_stack_template_t *stack);
    
    lib_handle = wdlopen(stack->filename,RTLD_NOW);
    if(!lib_handle) {
	debug(D_ERROR, L"Failed to open lib %ls: %s\n", stack->name, dlerror());
	goto CLEANUP;
    }

    sb_clear(&sb);
    sb_printf(&sb, L"anna_%ls_create", stack->name);    
    create = (void (*)(anna_stack_template_t *)) wdlsym(lib_handle,sb_content(&sb));
    if(!create) {
	debug(D_ERROR,L"Failed to get create function in library %ls: %s\n", stack->name, dlerror());
	goto CLEANUP;
    }

    sb_clear(&sb);
    sb_printf(&sb, L"anna_%ls_load", stack->name);    
    load = (void (*)(anna_stack_template_t *)) wdlsym(lib_handle,sb_content(&sb));
    if(!load) {
	debug(D_ERROR,L"Failed to get load function in library %ls: %s\n", stack->name, dlerror());
	goto CLEANUP;
    }
    create(stack);
    load(stack);
  CLEANUP:
    sb_destroy(&sb);
}

void anna_module_init(wchar_t *name)
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
	    { L"ctime", 0, anna_ctime_load },
	    { L"debug", anna_debug_create_types, anna_debug_load },
	    { L"mp", anna_mp_create_types, anna_mp_load },
	};

    anna_module_data_create(modules, stack_global);
    anna_module_doc();
    
    anna_stack_template_t *stack_macro = anna_stack_create(stack_global);
    anna_macro_init(stack_macro);
    al_push(&stack_global->expand, anna_use_create_stack(stack_macro));
    
    anna_stack_template_t *stack_lang = anna_stack_unwrap(
	anna_as_obj(
	    anna_stack_get(
		stack_global, anna_intern_static(L"lang"))));
    
    anna_stack_template_t *stack_parser = anna_stack_unwrap(
	anna_as_obj(
	    anna_stack_get(
		stack_global, anna_intern_static(L"parser"))));
    
    anna_stack_template_t *stack_reflection = anna_stack_unwrap(
	anna_as_obj(
	    anna_stack_get(
		stack_global, anna_intern_static(L"reflection"))));
    
    anna_stack_template_t *stack_debug = anna_stack_unwrap(
	anna_as_obj(
	    anna_stack_get(
		stack_global, anna_intern_static(L"debug"))));
    
    anna_object_t *g_obj = anna_stack_wrap(stack_global);
    anna_stack_declare(
	stack_global,
	anna_intern_static(L"global"),
	g_obj->type,
	anna_from_obj(g_obj),
	ANNA_STACK_READONLY);

    anna_stack_declare(
	stack_global,
	anna_intern_static(L"__name__"),
	imutable_string_type,
	anna_from_obj(anna_string_create(wcslen(name), name)),
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
    anna_module_bootstrap_macro(L"attributes");
    anna_module_bootstrap_macro(L"iter");
    anna_module_bootstrap_macro(L"collectionMacro");
    anna_module_bootstrap_monkeypatch(stack_parser, L"monkeypatchNode");
    anna_module_bootstrap_monkeypatch(stack_global, L"monkeypatchLang");
    anna_module_bootstrap_macro(L"macroMisc");
    anna_module_bootstrap_macro(L"range");
    anna_module_bootstrap_monkeypatch(stack_lang, L"monkeypatchMisc");
    anna_module_bootstrap_monkeypatch(stack_lang, L"monkeypatchList");
    anna_module_bootstrap_monkeypatch(stack_lang, L"monkeypatchRange");
    anna_module_bootstrap_monkeypatch(stack_lang, L"monkeypatchString");
    anna_module_bootstrap_macro(L"switch");
    anna_module_bootstrap_macro(L"struct");
    anna_module_bootstrap_macro(L"enum");
    anna_module_bootstrap_macro(L"errorMacros");
    anna_module_bootstrap_macro(L"expandCode");
    anna_module_bootstrap_macro(L"propertyAttribute");
    anna_module_bootstrap_macro(L"class");
    anna_module_bootstrap_macro(L"listAssign");
    anna_module_bootstrap_monkeypatch(stack_debug, L"monkeypatchDebug");
    anna_module_bootstrap_monkeypatch(stack_reflection, L"monkeypatchContinuation");

    /* Load additional binary modules */
    //anna_module_load_dynamic(L"unix", stack_global);

    /*
      Load all non-native libraries
    */

    int i;
    array_list_t path = AL_STATIC;
    anna_module_path(&path);
    for(i=0; i<al_get_count(&path); i++)
    {
	anna_module_init_recursive((wchar_t *)al_get(&path, i), stack_global);
    }
    al_destroy(&path);
}

static void import_parse(anna_node_t *node, array_list_t *list)
{
    
    if(node->node_type == ANNA_NODE_IDENTIFIER)
    {
	anna_node_identifier_t *id = (anna_node_identifier_t *)node;
	al_push(list, id->name);
	return;	
    }
    if(anna_node_is_call_to(node, L"__memberGet__"))
    {
	anna_node_call_t *call = (anna_node_call_t *)node;
	if(call->child_count == 2 && call->child[1]->node_type == ANNA_NODE_IDENTIFIER)
	{
	    import_parse(call->child[0], list);
	    anna_node_identifier_t *id = (anna_node_identifier_t *)call->child[1];
	    al_push(list, id->name);
	    return;
	}	
    }

    anna_error(
	node,
	L"Invalid module. All module names must be valid namespaces");
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
		array_list_t *el_list = malloc(sizeof(array_list_t));
		al_init(el_list);
		al_push(import, el_list);		
		import_parse(im->child[j], el_list);
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

/**
   Internal helper function for anna_module_load_i.

   This function runs after anna_module_load_i has been run on a
   module and (resursively) on all it's dependencies.

   anna_module_load_i will by itself load the module file and parse it
   enough so that it's public interface can be determined. After that,
   right before the root call to anna_module_load_i finishes,
   anna_module_load_i_phase_2 is called once, and it will finish
   loading all such partially loaded modules, which involves resolving
   all AST node types, validating all the source code, and finally
   compiling it.

 */
static void anna_module_load_i_phase_2()
{
    int j;
    if(anna_error_count != 0)
    {
	goto CLEANUP;
    }
    
    
    for(j=0; j<al_get_count(&anna_module_in_transit); j+= 2)
    {
	anna_stack_template_t *module_stack = al_get(&anna_module_in_transit, j);
	anna_node_t *node = al_get(&anna_module_in_transit, j+1);
	anna_node_call_t *module_node = node_cast_call(node);
	    
	anna_node_resolve_identifiers(node);
	debug(
	    D_SPAM,
	    L"Identifiers resolved in module %ls\n", 
	    module_stack->filename);	    
	
	anna_node_calculate_type_children((anna_node_t *)module_node);
	if(anna_error_count)
	{
	    debug(
		D_ERROR,
		L"Found %d error(s) during module loading\n",
		anna_error_count);
	    goto CLEANUP;
	}
	debug(D_SPAM,L"Return types set up for module %ls\n", module_stack->filename);
	
    }
    for(j=0; j<al_get_count(&anna_module_in_transit); j+= 2)
    {
	anna_stack_template_t *module_stack = al_get(&anna_module_in_transit, j);
	anna_node_t *node = al_get(&anna_module_in_transit, j+1);
	anna_node_call_t *module_node = node_cast_call(node);

	anna_node_each(
	    (anna_node_t *)module_node, 
	    (anna_node_function_t)&anna_node_validate, 
	    module_stack);
	
	if(anna_error_count)
	{
	    debug(
		D_ERROR,
		L"Found %d error(s) during module loading\n",
		anna_error_count);
	    goto CLEANUP;
	}
	
	debug(D_SPAM,L"AST validated for module %ls\n", module_stack->filename);
	anna_node_each((anna_node_t *)module_node, &anna_module_compile, 0);
	
	debug(D_SPAM,L"Module %ls is compiled\n", module_stack->filename);	
	anna_type_setup_interface(anna_stack_wrap(module_stack)->type);

    }    
  CLEANUP:
    al_truncate(&anna_module_in_transit, 0);
}

static void anna_module_load_ast(anna_stack_template_t *module_stack, anna_node_t *program);

/**
   Actually perform the loading of a module.

   Module loading happens in two phases. The first phase is handled
   internally by anna_module_load_i, and involves reading files, and
   performing macro expansion and enough parsing of the resulting AST
   tree to generate a public interface for the module. During phase 1,
   anna_module_load_i will recursively call itself in order to load
   any additional modules used by the original module.
   
   Right before the original root call to anna_module_load_i returns,
   it calls anna_module_load_i_phase_2 is called. This function will
   take all the semi-compiled modules and finish compiling them. The
   reason why this two-phase compilation is eeded is because modules
   can have mutual interdependencies that mean that module A needs to
   know the public interface of module B when compiling and vice
   versa.
*/
static void anna_module_load_i(anna_stack_template_t *module_stack)
{
    /*
      This variable is used to keep track of whether we are the root
      call anna_module_load_i or not.
    */
    
    if(!module_stack->filename)
    {
        return;
    }

    if(module_stack->flags & ANNA_STACK_LOADED)
    {
	return;
    }
    module_stack->flags |= ANNA_STACK_LOADED;
    
    wchar_t *suffix = wcsrchr(module_stack->filename, L'.');
    if(suffix && wcscmp(suffix, L".so")==0)
    {
	debug(D_SPAM,L"Load native library %ls...\n", module_stack->filename);    
	anna_module_load_native(module_stack);
	anna_type_setup_interface(anna_stack_wrap(module_stack)->type);
    }
    else
    {
	debug(D_SPAM, L"Parsing file %ls...\n", module_stack->filename);
	anna_node_t *program = anna_parse(module_stack->filename);
	anna_module_load_ast(module_stack, program);
    }
    
    anna_type_t *module_type = anna_stack_wrap(module_stack)->type;
    if(module_stack->name && anna_attribute_flag(module_type->attribute, L"internal"))
    {
	anna_type_t *parent_type = anna_stack_wrap(module_stack->parent)->type;

	anna_member_t *module_member = anna_member_get(parent_type, anna_mid_get(module_stack->name));
	anna_member_set_internal(module_member, 1);
    }
}

static void anna_module_load_ast(anna_stack_template_t *module_stack, anna_node_t *program)
{
    int i;
    static int recursion_count = 0;
    
    if(!program || anna_error_count) 
    {
	debug(D_ERROR,L"Module %ls failed to parse correctly; exiting %d.\n", module_stack->filename);
	return;
    }
    
    debug(D_SPAM,L"Parsed AST for module %ls:\n", module_stack->filename);    
    anna_node_print(D_SPAM, program);    

    /*
      Implicitly add an import
      of the lang module to the top of the ast.
    */
    array_list_t *lang_list = malloc(sizeof(array_list_t));    
    al_init(lang_list);
    al_push(lang_list, L"lang");
    al_push(&module_stack->import, lang_list);
    
    anna_module_find_expand(program, &module_stack->expand);    
    anna_module_find_import(program, &module_stack->import);
    
    for(i=0; i<al_get_count(&module_stack->expand); i++ )
    {
	array_list_t *el_list = (array_list_t *)al_get(&module_stack->expand, i);
	anna_stack_template_t *mod = anna_module_recursive(stack_global, el_list, 0);
	al_destroy(el_list);
	free(el_list);

	if(!mod)
	{
	    anna_error(
		program,
		L"Invalid module");
	}
	
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
	debug(D_ERROR,L"Found %d error(s) during macro expansion phase\n", anna_error_count);
	return;
    }
    
    anna_node_call_t *module_node = node_cast_call(node);

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
	    D_ERROR,
	    L"Found %d error(s) during loading of module %ls\n", 
	    anna_error_count, module_stack->filename);
	return;
    }
    debug(
	D_SPAM,
	L"Declarations registered in module %ls\n", 
	module_stack->filename);
    
    anna_node_set_stack(node, module_stack);
    debug(
	D_SPAM,
	L"Stack set in module %ls\n", 
	module_stack->filename);
    
    recursion_count++;
    
    for(i=0; i<al_get_count(&module_stack->import); i++ )
    {
	array_list_t *el_list = (array_list_t *)al_get(&module_stack->import, i);
	anna_stack_template_t *mod = anna_module_recursive(stack_global, el_list, 0);
	al_destroy(el_list);
	free(el_list);
	
	if(!mod)
	{
	    anna_error(
		program,
		L"Invalid module");
	}
	
	if(anna_error_count || !mod)
	{
	    goto CLEANUP;
	}
	al_set(&module_stack->import, i, anna_use_create_stack(mod));
    }
    al_push(&module_stack->import, anna_use_create_stack(module_stack));    
    
    al_push(&anna_module_in_transit, module_stack);    
    al_push(&anna_module_in_transit, node);    
    
    for(i=0; i<module_node->child_count; i++)
    {
	anna_node_static_invoke(module_node->child[i], module_stack);
	if(anna_error_count)
	{
	    debug(
		D_ERROR,
		L"Found %d error(s) during module loading\n",
		anna_error_count);
	    goto CLEANUP;
	}
    }    
    debug(D_SPAM,L"Module stack object set up for %ls\n", module_stack->filename);
    
  CLEANUP:

    /*
      Even on error, we need to call anna_module_load_i_phase_2,
      because otherwise, the phase 2 work queue will not be
      emptied. That's why this code lives in the cleanup section.
     */
    if(recursion_count > 1)
    {
	debug(
	    D_SPAM,
	    L"Postponing recursive load of %ls\n", 
	    module_stack->filename);
    }
    else
    {
	anna_module_load_i_phase_2();	
    }
    
    recursion_count--;    
}

anna_object_t *anna_module_create(
    anna_node_t *node)
{
    anna_stack_template_t *module = 
	anna_stack_create(stack_global);
    anna_stack_name(module, L"!anonymous");
    anna_module_load_ast(
	module, node);
    
    return module?anna_stack_wrap(module):0;
}

anna_object_t *anna_module_load(wchar_t *module_name)
{
    anna_stack_template_t *module=0;
    wchar_t *full_path = anna_module_search_suffix(module_name);
    if(full_path)
    {
	module = anna_module(
	    stack_global, 0, full_path);
	anna_module_load_i(module);
    }
    else
    {
	anna_error(0, L"Failed to find module named «%ls»", module_name);
    }
    return module?anna_stack_wrap(module):0;
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
    anna_node_t **argd,
    wchar_t *doc
    )
{
    anna_function_t *f = anna_native_create(
	name,
	flags, native,
	return_type, 
	argc, argv, argn, argd,
	stack);    
    anna_stack_declare(
	stack,
	name,
	f->wrapper->type,
	anna_from_obj(f->wrapper),
	ANNA_STACK_READONLY);
    if(doc)
    {
	anna_function_document(f,anna_intern_static(doc));
    }
    return f;
}

