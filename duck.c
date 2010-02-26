#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "duck.h"
#include "duck_node.h"
#include "duck_stack.h"
#include "duck_int.h"
#include "duck_float.h"
#include "duck_string.h"
#include "duck_char.h"
#include "duck_list.h"

/*
  List type
  ComparisonMap type
  HashMap type
  Range type
  Pair type

  Test null handling a bit better
    
  Simple type checking
  Complex type checking
  Type support for lists
  Type checking
  General purpose currying
  Namespaces
  Method macros
  Correct string literal parsing
  
  Implement basic string methods
  Implement basic list methods
  Implement string comparison methods
  Implement basic char methods
  Implement char comparison methods
  
  Function default argument values
  Named function arguments
  Inner functions with shared return flag, etc.
  Variadic functions
  Garbage collection
  
  Real check if type abides to other type, instead of lame type ptr comparison
  Create intersection of two types
  
  __macro__ macro
  elif macro
  __type__ function
  each function
  extends function
  abides function
  __add__, __sub__, __mul__ and friends: full abides/reverse checking implementation
  __return__ macro
  __returnAssign__ macro
  __templatize__ macro
  template macro
  __list__ macro
  while macro
  __while__ function
  __or__ macro
  __and__ macro
  
  Done: 
  
  Sugar parser
  
  Variable declarations
  Function argument passing
  Method calls with proper this handling
  Inner functions with access to outer scope
  Make ; after } optional
  Constructors with no parameters
  
  Type type
  Call type
  Int type
  String type
  Char type
  Null type  
  
  Represent objects 
  Represent types
  Represent member data
  Represent the stack
  
  Parse int literals
  Parse lookups
  Parse calls
  Parse string literals
  Parse chars literals
  Parse float literals
  
  Implement basic int methods
  Implement int comparison methods
  
  __block__ function
  __assign__ function
  __declare__ function
  __function__ function  
  __if__ function
  __add__, __sub__, __mul__ and friends, simple exact mapping
  if macro
  else macro

*/

duck_type_t *type_type=0, *object_type=0, *int_type=0, *string_type=0, *char_type=0, *null_type=0,  *string_type, *char_type, *list_type, *float_type;
duck_object_t *null_object=0;

static hash_table_t duck_type_for_function_lookup;
static hash_table_t duck_mid_lookup;

duck_node_t *duck_node_null=0;

static duck_stack_frame_t *stack_global;
static size_t mid_pos = DUCK_MID_FIRST_UNRESERVED;

static duck_member_t **duck_mid_lookup_create();
duck_function_t *duck_native_create(wchar_t *name,
				    int flags,
				    duck_native_t native, 
				    duck_type_t *return_type,
				    size_t argc,
				    duck_type_t **argv,
				    wchar_t **argn);

duck_object_t *duck_function_invoke(duck_function_t *function, duck_node_call_t *param, duck_stack_frame_t *stack, duck_stack_frame_t *outer);
static duck_type_t *duck_type_create_raw(wchar_t *name, size_t static_member_count);
static void duck_type_wrapper_create(duck_type_t *result, int creatable);

duck_object_t *duck_i_function_wrapper_call(duck_node_call_t *node, duck_stack_frame_t *stack);
void duck_object_print(duck_object_t *obj, int level);

void duck_native_declare(duck_stack_frame_t *stack,
			 wchar_t *name,
			 int flags,
			 duck_native_t func,
			 duck_type_t *result,
			 size_t argc, 
			 duck_type_t **argv,
			 wchar_t **argn)
{
  duck_function_t *f = duck_native_create(name, flags, func, result, argc, argv, argn);
  duck_stack_declare(stack, name, f->type, f->wrapper);
}

duck_object_t **duck_member_addr_get_str(duck_object_t *obj, wchar_t *name)
{
    duck_member_t *m = (duck_member_t *)hash_get(&(obj->type->name_lookup), name);
    //    wprintf(L"Woo, get address of member %ls: %d\n", name, m);
    
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

duck_object_t **duck_member_addr_get_mid(duck_object_t *obj, size_t mid)
{
   /*  wprintf(L"Get mid %d on object\n", mid);
       wprintf(L"of type %ls\n", obj->type->name);
   */
    duck_member_t *m = obj->type->mid_lookup[mid];
    if(!m) 
    {
	return 0;
    }
    
    //wprintf(L"Lala, get addr of member %ls\n", m->name);    
    if(m->is_static) {
	return &obj->type->static_member[m->offset];
    } else {
	return &(obj->member[m->offset]);
    }
}

duck_object_t **duck_static_member_addr_get_mid(duck_type_t *type, size_t mid)
{
  /*  wprintf(L"Get mid %d on object\n", mid);
  wprintf(L"of type %ls\n", obj->type->name);
  */
    duck_member_t *m = type->mid_lookup[mid];
    if(!m) 
    {
	return 0;
    }
    
    //wprintf(L"Lala, get addr of member %ls\n", m->name);    
    if(m->is_static) {
	return &type->static_member[m->offset];
    } else {
	wprintf(L"Tried to acces non-static element with mid %d of type %ls as a static member\n", mid, type->name);
	exit(1);
    }
}

void duck_mid_put(wchar_t *name, size_t mid)
{
   size_t *offset_ptr = hash_get(&duck_mid_lookup, name);
   if(offset_ptr)
   {
      wprintf(L"Tried to reassign mid!\n");
      exit(1);
   }
   
   offset_ptr = malloc(sizeof(size_t));
   *offset_ptr = mid;
   hash_put(&duck_mid_lookup, name, offset_ptr);   
}

size_t duck_mid_get(wchar_t *name)
{
   size_t *offset_ptr = hash_get(&duck_mid_lookup, name);
   if(!offset_ptr)
   {      
      size_t gg = mid_pos++;
      duck_mid_put(name, gg);
      return gg;
   }
   return *offset_ptr;
}

int hash_function_type_func(void *a)
{
    duck_function_type_key_t *key = (duck_function_type_key_t *)a;
    int res = (int)key->result;
    int i;
    for(i=0;i<key->argc; i++)
    {
	res = (res<<19) ^ (int)key->argv[i] ^ (res>>13);
    }
    return res;
}

int hash_function_type_comp(void *a, void *b)
{
    int i;
    
    duck_function_type_key_t *key1 = (duck_function_type_key_t *)a;
    duck_function_type_key_t *key2 = (duck_function_type_key_t *)b;

    //wprintf(L"Compare type for function %ls %d and %ls %d\n", key1->result->name, hash_function_type_func(key1), key2->result->name, hash_function_type_func(key2));

    if(key1->result != key2->result)
	return 0;
    if(key1->argc != key2->argc)
	return 0;

    for(i=0;i<key1->argc; i++)
    {
	if(key1->argv[i] != key2->argv[i])
	    return 0;
    }
    //wprintf(L"Same!\n");
    
    return 1;
}

duck_type_t *duck_type_for_function(duck_type_t *result, size_t argc, duck_type_t **argv)
{
    //  static int count=0;
    //if((count++)==10) {CRASH};
    
    int i;
    static duck_function_type_key_t *key = 0;
    static size_t key_sz = 0;
    size_t new_key_sz = sizeof(duck_function_type_key_t) + sizeof(duck_type_t *)*argc;
    
    if(argc)
	assert(argv);
    
    if(new_key_sz>key_sz)
    {
	key = realloc(key, new_key_sz);
	key_sz = new_key_sz;
    }
    key->result=result;
    key->argc = argc;
    for(i=0; i<argc;i++)
    {
	key->argv[i]=argv[i];
    }
    
    duck_type_t *res = hash_get(&duck_type_for_function_lookup, key);
    if(!res)
    {
	duck_function_type_key_t *new_key = malloc(new_key_sz);
	memcpy(new_key, key, new_key_sz);	
	res = duck_type_create_raw(L"!FunctionType", 64);	
	hash_put(&duck_type_for_function_lookup, new_key, res);
	duck_member_create(res, DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD, L"!functionTypePayload",
			   1, null_type);
	
	duck_member_create(res, DUCK_MID_FUNCTION_WRAPPER_PAYLOAD, L"!functionPayload", 
			   0, null_type);
	duck_member_create(res, DUCK_MID_FUNCTION_WRAPPER_STACK, L"!functionStack", 
			   0, null_type);
	duck_type_wrapper_create(res, 0);
	
	(*duck_static_member_addr_get_mid(res, DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD)) = (duck_object_t *)new_key;
    }
    else 
    {
	//wprintf(L"YAY\n");
    }
    
    return res;
}


duck_function_t *duck_function_unwrap(duck_object_t *obj)
{
    assert(obj);
    
    duck_function_t **function_ptr = (duck_function_t **)duck_member_addr_get_mid(obj, DUCK_MID_FUNCTION_WRAPPER_PAYLOAD);
    if(function_ptr) 
    {
//	    wprintf(L"Got __call__ member\n");
	return *function_ptr;
    }
    else 
    {
	duck_object_t **function_wrapper_ptr = duck_static_member_addr_get_mid(obj->type, DUCK_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    return duck_function_unwrap(*function_wrapper_ptr);	    
	}
	
	wprintf(L"DANG!!!!\n");
	duck_object_print(obj, 1);
	
	CRASH;
    }
//     FIXME: Is there any validity checking we could do here?
  
}

duck_object_t *duck_function_wrapped_invoke(duck_object_t *obj, duck_node_call_t *param, duck_stack_frame_t *local)
{
    duck_function_t **function_ptr = (duck_function_t **)duck_member_addr_get_mid(obj, DUCK_MID_FUNCTION_WRAPPER_PAYLOAD);
    if(function_ptr) 
    {
	duck_stack_frame_t **stack_ptr = (duck_stack_frame_t **)duck_member_addr_get_mid(obj, DUCK_MID_FUNCTION_WRAPPER_STACK);
	return duck_function_invoke(*function_ptr, param, local, *stack_ptr);
    }
    else 
    {
	/*
	  FIXME: On __call__ pseudo-function gibberish, we create a
	  duck_function_t copy and set the this pointer. That's slow
	  and silly, figure out something more efficient.
	 */
	duck_object_t **function_wrapper_ptr = duck_static_member_addr_get_mid(obj->type, DUCK_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    return duck_function_wrapped_invoke(duck_method_wrap(*function_wrapper_ptr,obj), param, local);
	}
	
	wprintf(L"GOSH!!!");
	CRASH;
	
    }
//     FIXME: Is there any validity checking we could do here?
  
}



static int duck_is_member_get(duck_node_t *node)
{
    if(node->node_type != DUCK_NODE_LOOKUP)
	return 0;
    duck_node_lookup_t *node2 = (duck_node_lookup_t *)node;
    return wcscmp(node2->name, L"__memberGet__")==0;
    
}


duck_type_t *duck_type_member_type_get(duck_type_t *type, wchar_t *name)
{
   duck_member_t *m = (duck_member_t *)hash_get(&type->name_lookup, name);
   if(!m)
   {
       wprintf(L"Unknown member %ls in type %ls\n", name, type->name);
       exit(1);
   }
   
   return m->type;
}



/*
duck_object_t *duck_call(duck_stack_frame_t *stack, duck_object_t *obj)
{
    duck_object *call = *duck_get_member_addr_int(obj, DUCK_MID_CALL);
    duck_node *node = (duck_node *)*duck_get_member_addr_int(obj, DUCK_MID_NODE);
    return node->invoke(node, stack);
}
*/

duck_object_t *duck_construct(duck_object_t **param)
{
    duck_object_t *type_wrapper = param[0];
    duck_type_t *type = duck_type_unwrap(type_wrapper);
    duck_object_t *result = duck_object_create(type);
    wprintf(L"Running constructor for object of type %ls\n", type->name);
    
    /*
      FIXME: Constructor goes here!
    */
    
    return result;
}


/*
duck_object_t *duck_i_get(duck_node_lookup_t *this, duck_stack_frame_t *stack)
{
  wchar_t *name = this->child[0]->name;
  return duck_stack_get(stack, name);
}

duck_object_t *duck_i_set(duck_node_t *this, duck_stack_frame_t *stack)
{
  wchar_t *name = this->child[0]->name;
  duck_object_t *value = this->children[1]->invoke(this->children[1], stack);
  duck_stack_set(stack, name, value);
  return value;
}
*/

/*
duck_object_t *duck_i_create_function(duck_node_call_t *this, duck_stack_frame_t *stack)
{
    wchar_t *name = this->child[0]->name;
    duck_object_t *value = this->children[1]->invoke(this->children[1], stack);
    duck_stack_set(stack, name, value);
  
    //  FIXME: Not done yet...
    return value;
}
*/

duck_object_t *duck_method_wrap(duck_object_t *method, duck_object_t *owner)
{
    duck_function_t *function_original = duck_function_unwrap(method);
    size_t func_size = sizeof(duck_function_t) + function_original->input_count*sizeof(duck_type_t *);
    duck_function_t *function_copy = malloc(func_size);
    memcpy(function_copy, function_original, func_size);
    function_copy->this = owner;
    function_copy->wrapper = duck_object_create(function_copy->type);
    memcpy(duck_member_addr_get_mid(function_copy->wrapper,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD), 
	   &function_copy, sizeof(duck_function_t *));
    memcpy(duck_member_addr_get_mid(function_copy->wrapper,DUCK_MID_FUNCTION_WRAPPER_STACK),
	   duck_member_addr_get_mid(function_original->wrapper,DUCK_MID_FUNCTION_WRAPPER_STACK),
	   sizeof(duck_stack_frame_t *));
    return function_copy->wrapper;
}

void duck_prepare_children(duck_node_call_t *in, duck_function_t *func, duck_node_list_t *parent)
{
   int i;
   for(i=0; i< in->child_count; i++)
       in->child[i] = duck_node_prepare(in->child[i], func, parent);
}

duck_object_t *duck_i_member_call(duck_node_call_t *this, duck_stack_frame_t *stack)
{
/*
    wprintf(L"member_get on node at %d\n", this);
    duck_node_print((duck_node_t *)this);
    wprintf(L"\n");
*/  
    duck_object_t *obj = duck_node_invoke(this->child[0], stack);
    duck_node_lookup_t *name_node = node_cast_lookup(this->child[1]);
    //   wprintf(L"name node is %d, name is %ls\n", name_node, name_node->name);
    duck_object_t *member = *duck_member_addr_get_str(obj, name_node->name);
    
    if(member == null_object){
      return member;
    }
    
    return duck_function_wrapped_invoke(member, this, stack);    
}

void duck_function_prepare(duck_function_t *function)
{
    /*
      FIXME:
      - Validation
      - Replace memberGet + call with memberCall
    */
   int i;
   duck_node_list_t list = 
      {
	 (duck_node_t *)function->body, 0, 0
      }
   ;
   for(i=0; i<function->body->child_count; i++) 
   {
      list.idx=i;
      function->body->child[i] = duck_node_prepare(function->body->child[i], function, &list);
   }
   /*
   wprintf(L"Function after preparations:\n");
   duck_node_print(function->body);
   wprintf(L"\n");
   */
}


duck_function_t *duck_function_create(wchar_t *name,
				      int flags,
				      duck_node_call_t *body, 
				      duck_type_t *return_type,
				      size_t argc,
				      duck_type_t **argv,
				      wchar_t **argn,
				      duck_stack_frame_t *parent_stack)
{
    if(!flags) {
	assert(return_type);
	if(argc) {
	    assert(argv);
	    assert(argn);
	}
    }
    
    duck_function_t *result = calloc(1,sizeof(duck_function_t) + argc*sizeof(duck_type_t *));
    result->native.function=0;
    result->flags=flags;
    result->name = name;
    result->body = body;
    result->return_type=return_type;
    result->input_count=argc;
    memcpy(&result->input_type, argv, sizeof(duck_type_t *)*argc);
    result->input_name = argn;
    
    duck_type_t *function_type = duck_type_for_function(return_type, argc, argv);
    result->type = function_type;    
    result->wrapper = duck_object_create(function_type);
    memcpy(duck_member_addr_get_mid(result->wrapper,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD), &result, sizeof(duck_function_t *));
    memcpy(duck_member_addr_get_mid(result->wrapper,DUCK_MID_FUNCTION_WRAPPER_STACK), &stack_global, sizeof(duck_stack_frame_t *));
    //wprintf(L"Function object is %d, wrapper is %d\n", result, result->wrapper);
    result->stack_template = duck_stack_create(64, parent_stack);
    int i;
    for(i=0; i<argc;i++)
    {
	duck_stack_declare(result->stack_template, argn[i], argv[i], null_object);	
    }
    
    duck_function_prepare(result);
    
    return result;
}

duck_function_t *duck_native_create(wchar_t *name,
				    int flags,
				    duck_native_t native, 
				    duck_type_t *return_type,
				    size_t argc,
				    duck_type_t **argv,
				    wchar_t **argn)				
{
    if(!flags) {
	assert(return_type);
	if(argc) {
	    assert(argv);
	    assert(argn);
	}
    }
    //wprintf(L"duck_native_create\n");
  
    duck_function_t *result = calloc(1, sizeof(duck_function_t) + argc*sizeof(duck_type_t *));
    result->flags=flags;
    result->native = native;
    result->name = name;
    result->return_type=return_type;
    result->input_count=argc;
    memcpy(&result->input_type, argv, sizeof(duck_type_t *)*argc);
    result->input_name = argn;
    duck_type_t *function_type = duck_type_for_function(return_type, argc, argv);
    result->type = function_type;
    result->wrapper = duck_object_create(function_type);
    
    duck_object_t **member_ptr = duck_member_addr_get_mid(result->wrapper,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD);
    if(!member_ptr) {
	wprintf(L"Error: function_wrapper_type for function %ls does not have a payload!!!\n",
		name);
	//duck_object_print(result->wrapper, 3);
	CRASH;	
    }
    
    memcpy(duck_member_addr_get_mid(result->wrapper,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD), &result, sizeof(duck_function_t *));
    //wprintf(L"Creating function %ls @ %d with macro flag %d\n", result->name, result, result->flags);

    return result;
}


int duck_abides(duck_type_t *contender, duck_type_t *role_model)
{
    return contender == role_model;
}

/*
duck_object_t *duck_i_member_call(duck_node_t *this, duck_stack_frame_t *stack)
{
  duck_object_t *obj = duck_i_get(this, stack);
  wchar_t *name = this->child[1]->name;
  duck_object_t *method = *duck_member_addr_get_str(obj, name);
  
}
*/

 /*
duck_object_t *duck_i_const_string(duck_node_t *this, duck_stack_frame_t *stack)
{
    duck_node_string_literal_t *this2 = node_cast_string_literal(this);
    return duck_create_string(this2->payload_size, this2->payload);
}
 */
duck_object_t *duck_i_const_int(duck_node_t *this, duck_stack_frame_t *stack)
{
    duck_node_int_literal_t *this2 = node_cast_int_literal(this);
    duck_object_t *result = duck_int_create();
    duck_int_set(result, this2->payload);
    return result;
}

duck_object_t *duck_i_null_function(duck_object_t **node_base)
{
  wprintf(L"YAY!! RUNNING THE NULL FUNCTION\n");
  return null_object;
}


duck_type_t *duck_type_unwrap(duck_object_t *wrapper)
{
    duck_type_t *result;
    
    memcpy(&result, duck_member_addr_get_mid(wrapper, DUCK_MID_TYPE_WRAPPER_PAYLOAD), sizeof(duck_type_t *));
    return result;
}

static void duck_type_wrapper_create(duck_type_t *result, int creatable)
{
  result->wrapper = duck_object_create(type_type);
  if(creatable) {      
      duck_type_t *argv[]=
	  {
	      type_type
	  }
      ;
      wchar_t *argn[]=
	  {
	      L"type"
	  }
      ;
      
      duck_native_method_create(result, DUCK_MID_CALL_PAYLOAD, L"__call__",
				0, (duck_native_t)&duck_construct,
				result, 1, argv, argn);
      memcpy(duck_member_addr_get_mid(result->wrapper, DUCK_MID_TYPE_WRAPPER_PAYLOAD), &result, sizeof(duck_type_t *));
  } 
  
}

static duck_type_t *duck_type_create_raw(wchar_t *name, size_t static_member_count)
{
    duck_type_t *result = calloc(1,sizeof(duck_type_t)+sizeof(duck_object_t *)*static_member_count);
    result->static_member_count = 0;
    result->member_count = 0;
    hash_init(&result->name_lookup, &hash_wcs_func, &hash_wcs_cmp);
    result->mid_lookup = duck_mid_lookup_create();
    result->name = name;
    return result;  
}

duck_type_t *duck_type_create(wchar_t *name, size_t static_member_count)
{
    duck_type_t *result = duck_type_create_raw(name, static_member_count);
    duck_type_wrapper_create(result, 1);
    return result;
}
			  
duck_object_t *duck_object_create(duck_type_t *type) {
   duck_object_t *result = calloc(1,sizeof(duck_object_t)+sizeof(duck_object_t *)*type->member_count);
   result->type = type;
  
   return result;
}

size_t duck_member_create(duck_type_t *type,
			  ssize_t mid,
			  wchar_t *name,
			  int is_static,
			  duck_type_t *member_type)
{
   assert(member_type);
    duck_member_t * member = calloc(1,sizeof(duck_member_t) + sizeof(wchar_t) * (wcslen(name)+1));

    wcscpy(member->name, name);
    
    if (mid == (ssize_t)-1) {
	mid = duck_mid_get(name);
    }
    else 
    {
      if(mid != duck_mid_get(name))
	{
	  wprintf(L"Error, multiple mids for name %ls: %d and %d\n", name, mid, duck_mid_get(name));
	  exit(1);
	}
    }
    
    member->type = member_type;
    member->is_static = is_static;
    if(is_static) {
	member->offset = type->static_member_count++;
    } else {
	member->offset = type->member_count++;
    }
//  wprintf(L"Add member with mid %d\n", mid);
    
    type->mid_lookup[mid] = member;
    hash_put(&type->name_lookup, name, member);
    return mid;
}

static duck_member_t **duck_mid_lookup_create()
{
    /*
      FIXME: Track, reallocate when we run out of space, etc.
    */
    return calloc(1,4096);
}

size_t duck_native_method_create(duck_type_t *type,
				 ssize_t mid,
				 wchar_t *name,
				 int flags,
				 duck_native_t func,
				 duck_type_t *result,
				 size_t argc,
				 duck_type_t **argv,
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
  
  mid = duck_member_create(type, mid, name, 1, duck_type_for_function(result, argc, argv));
  duck_member_t *m = type->mid_lookup[mid];
  //wprintf(L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
  type->static_member[m->offset] = duck_native_create(name, flags, func, result, argc, argv, argn)->wrapper;
  return (size_t)mid;
}

static void duck_type_type_create_early()
{
  type_type->member_count = 1;
  duck_member_create(type_type, DUCK_MID_TYPE_WRAPPER_PAYLOAD, L"!typeWrapperPayload",
		     0, null_type);
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

static void duck_null_type_create_early()
{
  int i;
  
  null_type->member_count = 0;
  
  wchar_t *member_name = L"!null_member";
  
  duck_member_t *null_member = malloc(sizeof(duck_member_t)+(sizeof(wchar_t*)*(1+wcslen(member_name))));
  //wprintf(L"Null member is %d\n", null_member);

  null_member->type = null_type;
  null_member->offset=0;
  null_member->is_static=1;
  wcscpy(null_member->name, member_name);
  
  null_type->static_member[0] = duck_native_create(L"!nullFunction", 0, 
						   (duck_native_t)&duck_i_null_function, 
						   null_type, 0, 0,0)->wrapper;

  hash_init(&null_type->name_lookup, &hash_null_func, &hash_null_cmp);
  hash_put(&null_type->name_lookup, L"!null_member", null_member);
  
  for(i=0; i<64;i++) {
    null_type->mid_lookup[i] = null_member;
  }
}


static void duck_object_type_create_early()
{
    object_type->member_count = 0;
}


static void duck_init()
{
    hash_init(&duck_mid_lookup, &hash_wcs_func, &hash_wcs_cmp);
    hash_init(&duck_type_for_function_lookup, &hash_function_type_func, &hash_function_type_comp);
    
    duck_mid_put(L"!typeWrapperPayload", DUCK_MID_TYPE_WRAPPER_PAYLOAD);
    duck_mid_put(L"!callPayload", DUCK_MID_CALL_PAYLOAD);
    duck_mid_put(L"!stringPayload", DUCK_MID_STRING_PAYLOAD);
    duck_mid_put(L"!stringPayloadSize", DUCK_MID_STRING_PAYLOAD_SIZE);
    duck_mid_put(L"!charPayload", DUCK_MID_CHAR_PAYLOAD);
    duck_mid_put(L"!intPayload", DUCK_MID_INT_PAYLOAD);
    duck_mid_put(L"!listPayload", DUCK_MID_LIST_PAYLOAD);
    duck_mid_put(L"!listSize", DUCK_MID_LIST_SIZE);
    duck_mid_put(L"!listCapacity", DUCK_MID_LIST_CAPACITY);
    duck_mid_put(L"!functionTypePayload", DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    duck_mid_put(L"!functionPayload", DUCK_MID_FUNCTION_WRAPPER_PAYLOAD);
    duck_mid_put(L"!floatPayload", DUCK_MID_FLOAT_PAYLOAD);
    duck_mid_put(L"!functionStack", DUCK_MID_FUNCTION_WRAPPER_STACK);
    duck_mid_put(L"__call__", DUCK_MID_CALL_PAYLOAD);    
    
    stack_global = duck_stack_create(4096, 0);
    
    /*
      Create lowest level stuff. Bits of magic, be careful with
      ordering here. A lot of intricate dependencies going on between
      the various calls...
    */
    
    type_type = duck_type_create_raw(L"Type", 64);
    object_type = duck_type_create_raw(L"Object", 64);
    null_type = duck_type_create_raw(L"Null", 1);

    string_type = duck_type_create_raw(L"String", 64);
    char_type = duck_type_create_raw(L"Char", 64);
    float_type = duck_type_create_raw(L"Float", 64);
    
    duck_type_type_create_early();
    duck_object_type_create_early();
    duck_null_type_create_early();

    duck_int_type_create(stack_global);
    duck_list_type_create(stack_global);
    duck_char_type_create();
    duck_string_type_create();
    duck_float_type_create();
    
    duck_type_wrapper_create(type_type,1);
    duck_type_wrapper_create(null_type, 1);
    duck_type_wrapper_create(object_type, 1);
    duck_type_wrapper_create(int_type, 1);
    duck_type_wrapper_create(string_type, 1);
    duck_type_wrapper_create(char_type, 1);
    duck_type_wrapper_create(list_type, 1);
    duck_type_wrapper_create(float_type, 1);
    
    duck_stack_declare(stack_global, L"Object", object_type, object_type->wrapper);
    duck_stack_declare(stack_global, L"Null", null_type, null_type->wrapper);
    duck_stack_declare(stack_global, L"Char", type_type, char_type->wrapper);
    duck_stack_declare(stack_global, L"List", type_type, list_type->wrapper);
    duck_stack_declare(stack_global, L"String", type_type, string_type->wrapper);
    duck_stack_declare(stack_global, L"Float", type_type, float_type->wrapper);
    duck_stack_declare(stack_global, L"Type", type_type, type_type->wrapper);
    
    null_object = duck_object_create(null_type);
    
    duck_function_implementation_init(stack_global);
    duck_macro_init(stack_global);

}

void duck_print_member(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    wchar_t *key = (wchar_t *)key_ptr;
    duck_member_t *member = (duck_member_t *)val_ptr;
    duck_object_t *obj = (duck_object_t *)aux_ptr;
    duck_object_t *value = member->is_static?obj->type->static_member[member->offset]:obj->member[member->offset];    
    wprintf(L"  %ls: %ls\n", key, member->type?member->type->name:L"?");
}

void duck_object_print(duck_object_t *obj, int level)
{
    wprintf(L"%ls:\n", obj->type->name);
    hash_foreach2(&obj->type->name_lookup, &duck_print_member, obj);
}

duck_object_t *duck_function_invoke(duck_function_t *function, 
				    duck_node_call_t *param, 
				    duck_stack_frame_t *stack,
				    duck_stack_frame_t *outer) 
{
    //wprintf(L"duck_function_invoke %ls %d; %d params\n", function->name, function, function->input_count);    
    
    switch(function->flags) 
    {
	default:
	{
	    wprintf(L"FATAL: Macro %ls at invoke!!!!\n", function->name);
	    exit(1);
	}
	case DUCK_FUNCTION_FUNCTION:
	{
	    if(function->native.function) 
	    {
		duck_object_t **argv=malloc(sizeof(duck_object_t *)*function->input_count);
		int i;
		
		int offset=0;
		if(function->this)
		{
		    offset=1;
		    argv[0]=function->this;		    
		}
		
		for(i=0; i<(function->input_count-offset); i++) 
		{
		    argv[i+offset]=duck_node_invoke(param->child[i], stack);
		}
		//wprintf(L"native, %d\n", function->native);    
		return function->native.function(argv);
	    }
	    else 
	    {
		duck_object_t *result = null_object;
		int i;
		duck_stack_frame_t *my_stack = duck_stack_clone(function->stack_template);//function->input_count+1);
		my_stack->parent = outer;
		
		/*
		  FIXME:
		  Support return statement
		*/
		//wprintf(L"Run non-native function %ls with %d params\n", function->name, function->input_count);
		int offset=0;
		if(function->this)
		{
		    offset=1;
		    duck_stack_declare(my_stack, 
				       function->input_name[0],
				       function->input_type[0],
				       function->this);
		}
		
		for(i=0; i<(function->input_count-offset); i++) 
		{
		    duck_object_t *value=duck_node_invoke(param->child[i], stack);
		    duck_stack_declare(my_stack, 
				       function->input_name[i+offset],
				       function->input_type[i+offset],
				       value);
		}
		
		for(i=0; i<function->body->child_count; i++)
		{
		    result = duck_node_invoke(function->body->child[i], my_stack);
		}
		return result;
	    }
	}
    }
}

int main()
{
    wprintf(L"Initializing interpreter.\n");    
    duck_init();
    wprintf(L"Parsing program.\n");    
    duck_node_t *program = duck_parse(stdin, L"stdin");
    
    if(!program) 
    {
       wprintf(L"Program failed to parse correctly; exiting.\n");
       exit(1);
    }
    
    wprintf(L"Run program:\n");    
    duck_node_print(program);
    wprintf(L"\n");
    wprintf(L"Output:\n");    
    
    /*
      The entire program is a __block__ call, which we use to create an anonymous function definition
     */
    duck_node_dummy_t *program_callable = 
      duck_node_dummy_create(program->source_filename,
			     program->source_position,
			     duck_function_create(L"!program",
						  0,
						  node_cast_call(program),
						  null_type, 0, 0, 0, stack_global)->wrapper,
			     0);
    /*
      Invoke the anonymous function, the return is a function_type_t->wrapper
    */
    duck_object_t *program_object = duck_node_invoke((duck_node_t *)program_callable, stack_global);
        
    /*
      Run the function
     */
    duck_function_t *func=duck_function_unwrap(program_object);    
    assert(func);
    duck_function_invoke(func, 0, stack_global, stack_global);
    
    wprintf(L"\n");
}
