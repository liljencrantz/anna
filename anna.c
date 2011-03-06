#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

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

anna_type_t *type_type=0, 
    *object_type=0,
    *int_type=0, 
    *null_type=0,
    *string_type=0, 
    *char_type=0,
    *list_type=0,
    *float_type=0,
    *member_type=0,
    *range_type=0;

anna_object_t *null_object=0;

static hash_table_t anna_type_for_function_identifier;

anna_node_t *anna_node_null=0;

anna_stack_template_t *stack_global;

anna_object_t *anna_i_function_wrapper_call(anna_node_call_t *node, anna_stack_template_t *stack);

anna_object_t **anna_member_addr_get_str(
    anna_object_t *obj,
    wchar_t *name)
{
    anna_member_t *m = (anna_member_t *)hash_get(&(obj->type->name_identifier), name);
    /*
      debug(D_SPAM,L"Woo, get address of member %ls on object\n", name);
      debug(D_SPAM,L"of type %ls\n", obj->type->name);
    */
    if(!m) 
    {
	debug(D_SPAM,L"ERROR!!! Object %d of type %ls does not have a member %ls\n",obj, obj->type->name, name);
	exit(ANNA_STATUS_RUNTIME_ERROR);	
    }
    if(m->is_static) {
	return &obj->type->static_member[m->offset];
    } else {
	return &(obj->member[m->offset]);
    }
}

anna_object_t **anna_member_addr_get_mid(anna_object_t *obj, mid_t mid)
{
    /*
      debug(D_SPAM,L"Get mid %d on object\n", mid);
      debug(D_SPAM,L"of type %ls\n", obj->type->name);
    */
    anna_member_t *m = obj->type->mid_identifier[mid];
    if(!m) 
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

anna_object_t **anna_static_member_addr_get_mid(anna_type_t *type, mid_t mid)
{
    /*  debug(D_SPAM,L"Get mid %d on object\n", mid);
	debug(D_SPAM,L"of type %ls\n", obj->type->name);
    */
    anna_member_t *m = type->mid_identifier[mid];
    if(!m) 
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
    anna_function_type_key_t *key = (anna_function_type_key_t *)a;
    int res = (int)(long)key->result ^ key->flags;
    size_t i;
    
    for(i=0;i<key->argc; i++)
    {
	res = (res<<19) ^ (int)(long)key->argv[i] ^ (res>>13);
	res ^= wcslen(key->argn[i]);
    }
    
    return res;
}

static int hash_function_type_comp(void *a, void *b)
{
    size_t i;
    
    anna_function_type_key_t *key1 = (anna_function_type_key_t *)a;
    anna_function_type_key_t *key2 = (anna_function_type_key_t *)b;

    //debug(D_SPAM,L"Compare type for function %ls %d and %ls %d\n", key1->result->name, hash_function_type_func(key1), key2->result->name, hash_function_type_func(key2));

    if(key1->result != key2->result)
	return 0;
    if(key1->argc != key2->argc)
	return 0;
    if(key1->flags != key2->flags)
	return 0;

    for(i=0;i<key1->argc; i++)
    {
	if(key1->argv[i] != key2->argv[i])
	    return 0;
	if(wcscmp(key1->argn[i], key2->argn[i]) != 0)
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
    flags = flags & (ANNA_FUNCTION_VARIADIC | ANNA_FUNCTION_MACRO);
    size_t i;
    static anna_function_type_key_t *key = 0;
    static size_t key_sz = 0;
    size_t new_key_sz = sizeof(anna_function_type_key_t) + sizeof(anna_type_t *)*argc;
    
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
	key->argn = was_null?malloc(sizeof(wchar_t *)*argc):realloc(key->argn, sizeof(wchar_t *)*argc);
    }
    
    key->flags = flags;
    key->result=result;
    key->argc = argc;
    
    for(i=0; i<argc;i++)
    {
	if(argv[i] && wcscmp(argv[i]->name, L"!FakeFunctionType")==0)
	{
	    debug(D_CRITICAL,
		L"Critical: Tried to get a function key for function with uninitialized argument types\n");
	    CRASH;
	}
	
	key->argv[i]=argv[i];
	key->argn[i]=argn[i];
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
	anna_function_type_key_t *new_key = malloc(new_key_sz);
	memcpy(new_key, key, new_key_sz);	
	new_key->argn = malloc(sizeof(wchar_t *)*argc);
	for(i=0; i<argc;i++)
	{
	    new_key->argn[i]=wcsdup(argn[i]);
	}
	static int num = 0;
	string_buffer_t sb;
	sb_init(&sb);
	sb_printf(&sb, L"%ls%d", L"!FunctionType", num++);
	res = anna_type_native_create(sb_content(&sb), stack_global);
	sb_destroy(&sb);
	hash_put(&anna_type_for_function_identifier, new_key, res);
	anna_function_type_create(new_key, res);
    }

    anna_function_type_key_t *ggg = anna_function_unwrap_type(res);
    assert(ggg->argc == argc);
    
    return res;
}


anna_object_t *anna_function_wrapped_invoke(anna_object_t *obj, 
					    anna_object_t *this,
					    size_t param_count,
					    anna_node_t **param,
					    anna_stack_template_t *local)
{
//    debug(D_SPAM,L"Wrapped invoke of function %ls\n", obj->type->name);
    if(obj == null_object)
	return null_object;
    
    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(obj, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_template_t **stack_ptr = (anna_stack_template_t **)anna_member_addr_get_mid(obj, ANNA_MID_FUNCTION_WRAPPER_STACK);
    if(function_ptr) 
    {
//	if(stack_ptr)
//	{
//	    debug(D_CRITICAL,L"Invoking wrapped function %ls with parent stack\n", (*function_ptr)->name);
//	    anna_stack_print(*stack_ptr);
//	}
        return anna_function_invoke(*function_ptr, this, param_count, param, local, *stack_ptr);
    }
    else 
    {
	anna_object_t **function_wrapper_ptr = anna_static_member_addr_get_mid(
	    obj->type, 
	    ANNA_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    return anna_function_wrapped_invoke(
		*function_wrapper_ptr,
		obj,
		param_count,
		param,
		stack_ptr?*stack_ptr:stack_global);
	}
	
	debug(D_CRITICAL,L"Critical: Tried to call a non-function\n");
	anna_object_print(obj);
	
	CRASH;
    }
    return 0;
}

anna_object_t *anna_construct(
    anna_type_t *type,
    struct anna_node_call *param,
    anna_stack_template_t *stack)
{
    anna_object_t *result = anna_object_create(type);
    //debug(D_SPAM,L"Creating new object of type %ls\n", type->name);
    assert(type->name);
    
    anna_object_t **constructor_ptr = anna_member_addr_get_mid(
	result,
	ANNA_MID_INIT_PAYLOAD);
    if(constructor_ptr)
    {
	anna_function_wrapped_invoke(
	    *constructor_ptr, 
	    result,
	    param->child_count,
	    param->child,
	    stack);
    }
    
    return result;
}

anna_object_t *anna_method_wrap(anna_object_t *method, anna_object_t *owner)
{
    anna_function_t *function_original = anna_function_unwrap(method);
    size_t func_size = sizeof(anna_function_t);
    anna_function_t *function_copy = malloc(func_size);
    
    memcpy(
	function_copy,
	function_original,
	func_size);
    function_copy->this = owner;
    function_copy->wrapper = 
	anna_object_create(
	    function_original->wrapper->type);
    memcpy(
	anna_member_addr_get_mid(
	    anna_function_wrap(function_copy),
	    ANNA_MID_FUNCTION_WRAPPER_PAYLOAD), 
	&function_copy, sizeof(anna_function_t *));
    memcpy(
	anna_member_addr_get_mid(
	    anna_function_wrap(function_copy),
	    ANNA_MID_FUNCTION_WRAPPER_STACK),
	anna_member_addr_get_mid(
	    anna_function_wrap(function_original),
	    ANNA_MID_FUNCTION_WRAPPER_STACK),
	sizeof(anna_stack_template_t *));
    return anna_function_wrap(function_copy);
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

int anna_abides_fault_count(anna_type_t *contender, anna_type_t *role_model)
{
    if(contender == role_model)
    {
	return 0;
    }
    size_t i;
    int res = 0;    

    if(!contender){CRASH;}
    assert(role_model);
    
//    debug(D_SPAM,L"Check type %ls abides against %ls\n", contender->name, role_model->name);
    
    //debug(D_SPAM,L"Role model %ls has %d members\n", role_model->name, role_model->member_count+role_model->static_member_count);
    wchar_t **members = calloc(sizeof(wchar_t *), anna_type_member_count(role_model));
    anna_type_get_member_names(role_model, members);    
    
    for(i=0; i<anna_type_member_count(role_model); i++)
    {
	assert(members[i]);
	anna_type_t *contender_subtype = anna_type_member_type_get(contender, members[i]);
	anna_type_t *role_model_subtype = anna_type_member_type_get(role_model, members[i]);
	if(!contender_subtype || !anna_abides(contender_subtype, role_model_subtype))
	{
	    res++;
	}	
    }
    free(members);
    return res;
}

int anna_abides(anna_type_t *contender, anna_type_t *role_model)
{
    return !anna_abides_fault_count(contender, role_model);
}


void anna_member_redeclare(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *member_type)
{
    type->mid_identifier[mid]->type = member_type;
}


size_t anna_native_method_create(
    anna_type_t *type,
    mid_t mid,
    wchar_t *name,
    int flags,
    anna_native_function_t func,
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
    anna_native_t func_u;
    func_u.function=func;
    type->static_member[m->offset] = 
	anna_function_wrap(
	    anna_native_create(
		name, flags, func_u, result, 
		argc, argv, argn,
		0));
    return (size_t)mid;
}

size_t anna_method_create(anna_type_t *type,
			  mid_t mid,
			  wchar_t *name,
			  int unused(flags),
			  anna_function_t *definition)		
{
    mid = anna_member_create(type, mid, name, 1, definition->wrapper->type);
    anna_member_t *m = type->mid_identifier[mid];
    m->is_method=1;
    
    //debug(D_SPAM,L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    type->static_member[m->offset] = anna_function_wrap(definition);
    return (size_t)mid;
}

/**
   This method is the best ever! All method calls on the null object run this
*/
anna_object_t *anna_i_null_function(anna_object_t **unused(node_base))
{
    return null_object;
}


static void anna_init()
{
    hash_init(
	&anna_type_for_function_identifier,
	&hash_function_type_func,
	&hash_function_type_comp);
    anna_mid_init();
        
    stack_global = anna_stack_create(4096, 0);

/*
    anna_member_types_create(stack_global);
*/  
    
/*
    assert(anna_abides(int_type,object_type)==1);
    assert(anna_abides(list_type,object_type)==1);
    assert(anna_abides(object_type,int_type)==0);
    assert(anna_abides(int_type,int_type)==1);
*/  
    
}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
	debug(D_CRITICAL,L"Error: Expected at least one argument, a name of a file to run.\n");
	exit(ANNA_STATUS_ARGUMENT_ERROR);
    }
    
    wchar_t *module_name = str2wcs(argv[1]);
    
    debug(D_SPAM,L"Initializing interpreter...\n");    
    anna_init();
    anna_module_load(L"lang");
    
    if(anna_error_count)
    {
	debug(D_CRITICAL,L"Found %d error(s) during initialization, exiting\n", anna_error_count);
	exit(1);
    }
    
    null_object = anna_object_create_raw(0);    
    null_object->type = null_type;
    anna_int_one = anna_int_create(1);
    
    anna_stack_template_t *module = anna_stack_unwrap(anna_module_load(module_name));
    
    anna_object_t **main_wrapper_ptr = anna_stack_addr_get_str(module, L"main");
    if(!main_wrapper_ptr)
    {
	debug(D_CRITICAL,L"No main method defined in module %ls\n", module_name);
	exit(1);	
    }
    
    debug(D_SPAM,L"Program fully loaded and ready to be executed\n");    

    anna_function_t *fun = anna_function_unwrap(*main_wrapper_ptr);
    anna_vm_run(fun);
      
//    anna_function_wrapped_invoke(*main_wrapper_ptr, 0, 0, 0, stack_global);
    
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
