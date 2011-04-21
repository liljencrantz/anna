#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <locale.h>

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

#define ABIDES_IN_TRANSIT -1

static hash_table_t anna_abides_cache;

anna_type_t *type_type=0, 
    *object_type=0,
    *int_type=0, 
    *null_type=0,
    *string_type=0, 
    *char_type=0,
    *list_type=0,
    *float_type=0,
    *member_type=0,
    *range_type=0,
    *complex_type=0,
    *hash_type=0,
    *pair_type=0;

anna_object_t *null_object=0;

static hash_table_t anna_type_for_function_identifier;

anna_node_t *anna_node_null=0;

anna_stack_template_t *stack_global;

__pure anna_object_t **anna_member_addr_get_mid(anna_object_t *obj, mid_t mid)
{
    /*
      debug(D_SPAM,L"Get mid %d on object\n", mid);
      debug(D_SPAM,L"of type %ls\n", obj->type->name);
    */
    
    anna_member_t *m = obj->type->mid_identifier[mid];
    //wprintf(L"Get member %ls in object of type %ls\n", anna_mid_get_reverse(mid), obj->type->name);
    
    if(unlikely(!m)) 
    {
	return 0;
    }
    //debug(D_SPAM,L"Found! Pos is %d, static is %d\n", m->offset, m->is_static);
    
    //debug(D_SPAM,L"Lala, get addr of member %ls\n", m->name);    
    if(m->is_static) {
	return &obj->type->static_member[m->offset];
    } else {
	return &(obj->member[m->offset]);
    }
}

__pure anna_object_t **anna_static_member_addr_get_mid(anna_type_t *type, mid_t mid)
{
    /*  debug(D_SPAM,L"Get mid %d on object\n", mid);
	debug(D_SPAM,L"of type %ls\n", obj->type->name);
    */
    anna_member_t *m = type->mid_identifier[mid];
    if(unlikely(!m)) 
    {
	return 0;
    }
    
    //debug(D_SPAM,L"Lala, get addr of member %ls\n", m->name);    
    if(m->is_static) {
	return &type->static_member[m->offset];
    } else {
	return 0;
    }
}

static int hash_function_type_func(void *a)
{
    anna_function_type_t *key = (anna_function_type_t *)a;
    int res = (int)(long)key->return_type ^ key->flags;
    size_t i;
    
    for(i=0;i<key->input_count; i++)
    {
	res = (res<<19) ^ (int)(long)key->input_type[i] ^ (res>>13);
	res ^= wcslen(key->input_name[i]);
    }
    
    return res;
}

static int hash_function_type_comp(void *a, void *b)
{
    size_t i;
    
    anna_function_type_t *key1 = (anna_function_type_t *)a;
    anna_function_type_t *key2 = (anna_function_type_t *)b;

    //debug(D_SPAM,L"Compare type for function %ls %d and %ls %d\n", key1->result->name, hash_function_type_func(key1), key2->result->name, hash_function_type_func(key2));

    if(key1->return_type != key2->return_type)
	return 0;
    if(key1->input_count != key2->input_count)
	return 0;
    if(key1->flags != key2->flags)
	return 0;

    for(i=0;i<key1->input_count; i++)
    {
	if(key1->input_type[i] != key2->input_type[i])
	    return 0;
	if(wcscmp(key1->input_name[i], key2->input_name[i]) != 0)
	    return 0;
    }
    //debug(D_SPAM,L"Same!\n");
    
    return 1;
}


anna_type_t *anna_type_for_function(
    anna_type_t *result, 
    size_t argc, 
    anna_type_t **argv, 
    wchar_t **argn, 
    int flags)
{

    //static int count=0;
    //if((count++)==10) {CRASH};
    flags = flags & (ANNA_FUNCTION_VARIADIC | ANNA_FUNCTION_MACRO | ANNA_FUNCTION_CONTINUATION | ANNA_FUNCTION_METHOD_WRAPPER);
    size_t i;
    static anna_function_type_t *key = 0;
    static size_t key_sz = 0;
    size_t new_key_sz = sizeof(anna_function_type_t) + sizeof(anna_type_t *)*argc;
    
    if(!result)
    {
	debug(D_CRITICAL,
	    L"Critical: Function lacks return type!\n");
	CRASH;
    }
    
    if(argc)
	assert(argv);
    
    if(new_key_sz>key_sz)
    {
	int was_null = (key==0);
	key = realloc(key, new_key_sz);
	key_sz = new_key_sz;
	key->input_name = was_null?malloc(sizeof(wchar_t *)*argc):realloc(key->input_name, sizeof(wchar_t *)*argc);
    }
    
    key->flags = flags;
    key->return_type=result;
    key->input_count = argc;
    
    for(i=0; i<argc;i++)
    {
	if(argv[i] && wcscmp(argv[i]->name, L"!FakeFunctionType")==0)
	{
	    debug(D_CRITICAL,
		L"Critical: Tried to get a function key for function with uninitialized argument types\n");
	    CRASH;
	}
	
	key->input_type[i]=argv[i];
	key->input_name[i]=argn[i];
    }
/*
    debug(D_SPAM,L"Weee %ls <-", result->name);
    for(i=0;i<argc; i++)
    {
	debug(D_SPAM,L" %ls", argv[i]->name);	
    }
    debug(D_SPAM,L"\n");
*/
    anna_type_t *res = hash_get(&anna_type_for_function_identifier, key);
    if(!res)
    {
	anna_function_type_t *new_key = malloc(new_key_sz);
	memcpy(new_key, key, new_key_sz);	
	new_key->input_name = malloc(sizeof(wchar_t *)*argc);
	for(i=0; i<argc;i++)
	{
	    new_key->input_name[i]=wcsdup(argn[i]);
	}
	static int num=0;
	
	string_buffer_t sb;
	sb_init(&sb);
	wchar_t *fn = L"def";
	if(flags & ANNA_FUNCTION_MACRO)
	{ 
	    fn = L"macro";
	}
	else if(flags&ANNA_FUNCTION_CONTINUATION)
	{
	    fn = L"continuation";
	}
	
	sb_printf(&sb, L"!%ls %ls (", fn, result->name);
	for(i=0; i<argc;i++)
	{
	    wchar_t *dots = (i==argc-1) && (flags & ANNA_FUNCTION_VARIADIC)?L"...":L"";
	    
	    sb_printf(&sb, L"%ls%ls %ls%ls", i==0?L"":L", ", argv[i]->name, dots, argn[i]);
	}
	sb_printf(&sb, L")%d", num++);
	
	res = anna_type_native_create(sb_content(&sb), stack_global);
	sb_destroy(&sb);
	hash_put(&anna_type_for_function_identifier, new_key, res);
	anna_function_type_create(new_key, res);
    }

    anna_function_type_t *ggg = anna_function_type_unwrap(res);
    assert(ggg->input_count == argc);
    
    return res;
}

anna_type_t *anna_type_intersect(anna_type_t *t1, anna_type_t *t2)
{
    if(t1 == t2)
    {
	return t1;
    }
    /*
      FIXME: Do proper intersection!
    */
    
    return object_type;
}

static int anna_abides_function(
    anna_function_type_t *contender,
    anna_function_type_t *role_model,
    int is_method)
{
    return 1;
}


int anna_abides_fault_count(anna_type_t *contender, anna_type_t *role_model)
{
    
    if(contender == role_model || contender == null_type)
    {
	return 0;
    }

    anna_tt_t tt = 
	{
	    contender, role_model
	}
    ;
    
    long count = (long)hash_get(&anna_abides_cache, &tt);
    if(count == ABIDES_IN_TRANSIT)
    {
	return 0;
    }
    else if(count != 0)
    {
	return count - 1;
    }

    static int level = 0;

    level++;

    hash_put(&anna_abides_cache, anna_tt_make(contender, role_model), (void *)(long)ABIDES_IN_TRANSIT);

    
    size_t i;
    int res = 0;    

    if(!contender)
    {
	CRASH;
    }
    
    if(!role_model)
    {
	CRASH;
    }
    /*  
    if(level==1)
	debug(D_ERROR,L"Check if type %ls abides to %ls\n", contender->name, role_model->name);
    */
    //debug(D_SPAM,L"Role model %ls has %d members\n", role_model->name, role_model->member_count+role_model->static_member_count);
    wchar_t **members = calloc(sizeof(wchar_t *), hash_get_count(&role_model->name_identifier));
    anna_type_get_member_names(role_model, members);    
    
    for(i=0; i<anna_type_member_count(role_model); i++)
    {
	assert(members[i]);
	if(wcscmp(members[i], L"__init__") == 0)
	    continue;
	if(wcscmp(members[i], L"__del__") == 0)
	    continue;

	anna_member_t *c_memb = anna_member_get(
	    contender, 
	    anna_mid_get(members[i]));	
	anna_member_t *r_memb = anna_member_get(
	    role_model, 
	    anna_mid_get(members[i]));
	int ok=1;
	if(!c_memb)
	{
	    ok=0;
/*	    if(!ok && level==1)
		wprintf(L"Miss on %ls because of missing member in contender\n", members[i]);
*/
	}
	else if(r_memb->is_method != c_memb->is_method)
	{
	    ok=0;
/*	    if(!ok && level==1)
		wprintf(L"Miss on %ls because of one is a method\n", members[i]);
*/
	}
	else if(r_memb->is_method)
	{
	    ok = anna_abides_function(
		anna_function_type_unwrap(c_memb->type),
		anna_function_type_unwrap(r_memb->type),
		1);
/*
	    if(!ok && level==1)
	    wprintf(L"Miss on %ls because method signature mismatch\n", members[i]);
*/
	}
	else
	{
	    ok = anna_abides(c_memb->type, r_memb->type);
/*	    
	    if(!ok && level==1)
		wprintf(L"Miss on %ls because of %ls\n", members[i], c_memb->type?L"incompatibility":L"missing member");
*/	    
	}

	res += !ok;
    }
    free(members);
/*
    if(level==1)
	wprintf(L"%ls abides to %ls: %ls\n", contender->name, role_model->name, res==0?L"true": L"false");
*/  
    hash_put(&anna_abides_cache, anna_tt_make(contender, role_model), (void *)(long)(res+1));
    level--;
    

    return res;
}

int anna_abides(anna_type_t *contender, anna_type_t *role_model)
{
    return !anna_abides_fault_count(contender, role_model);
}


size_t anna_native_method_create(
    anna_type_t *type,
    mid_t mid,
    wchar_t *name,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn)
{
    if(!flags) 
    {
	if(!result)
	{
	    CRASH;
	}
	
	if(argc) 
	{
	    assert(argv);
	    assert(argn);
	}
    }
    
    mid = anna_member_create(
	type,
	mid, 
	name,
	1,
	anna_type_for_function(
	    result, 
	    argc, 
	    argv,
	    argn,
	    flags));
    anna_member_t *m = type->mid_identifier[mid];
    //debug(D_SPAM,L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    m->is_method=1;
    type->static_member[m->offset] = 
	anna_function_wrap(
	    anna_native_create(
		name, flags, func, result, 
		argc, argv, argn,
		0));
    return (size_t)mid;
}

static void anna_init()
{
    hash_init(
	&anna_type_for_function_identifier,
	&hash_function_type_func,
	&hash_function_type_comp);
    anna_mid_init();
    anna_slab_init();
    
    stack_global = anna_stack_create(0);
    stack_global->flags |= ANNA_STACK_NAMESPACE;
    
    hash_init(&anna_abides_cache, hash_tt_func, hash_tt_cmp);
}

int main(int argc, char **argv)
{
    wsetlocale(LC_ALL, L"");
    wsetlocale(LC_NUMERIC, L"C");
    
    if(argc != 2)
    {
	debug(D_CRITICAL,L"Error: Expected at least one argument, a name of a file to run.\n");
	exit(ANNA_STATUS_ARGUMENT_ERROR);
    }
    
    wchar_t *module_name = str2wcs(argv[1]);
    
    debug(D_SPAM,L"Initializing interpreter...\n");    
    anna_init();
    null_object = anna_object_create_raw(0);    
    anna_module_init();
    
    if(anna_error_count)
    {
	debug(D_CRITICAL,L"Found %d error(s) during initialization, exiting\n", anna_error_count);
	exit(1);
    }
    
    null_object->type = null_type;
    anna_int_one = anna_int_create(1);
    anna_int_minus_one = anna_int_create(-1);
    anna_int_zero = anna_int_create(0);
    anna_vm_init();
    
    anna_stack_template_t *module = anna_stack_unwrap(anna_module_load(module_name));
    
    anna_stack_populate_wrapper(stack_global);

    anna_object_t **main_wrapper_ptr = anna_stack_addr_get(module, L"main");
    if(!main_wrapper_ptr)
    {
	debug(D_CRITICAL,L"No main method defined in module %ls\n", module_name);
	exit(1);	
    }
    
    debug(D_SPAM,L"Program fully loaded and ready to be executed\n");    
    anna_vm_run(*main_wrapper_ptr, 0, 0);

#ifdef ANNA_FULL_GC_ON_SHUTDOWN
    anna_gc_destroy();
    anna_vm_destroy();
    anna_mid_destroy();
    hash_foreach(&anna_type_for_function_identifier, fun_key_free);
    hash_destroy(&anna_type_for_function_identifier);
#endif
    
/*    
    anna_function_t *main_func = anna_function_unwrap(*main_wrapper_ptr);
    if(!main_func)
    {
	debug(D_SPAM,L"\"main\" member of module \"%ls\" is not a function\n", module_name);
	exit(1);	
    }
    
    debug(D_SPAM,L"Output:\n");        
    anna_function_invoke(main_func, 0, 0, module);
    debug(D_SPAM,L"\n");
*/

    return 0;
}
