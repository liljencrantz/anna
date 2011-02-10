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
#include "anna_node.h"
#include "anna_module.h"
#include "anna_int.h"
#include "anna_float.h"
#include "anna_string.h"
#include "anna_char.h"
#include "anna_list.h"
#include "anna_function_type.h"
#include "anna_type.h"
#include "anna_node_create.h"
#include "anna_type_type.h"
#include "anna_macro.h"
#include "anna_member.h"
#include "anna_node_wrapper.h"

anna_type_t *type_type=0, 
    *object_type=0,
    *int_type=0, 
    *string_type=0,
    *char_type=0, 
    *null_type=0,
    *string_type, 
    *char_type,
    *list_type,
    *float_type,
    *member_type;
anna_object_t *null_object=0;

static hash_table_t anna_type_for_function_identifier;

anna_node_t *anna_node_null=0;

anna_stack_frame_t *stack_global;

anna_object_t *anna_i_function_wrapper_call(anna_node_call_t *node, anna_stack_frame_t *stack);

anna_object_t **anna_member_addr_get_str(
    anna_object_t *obj,
    wchar_t *name)
{
    anna_member_t *m = (anna_member_t *)hash_get(&(obj->type->name_identifier), name);
    /*
      wprintf(L"Woo, get address of member %ls on object\n", name);
      wprintf(L"of type %ls\n", obj->type->name);
    */
    if(!m) 
    {
	wprintf(L"ERROR!!! Object %d of type %ls does not have a member %ls\n",obj, obj->type->name, name);
	exit(1);	
    }
    if(m->is_static) {
	return &obj->type->static_member[m->offset];
    } else {
	return &(obj->member[m->offset]);
    }
}

anna_object_t **anna_member_addr_get_mid(anna_object_t *obj, size_t mid)
{
    /*
      wprintf(L"Get mid %d on object\n", mid);
      wprintf(L"of type %ls\n", obj->type->name);
    */
    anna_member_t *m = obj->type->mid_identifier[mid];
    if(!m) 
    {
	return 0;
    }
    //wprintf(L"Found! Pos is %d, static is %d\n", m->offset, m->is_static);
    
    //wprintf(L"Lala, get addr of member %ls\n", m->name);    
    if(m->is_static) {
	return &obj->type->static_member[m->offset];
    } else {
	return &(obj->member[m->offset]);
    }
}

anna_object_t **anna_static_member_addr_get_mid(anna_type_t *type, size_t mid)
{
    /*  wprintf(L"Get mid %d on object\n", mid);
	wprintf(L"of type %ls\n", obj->type->name);
    */
    anna_member_t *m = type->mid_identifier[mid];
    if(!m) 
    {
	return 0;
    }
    
    //wprintf(L"Lala, get addr of member %ls\n", m->name);    
    if(m->is_static) {
	return &type->static_member[m->offset];
    } else {
	return 0;
    }
}

static int hash_function_type_func(void *a)
{
    anna_function_type_key_t *key = (anna_function_type_key_t *)a;
    int res = (int)key->result ^ key->flags;
    int i;
    
    for(i=0;i<key->argc; i++)
    {
	res = (res<<19) ^ (int)key->argv[i] ^ (res>>13);
	res ^= wcslen(key->argn[i]);
    }
    
    return res;
}

static int hash_function_type_comp(void *a, void *b)
{
    int i;
    
    anna_function_type_key_t *key1 = (anna_function_type_key_t *)a;
    anna_function_type_key_t *key2 = (anna_function_type_key_t *)b;

    //wprintf(L"Compare type for function %ls %d and %ls %d\n", key1->result->name, hash_function_type_func(key1), key2->result->name, hash_function_type_func(key2));

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
    //wprintf(L"Same!\n");
    
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
    int i;
    static anna_function_type_key_t *key = 0;
    static size_t key_sz = 0;
    size_t new_key_sz = sizeof(anna_function_type_key_t) + sizeof(anna_type_t *)*argc;
    
    if(!result)
    {
	wprintf(
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
	    wprintf(
		L"Critical: Tried to get a function key for function with uninitialized argument types\n");
	    CRASH;
	}
	
	key->argv[i]=argv[i];
	key->argn[i]=argn[i];
    }
/*
    wprintf(L"Weee %ls <-", result->name);
    for(i=0;i<argc; i++)
    {
	wprintf(L" %ls", argv[i]->name);	
    }
    wprintf(L"\n");
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
	res = anna_function_type_create(new_key);
	hash_put(&anna_type_for_function_identifier, new_key, res);
    }

    anna_function_type_key_t *ggg = anna_function_unwrap_type(res);
    assert(ggg->argc == argc);
    
    return res;
}


anna_object_t *anna_function_wrapped_invoke(anna_object_t *obj, 
					    anna_object_t *this,
					    size_t param_count,
					    anna_node_t **param,
					    anna_stack_frame_t *local)
{
//    wprintf(L"Wrapped invoke of function %ls\n", obj->type->name);
    if(obj == null_object)
	return null_object;
    
    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(obj, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(obj, ANNA_MID_FUNCTION_WRAPPER_STACK);
    if(function_ptr) 
    {
	if(stack_ptr)
	{
//	    wprintf(L"Invoking wrapped function %ls with parent stack\n", (*function_ptr)->name);
//	    anna_stack_print(*stack_ptr);
	}
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
	
	wprintf(L"Critical: Tried to call a non-function\n");
	anna_object_print(obj);
	
	CRASH;
    }
    return 0;
}

anna_object_t *anna_construct(
    anna_type_t *type,
    struct anna_node_call *param,
    anna_stack_frame_t *stack)
{
    anna_object_t *result = anna_object_create(type);
    //wprintf(L"Creating new object of type %ls\n", type->name);
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
	sizeof(anna_stack_frame_t *));
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
    int i;
    int res = 0;    

    assert(contender);
    assert(role_model);
    
//    wprintf(L"Check type %ls abides against %ls\n", contender->name, role_model->name);
    
    //wprintf(L"Role model %ls has %d members\n", role_model->name, role_model->member_count+role_model->static_member_count);
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
    ssize_t mid,
    anna_type_t *member_type)
{
    type->mid_identifier[mid]->type = member_type;
}


size_t anna_native_method_create(
    anna_type_t *type,
    ssize_t mid,
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
	assert(result);
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
    //wprintf(L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    m->is_method=1;
    type->static_member[m->offset] = 
	anna_function_wrap(
	    anna_native_create(
		name, flags, (anna_native_t)func, result, 
		argc, argv, argn,
		0));
    return (size_t)mid;
}

size_t anna_method_create(anna_type_t *type,
			  ssize_t mid,
			  wchar_t *name,
			  int flags,
			  anna_function_t *definition)		
{
    mid = anna_member_create(type, mid, name, 1, definition->wrapper->type);
    anna_member_t *m = type->mid_identifier[mid];
    m->is_method=1;
    
    //wprintf(L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    type->static_member[m->offset] = anna_function_wrap(definition);
    return (size_t)mid;
}

/**
   This method is the best ever! All method calls on the null object run this
*/
anna_object_t *anna_i_null_function(anna_object_t **node_base)
{
    return null_object;
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

static void anna_init()
{
    hash_init(
	&anna_type_for_function_identifier,
	&hash_function_type_func,
	&hash_function_type_comp);
    anna_mid_init();
        
    stack_global = anna_stack_create(4096, 0);
    /*
      Create lowest level stuff. Bits of magic, be careful with
      ordering here. A lot of intricate dependencies going on between
      the various calls...
    */
    
    type_type = 
	anna_type_native_create(
	    L"Type", 
	    stack_global );
    object_type = anna_type_native_create(L"Object" ,stack_global);
    null_type = anna_type_native_create(L"Null", stack_global);
    int_type = 
	anna_type_native_create(
	    L"Int", 
	    stack_global );

    list_type = 
	anna_type_native_create(
	    L"List", 
	    stack_global );

    string_type = 
	anna_type_native_create(
	    L"String", 
	    stack_global );

    float_type = anna_type_native_create(L"Float", stack_global);
    char_type = anna_type_native_create(L"Char", stack_global);

    anna_type_type_create(stack_global);    
    anna_null_type_create();    
    anna_int_type_create(stack_global);
    anna_list_type_create(stack_global);
    anna_string_type_create(stack_global);
    anna_node_create_wrapper_types(stack_global);
    
    anna_stack_declare(stack_global, L"Type", type_type, anna_type_wrap(type_type), 0); 
    anna_stack_declare(stack_global, L"Int", type_type, anna_type_wrap(int_type), 0);       anna_stack_declare(stack_global, L"Object", type_type, anna_type_wrap(object_type), 0); 
    anna_stack_declare(stack_global, L"Null", type_type, anna_type_wrap(null_type), 0); 

    anna_stack_declare(stack_global, L"List", list_type, anna_type_wrap(list_type), 0); 
    anna_stack_declare(stack_global, L"String", string_type, anna_type_wrap(string_type), 0); 


    anna_char_type_create(stack_global);
    anna_float_type_create(stack_global);

/*
    anna_member_types_create(stack_global);
*/  
    
    anna_function_implementation_init(stack_global);
    anna_macro_init(stack_global);

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
	wprintf(L"Error: Expected at least one argument, a name of a file to run.\n");
	exit(1);
    }
    
    wchar_t *module_name = str2wcs(argv[1]);
    
    wprintf(L"Initializing interpreter...\n");    
    anna_init();
    
    if(anna_error_count)
    {
	wprintf(L"Found %d error(s) during initialization, exiting\n", anna_error_count);
	exit(1);
    }
    
    null_object = anna_object_create_raw(0);    
    null_object->type = null_type;
    anna_int_one = anna_int_create(1);
    
    //  anna_module_load(L"lang");
    
    anna_stack_frame_t *module = anna_module_load(module_name);
    
//    wprintf(L"Validated program:\n");    
    anna_object_t **main_wrapper_ptr = anna_stack_addr_get_str(module, L"main");
    if(!main_wrapper_ptr)
    {
	wprintf(L"No main method defined in module %ls\n", module_name);
	exit(1);	
    }

    anna_function_wrapped_invoke(*main_wrapper_ptr, 0, 0, 0, stack_global);
    
/*    
    anna_function_t *main_func = anna_function_unwrap(*main_wrapper_ptr);
    if(!main_func)
    {
	wprintf(L"\"main\" member of module \"%ls\" is not a function\n", module_name);
	exit(1);	
    }
    
    wprintf(L"Output:\n");        
    anna_function_invoke(main_func, 0, 0, module);
    wprintf(L"\n");
*/
}
