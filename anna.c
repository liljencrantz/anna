#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna.h"
#include "anna_node.h"
#include "anna_stack.h"
#include "anna_int.h"
#include "anna_float.h"
#include "anna_string.h"
#include "anna_char.h"
#include "anna_list.h"
#include "anna_type.h"
#include "anna_type_type.h"
#include "anna_node.h"
#include "anna_function.h"
#include "anna_prepare.h"
#include "anna_node_wrapper.h"
#include "anna_macro.h"
#include "anna_util.h"
#include "anna_member.h"
#include "anna_module.h"
#include "anna_function_type.h"

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
static hash_table_t anna_mid_identifier;
static array_list_t anna_mid_identifier_reverse;

anna_node_t *anna_node_null=0;

anna_stack_frame_t *stack_global;
static size_t mid_pos = ANNA_MID_FIRST_UNRESERVED;

int anna_error_count=0;


anna_object_t *anna_i_function_wrapper_call(anna_node_call_t *node, anna_stack_frame_t *stack);


void anna_native_declare(
    anna_stack_frame_t *stack,
    wchar_t *name,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc, 
    anna_type_t **argv,
    wchar_t **argn)
{
    anna_native_create(name, flags, func, result, argc, argv, argn, stack_global);
}


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

void anna_mid_put(wchar_t *name, size_t mid)
{
    size_t *offset_ptr = hash_get(&anna_mid_identifier, name);
    if(offset_ptr)
    {
	wprintf(L"Tried to reassign mid!\n");
	exit(1);
    }
   
    offset_ptr = malloc(sizeof(size_t));
    *offset_ptr = mid;
    hash_put(&anna_mid_identifier, name, offset_ptr);   
    al_set(&anna_mid_identifier_reverse, mid, wcsdup(name));
}

size_t anna_mid_get(wchar_t *name)
{
    size_t *offset_ptr = hash_get(&anna_mid_identifier, name);
    if(!offset_ptr)
    {      
	size_t gg = mid_pos++;
	anna_mid_put(name, gg);
	return gg;
    }
    return *offset_ptr;
}

wchar_t *anna_mid_get_reverse(size_t mid)
{
    return (wchar_t *)al_get(&anna_mid_identifier_reverse, mid);
}

int hash_function_type_func(void *a)
{
    anna_function_type_key_t *key = (anna_function_type_key_t *)a;
    int res = (int)key->result + key->is_variadic;
    int i;
    for(i=0;i<key->argc; i++)
    {
	res = (res<<19) ^ (int)key->argv[i] ^ (res>>13);
	res += wcslen(key->argn[i]);
    }
    return res;
}

int hash_function_type_comp(void *a, void *b)
{
    int i;
    
    anna_function_type_key_t *key1 = (anna_function_type_key_t *)a;
    anna_function_type_key_t *key2 = (anna_function_type_key_t *)b;

    //wprintf(L"Compare type for function %ls %d and %ls %d\n", key1->result->name, hash_function_type_func(key1), key2->result->name, hash_function_type_func(key2));

    if(key1->result != key2->result)
	return 0;
    if(key1->argc != key2->argc)
	return 0;
    if(key1->is_variadic != key2->is_variadic)
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
    int is_variadic)
{
    //static int count=0;
    //if((count++)==10) {CRASH};
    
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
    
    key->is_variadic = is_variadic;
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
					    anna_node_call_t *param,
					    anna_stack_frame_t *local)
{
    //wprintf(L"Wrapped invoke of function %ls\n", obj->type->name);
    if(obj == null_object)
	return null_object;
    
    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(obj, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(obj, ANNA_MID_FUNCTION_WRAPPER_STACK);
    if(function_ptr) 
    {
	if(stack_ptr)
	{
//	    wprintf(L"Invoking wrapped function %ls with parent stack\n", (*function_ptr)->name);
//	    anna_stack_print_trace(stack_ptr);
	}	
        return anna_function_invoke(*function_ptr, this, param, local, stack_ptr?*stack_ptr:stack_global);
    }
    else 
    {
	anna_object_t **function_wrapper_ptr = anna_static_member_addr_get_mid(obj->type, ANNA_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    return anna_function_wrapped_invoke(*function_wrapper_ptr,obj, param, stack_ptr?*stack_ptr:stack_global);
	}
	
	wprintf(L"Critical: Tried to call a non-function\n");
	anna_object_print(obj);
	
	CRASH;
    }
    return 0;
}



/*
  anna_object_t *anna_call(anna_stack_frame_t *stack, anna_object_t *obj)
  {
  anna_object *call = *anna_get_member_addr_int(obj, ANNA_MID_CALL);
  anna_node *node = (anna_node *)*anna_get_member_addr_int(obj, ANNA_MID_NODE);
  return node->invoke(node, stack);
  }
*/

anna_object_t *anna_construct(
    anna_type_t *type,
    struct anna_node_call *param,
    anna_stack_frame_t *stack)
{
    anna_object_t *result = anna_object_create(type);
    //wprintf(L"Creating new object of type %ls\n", type->name);
    assert(type->name);
    
    anna_object_t **constructor_ptr = anna_member_addr_get_mid(result, ANNA_MID_INIT_PAYLOAD);
    if(constructor_ptr)
    {
        anna_function_t *constructor = anna_function_unwrap(*constructor_ptr);
	//wprintf(L"First param is %ls type\n", param[1]->type->name);
	anna_function_invoke(constructor, result, param, stack, 0);
    }
    
    /*
      FIXME: Constructor goes here!
    */
    
    return result;
}


/*
  anna_object_t *anna_i_get(anna_node_identifier_t *this, anna_stack_frame_t *stack)
  {
  wchar_t *name = this->child[0]->name;
  return anna_stack_get(stack, name);
  }

  anna_object_t *anna_i_set(anna_node_t *this, anna_stack_frame_t *stack)
  {
  wchar_t *name = this->child[0]->name;
  anna_object_t *value = this->children[1]->invoke(this->children[1], stack);
  anna_stack_set(stack, name, value);
  return value;
  }
*/

/*
  anna_object_t *anna_i_create_function(anna_node_call_t *this, anna_stack_frame_t *stack)
  {
  wchar_t *name = this->child[0]->name;
  anna_object_t *value = this->children[1]->invoke(this->children[1], stack);
  anna_stack_set(stack, name, value);
  
  //  FIXME: Not done yet...
  return value;
  }
*/

anna_object_t *anna_method_wrap(anna_object_t *method, anna_object_t *owner)
{
    anna_function_t *function_original = anna_function_unwrap(method);
    size_t func_size = sizeof(anna_function_t);
    anna_function_t *function_copy = malloc(func_size);

    memcpy(function_copy, function_original, func_size);
    function_copy->this = owner;
    function_copy->wrapper = anna_object_create(function_copy->type);
    memcpy(anna_member_addr_get_mid(anna_function_wrap(function_copy),ANNA_MID_FUNCTION_WRAPPER_PAYLOAD), 
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

anna_object_t *anna_i_member_call(anna_node_call_t *param, anna_stack_frame_t *stack)
{
/*
  wprintf(L"member_get on node at %d\n", param);
  anna_node_print((anna_node_t *)param);
  wprintf(L"\n");
*/  
    anna_object_t *obj = anna_node_invoke(param->child[0], stack);
    anna_node_identifier_t *name_node = node_cast_identifier(param->child[1]);
    //   wprintf(L"name node is %d, name is %ls\n", name_node, name_node->name);
    anna_object_t *member = *anna_member_addr_get_str(obj, name_node->name);
    
    if(member == null_object){
	return member;
    }
    
    return anna_function_wrapped_invoke(member, 0, param, stack);    
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

/*
  anna_object_t *anna_i_member_call(anna_node_t *this, anna_stack_frame_t *stack)
  {
  anna_object_t *obj = anna_i_get(this, stack);
  wchar_t *name = this->child[1]->name;
  anna_object_t *method = *anna_member_addr_get_str(obj, name);
  
  }
*/

/*
  anna_object_t *anna_i_const_string(anna_node_t *this, anna_stack_frame_t *stack)
  {
  anna_node_string_literal_t *this2 = node_cast_string_literal(this);
  return anna_create_string(this2->payload_size, this2->payload);
  }
*/
anna_object_t *anna_i_const_int(anna_node_t *this, anna_stack_frame_t *stack)
{
    anna_node_int_literal_t *this2 = node_cast_int_literal(this);
    anna_object_t *result = anna_int_create(this2->payload);
    return result;
}

/**
   This method is the best ever! All method calls on the null object run this
*/
anna_object_t *anna_i_null_function(anna_object_t **node_base)
{
    return null_object;
}

anna_object_t *anna_object_create(anna_type_t *type) {
    anna_object_t *result = 
	calloc(
	    1,
	    sizeof(anna_object_t)+sizeof(anna_object_t *)*type->member_count);
    result->type = type;
    int i;
    for(i=0; i<type->member_count; i++)
    {
	result->member[i]=null_object;
    }
      
    return result;
}

anna_object_t *anna_object_create_raw(size_t sz)
{
    anna_object_t *result = 
	calloc(
	    1,
	    sizeof(anna_object_t)+sizeof(anna_object_t *)*sz);
    int i;
    for(i=0; i<sz; i++)
    {
	result->member[i]=null_object;
    }
    
    return result;
}

void anna_member_redeclare(
    anna_type_t *type,
    ssize_t mid,
    anna_type_t *member_type)
{
    type->mid_identifier[mid]->type = member_type;
}


size_t anna_member_create(anna_type_t *type,
			  ssize_t mid,
			  wchar_t *name,
			  int is_static,
			  anna_type_t *member_type)
{
/*
    if(!member_type)
    {
	wprintf(L"Critical: Create a member with unspecified type\n");
	CRASH;
    }
*/  
    if(hash_get(&type->name_identifier, name))
    {
	if(type == type_type && wcscmp(name, L"!typeWrapperPayload")==0)
	    return mid;
	if(mid == ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD ||
	   mid == ANNA_MID_FUNCTION_WRAPPER_PAYLOAD ||
	   mid == ANNA_MID_FUNCTION_WRAPPER_STACK ||
	   mid == ANNA_MID_STACK_TYPE_PAYLOAD)
	    return mid;
	
	wprintf(L"Critical: Redeclaring member %ls of type %ls\n",
		name, type->name);
	CRASH;	
    }
    
    anna_member_t * member = calloc(1,sizeof(anna_member_t) + sizeof(wchar_t) * (wcslen(name)+1));
    
    wcscpy(member->name, name);
    
    if (mid == (ssize_t)-1) {
	mid = anna_mid_get(name);
    }
    else 
    {
	if(mid != anna_mid_get(name))
	{
	    wprintf(L"Error, multiple mids for name %ls: %d and %d\n", name, mid, anna_mid_get(name));
	    CRASH;
	}
    }
    
    member->type = member_type;
    member->is_static = is_static;
    if(is_static) {
	member->offset = anna_type_static_member_allocate(type);
    } else {
	member->offset = type->member_count++;
    }
//  wprintf(L"Add member with mid %d\n", mid);
    
    type->mid_identifier[mid] = member;
    hash_put(&type->name_identifier, name, member);
    return mid;
}

void anna_member_add_node(anna_node_call_t *definition,
			  ssize_t mid,
			  wchar_t *name,
			  int is_static,
			  anna_node_t *member_type)
{
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_member_declare_create(
	    &definition->location,
	    mid,
	    name,
	    is_static,
	    member_type));
}


void anna_native_method_add_node(
    anna_node_call_t *definition,
    ssize_t mid,
    wchar_t *name,
    int flags,
    anna_native_t func,
    anna_node_t *result,
    size_t argc,
    anna_node_t **argv,
    wchar_t **argn)
{
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_native_method_declare_create(
	    &definition->location,
	    mid,
	    name,
	    flags,
	    func,
	    result,
	    argc,
	    argv,
	    argn));
}



size_t anna_native_method_create(
    anna_type_t *type,
    ssize_t mid,
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
	    flags & ANNA_FUNCTION_VARIADIC));
    anna_member_t *m = type->mid_identifier[mid];
    //wprintf(L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    m->is_method=1;
    type->static_member[m->offset] = 
	anna_function_wrap(
	    anna_native_create(
		name, flags, func, result, 
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
    mid = anna_member_create(type, mid, name, 1, definition->type);
    anna_member_t *m = type->mid_identifier[mid];
    m->is_method=1;
    
    //wprintf(L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    type->static_member[m->offset] = anna_function_wrap(definition);
    return (size_t)mid;
}



int hash_null_func( void *data )
{
    return 0;
}

int hash_null_cmp( void *a, 
		   void *b )
{
    return 1;
}

static void anna_null_type_create_early()
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

static void anna_object_type_create_early()
{
}

static void anna_init()
{
    al_init(&anna_mid_identifier_reverse);
    hash_init(&anna_mid_identifier, &hash_wcs_func, &hash_wcs_cmp);
    hash_init(&anna_type_for_function_identifier, &hash_function_type_func, &hash_function_type_comp);
    
    anna_mid_put(L"!typeWrapperPayload", ANNA_MID_TYPE_WRAPPER_PAYLOAD);
    anna_mid_put(L"!callPayload", ANNA_MID_CALL_PAYLOAD);
    anna_mid_put(L"!stringPayload", ANNA_MID_STRING_PAYLOAD);
    anna_mid_put(L"!stringPayloadSize", ANNA_MID_STRING_PAYLOAD_SIZE);
    anna_mid_put(L"!charPayload", ANNA_MID_CHAR_PAYLOAD);
    anna_mid_put(L"!intPayload", ANNA_MID_INT_PAYLOAD);
    anna_mid_put(L"!listPayload", ANNA_MID_LIST_PAYLOAD);
    anna_mid_put(L"!listSize", ANNA_MID_LIST_SIZE);
    anna_mid_put(L"!listCapacity", ANNA_MID_LIST_CAPACITY);
    anna_mid_put(L"!functionTypePayload", ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    anna_mid_put(L"!functionPayload", ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_mid_put(L"!floatPayload", ANNA_MID_FLOAT_PAYLOAD);
    anna_mid_put(L"!functionStack", ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_mid_put(L"__call__", ANNA_MID_CALL_PAYLOAD);    
    anna_mid_put(L"__init__", ANNA_MID_INIT_PAYLOAD);
    anna_mid_put(L"!nodePayload", ANNA_MID_NODE_PAYLOAD);
    anna_mid_put(L"!memberPayload", ANNA_MID_MEMBER_PAYLOAD);
    anna_mid_put(L"!memberTypePayload", ANNA_MID_MEMBER_TYPE_PAYLOAD);
    anna_mid_put(L"!stackPayload", ANNA_MID_STACK_PAYLOAD);
    anna_mid_put(L"!stackTypePayload", ANNA_MID_STACK_TYPE_PAYLOAD);
    anna_mid_put(L"from", ANNA_MID_FROM);
    anna_mid_put(L"to", ANNA_MID_TO);
    anna_mid_put(L"step", ANNA_MID_STEP);
    
    stack_global = anna_stack_create(4096, 0);
    /*
      Create lowest level stuff. Bits of magic, be careful with
      ordering here. A lot of intricate dependencies going on between
      the various calls...
    */
    
    anna_type_type_create(stack_global);
    object_type = anna_type_native_create(L"Object" ,stack_global);
    null_type = anna_type_native_create(L"Null", stack_global);
    
    anna_null_type_create_early();
    anna_object_type_create_early();
        
    anna_member_types_create(stack_global);
    anna_int_type_create(stack_global);
    anna_list_type_create(stack_global);
    anna_char_type_create(stack_global);
    anna_string_type_create(stack_global);
    anna_float_type_create(stack_global);
    anna_node_wrapper_types_create(stack_global);
    
    anna_function_implementation_init(stack_global);

    anna_macro_init(stack_global);

/*
    assert(anna_abides(int_type,object_type)==1);
    assert(anna_abides(list_type,object_type)==1);
    assert(anna_abides(object_type,int_type)==0);
    assert(anna_abides(int_type,int_type)==1);
*/  
    
}

void anna_print_member(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    wchar_t *key = (wchar_t *)key_ptr;
    anna_member_t *member = (anna_member_t *)val_ptr;
    //anna_object_t *obj = (anna_object_t *)aux_ptr;
    //anna_object_t *value = member->is_static?obj->type->static_member[member->offset]:obj->member[member->offset];    
    wprintf(L"  %ls: %ls\n", key, member->type?member->type->name:L"?");
}

void anna_object_print(anna_object_t *obj)
{
    wprintf(L"%ls:\n", obj->type->name);
    hash_foreach2(&obj->type->name_identifier, &anna_print_member, obj);
}

anna_object_t *anna_function_invoke_values(anna_function_t *function, 
					   anna_object_t *this,
					   anna_object_t **param,
					   anna_stack_frame_t *outer)
{
    if(function->flags & ANNA_FUNCTION_MACRO) 
    {
	wprintf(L"FATAL: Macro %ls at invoke!!!!\n", function->name);
	CRASH;
    }
    else
    {
	if(function->native.function) 
	{
	    anna_object_t **argv=param;
	    int i;
		
	    int offset=0;
	    if(this)
	    {
		argv=malloc(sizeof(anna_object_t *)*function->input_count);
		offset=1;
		argv[0]=this;		    
		for(i=0; i<(function->input_count-offset); i++) 
		{
		    argv[i+offset]=param[i];
		}
	    }
	    //wprintf(L"Invoking function with %d params\n", function->input_count);
	    
	    return function->native.function(argv);
	}
	else 
	{
	    anna_object_t *result = null_object;
	    int i;
	    anna_stack_frame_t *my_stack = anna_stack_clone(function->stack_template);//function->input_count+1);
	    my_stack->parent = outer?outer:stack_global;
	    /*
	    wprintf(L"Create new stack %d with frame depth %d while calling function %ls\n", my_stack, anna_stack_depth(my_stack), function->name);
	    wprintf(L"Run non-native function %ls with %d params and definition:\n", function->name, function->input_count);
	    anna_node_print(function->body);
	    */
	    int offset=0;
	    if(this)
	    {
		offset=1;
		anna_stack_declare(my_stack, 
				   function->input_name[0],
				   function->input_type[0],
				   this);
	    }
		
	    for(i=0; i<(function->input_count-offset); i++) 
	    {
/*
  wprintf(L"Declare input variable %d with name %ls on stack\n",
  i, function->input_name[i+offset]);
*/
		anna_stack_set_str(my_stack, 
				   function->input_name[i+offset],
				   param[i]);
	    }
	    
	    for(i=0; i<function->body->child_count && !my_stack->stop; i++)
	    {
		/*
		wprintf(L"Run node %d of function %ls\n",
			i, function->name);
	      
		anna_node_print(function->body->child[i]);
		*/
		result = anna_node_invoke(function->body->child[i], my_stack);
		//wprintf(L"Result is %d\n", result);
	      
	    }
	    /*
	      if(my_stack->stop) 
	      {
	      }
	    */
	    return result;
	}
    }
}

struct anna_node *anna_macro_invoke(
    anna_function_t *macro, 
    anna_node_call_t *node,
    anna_function_t *function,
    anna_node_list_t *parent)
{
    if(macro->native.macro)
    {
	return macro->native.macro(
	    node, function, parent);
    }
    else
    {
	int i;
	anna_stack_frame_t *my_stack = anna_stack_clone(macro->stack_template);
	anna_object_t *result = null_object;
	
	anna_stack_set_str(my_stack,
			   macro->input_name[0],
			   anna_node_wrap((anna_node_t *)node));
	for(i=0; i<macro->body->child_count && !my_stack->stop; i++)
	{
	    result = anna_node_invoke(macro->body->child[i], my_stack);
	}
	if(result == null_object)
	{
	    return anna_node_null_create(&node->location);
	}
	return anna_node_unwrap(result);
    }
}

anna_object_t *anna_function_invoke(anna_function_t *function, 
				    anna_object_t *this,
				    anna_node_call_t *param, 
				    anna_stack_frame_t *param_invoke_stack,
				    anna_stack_frame_t *outer) 
{
/*
    wprintf(L"Executing function %ls\n", function->name);
    if(function->body)
    {
	anna_node_print(function->body);
    }
*/  
    if(!this)
    {
	this=function->this;
    }
    
    anna_object_t **argv;
    //wprintf(L"anna_function_invoke %ls %d; %d params\n", function->name, function, function->input_count);    
    if(likely(function->input_count < 8))
    {
	anna_object_t *argv_c[8];
	argv=argv_c;
    }
    else
    {
	argv=malloc(sizeof(anna_object_t *)*function->input_count);
    }
    
    int i;
    //wprintf(L"Argv: %d\n", argv);
    
    int offset=0;
    if(this)
    {	    
	offset=1;
	argv[0]=this;		    
	//wprintf(L"We have a this parameter: %d\n", this);
    }
    int is_variadic = ANNA_IS_VARIADIC(function);
    //  wprintf(L"Function %ls has variadic flag set to %d\n", function->name, function->flags);
    
    for(i=0; i<(function->input_count-offset-is_variadic); i++)
    {
//wprintf(L"eval param %d of %d \n", i, function->input_count - is_variadic - offset);
	argv[i+offset]=anna_node_invoke(param->child[i], param_invoke_stack);
    }

    if(is_variadic)
    {
	anna_object_t *lst = anna_list_create();
	int variadic_count = param->child_count - i;
	if(variadic_count < 0)
	{
	    anna_error(
		(anna_node_t *)param,
		L"Critical: Tried to call function %ls with %d arguments, need at least %d\n",
		function->name,
		param->child_count,
		function->input_count-1+offset);
	    CRASH;	    
	}
	
	anna_list_set_capacity(lst, variadic_count);
	for(; i<param->child_count;i++)
	{
	    //  wprintf(L"eval variadic param %d of %d \n", i, param->child_count);
	    anna_list_add(lst, anna_node_invoke(param->child[i], param_invoke_stack));
	}
	//wprintf(L"Set variadic var at offset %d to %d\n", function->input_count-1, lst);
	
	argv[function->input_count-1] = lst;	    
    }
    if(!likely(function->input_count < 8))
    {
	free(argv);
    }
    return anna_function_invoke_values(function, 0, argv, outer);
    
}

void anna_error(anna_node_t *node, wchar_t *msg, ...)
{
    va_list va;
    va_start( va, msg );	
    fwprintf(stderr,L"Error in %ls, on line %d:\n", 
	     node->location.filename?node->location.filename:L"<internal>",
	     node->location.first_line);
    anna_node_print_code(node);
    fwprintf(stderr,L"\n");
    
    vfwprintf(stderr, msg, va);
    va_end( va );
    fwprintf(stderr, L"\n\n");
    anna_error_count++;
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
    anna_module_load(L"lang");
    null_object->type = null_type;
        
    anna_int_one = anna_int_create(1);

    anna_function_t *module = anna_module_load(module_name);

//    wprintf(L"Validated program:\n");    
    anna_object_t **main_wrapper_ptr = anna_stack_addr_get_str(module->stack_template, L"main");
    if(!main_wrapper_ptr)
    {
	wprintf(L"No main method defined in module %ls\n", module_name);
	exit(1);	
    }


    anna_function_t *main_func = anna_function_unwrap(*main_wrapper_ptr);
    if(!main_func)
    {
	wprintf(L"Main is not a method in module %ls\n", module_name);
	exit(1);	
    }

    anna_function_invoke(module, 0, 0, stack_global, stack_global);
    
    
    wprintf(L"Output:\n");        
//    anna_function_invoke(main_func, 0, 0, stack_global, stack_global);
    anna_function_invoke(main_func, 0, 0, stack_global, module->stack_template);
    
    wprintf(L"\n");
}
