#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "duck.h"
#include "duck_node.h"
#include "duck_stack.h"
#include "duck_int.h"
#include "duck_float.h"
#include "duck_string.h"
#include "duck_char.h"
#include "duck_list.h"

#define likely(x) (x)
/*
  Ideas for apps written in duck:
  Documentation generator: Introspect all data types and generate html documentation on them.
  live: Debug a running application using command line or web browser.
  Compiler front end.
*/
/*
  ComparisonMap type
  HashMap type
  Range type
  Pair type
  Node type
  Better Function type
  Better Type type  
  
  Code validator
  Type checking on function types
  Type support for lists
  General purpose currying
  Namespaces
  Method macros
  Functions that don't return an Int
 
  Implement basic string methods
  Implement string comparison methods
  Functional functions for lists (each, map, select, max, min)
  List arithmetic
  
  Function default argument values
  Named function arguments
  Variadic functions
  Garbage collection  
  Proper intersection of types
  static member lookup and assignment
  static function calls
  class member macros

  cast function
  import macro
  __macro__ macro
  elif macro
  each function
  extends macro
  abides function
  __returnAssign__ macro
  __templatize__ macro
  template macro
  __list__ macro
  use macro
  __memberCall__ macro
  __staticMemberGet__ macro
  __staticMember_set__ macro
  __with__ macro

  Done: 
  
  Sugar parser
  Real check if type abides to other type, instead of lame type ptr comparison
  Correct string literal parsing
  Variable declarations
  Function argument passing
  Method calls with proper this handling
  Inner functions with access to outer scope
  Make ; after } optional
  Constructors with no parameters
  Simple type checking
  Constructors
  Inner functions with shared return flag
  Do some real testing to find the optimal operator presedence order
  
  Type type
  Call type
  Int type
  String type
  Char type
  Null type  
  List type
  
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
  Implement basic float methods
  Implement int comparison methods
  Implement List getter and setter
  Implement basic char methods
  Implement char comparison methods
  Implement basic list methods
  
  __not__ function
  __block__ macro
  __assign__ macro
  __declare__ macro
  __function__ macro
  __if__ function
  __add__, __sub__, __mul__ and friends, simple exact mapping
  __set__, __get__, __add__, __sub__, __mul__ and friends: full abides/reverse checking implementation
  if macro
  else macro
  __get__ macro
  __set__ macro
  __or__ macro
  __and__ macro
  while macro
  __while__ function
  __type__ function
  return macro
  
*/

duck_type_t *type_type=0, *object_type=0, *int_type=0, *string_type=0, *char_type=0, *null_type=0,  *string_type, *char_type, *list_type, *float_type;
duck_object_t *null_object=0;

static hash_table_t duck_type_for_function_lookup;
static hash_table_t duck_mid_lookup;

duck_node_t *duck_node_null=0;

static duck_stack_frame_t *stack_global;
static size_t mid_pos = DUCK_MID_FIRST_UNRESERVED;

int duck_error_count=0;

static duck_member_t **duck_mid_lookup_create();

static duck_type_t *duck_type_create_raw(wchar_t *name, size_t static_member_count);
static void duck_type_wrapper_create(duck_type_t *result);

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

static void add_member(void *key, void *value, void *aux)
{
//    wprintf(L"Got member %ls\n", key);
    
    wchar_t ***dest = (wchar_t ***)aux;
    **dest = key;
    (*dest)++;
}


void duck_type_get_member_names(duck_type_t *type, wchar_t **dest)
{
    hash_foreach2(&type->name_lookup, &add_member, &dest);
}



duck_object_t **duck_member_addr_get_str(duck_object_t *obj, wchar_t *name)
{
    duck_member_t *m = (duck_member_t *)hash_get(&(obj->type->name_lookup), name);
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

duck_object_t **duck_member_addr_get_mid(duck_object_t *obj, size_t mid)
{
  /*
  wprintf(L"Get mid %d on object\n", mid);
  wprintf(L"of type %ls\n", obj->type->name);
  */
    duck_member_t *m = obj->type->mid_lookup[mid];
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
	return 0;
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
	duck_type_wrapper_create(res);
	
	(*duck_static_member_addr_get_mid(res, DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD)) = (duck_object_t *)new_key;
    }
    else 
    {
      //wprintf(L"YAY\n");
    }
    
    return res;
}


duck_function_type_key_t *duck_function_unwrap_type(duck_type_t *type)
{
    assert(type);
    //wprintf(L"Find function signature for call %ls\n", type->name);
    
    duck_function_type_key_t **function_ptr = (duck_function_type_key_t **)duck_static_member_addr_get_mid(type, DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    if(function_ptr) 
    {
	//wprintf(L"Got member, has return type %ls\n", (*function_ptr)->result->name);
	return *function_ptr;
    }
    else 
    {
	//wprintf(L"Not a direct function, check for __call__ member\n");
	duck_object_t **function_wrapper_ptr = duck_static_member_addr_get_mid(type, DUCK_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    //wprintf(L"Found, we're unwrapping it now\n");
	    return duck_function_unwrap_type((*function_wrapper_ptr)->type);	    
	}
	return 0;	
    }
//     FIXME: Is there any validity checking we could do here?
  
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
	return 0;	
    }
//     FIXME: Is there any validity checking we could do here?
  
}

duck_object_t *duck_function_wrapped_invoke(duck_object_t *obj, 
					    duck_object_t *this,
					    duck_node_call_t *param,
					    duck_stack_frame_t *local)
{
  //wprintf(L"Wrapped invoke of function %ls\n", obj->type->name);
  
    duck_function_t **function_ptr = (duck_function_t **)duck_member_addr_get_mid(obj, DUCK_MID_FUNCTION_WRAPPER_PAYLOAD);
    duck_stack_frame_t **stack_ptr = (duck_stack_frame_t **)duck_member_addr_get_mid(obj, DUCK_MID_FUNCTION_WRAPPER_STACK);
    if(function_ptr) 
    {
        return duck_function_invoke(*function_ptr, this, param, local, stack_ptr?*stack_ptr:stack_global);
    }
    else 
    {
	duck_object_t **function_wrapper_ptr = duck_static_member_addr_get_mid(obj->type, DUCK_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	  return duck_function_wrapped_invoke(*function_wrapper_ptr,obj, param, stack_ptr?*stack_ptr:stack_global);
	}
	
	wprintf(L"Critical: Tried to call a non-function");
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
    assert(type);
    assert(name);
    duck_member_t *m = (duck_member_t *)hash_get(&type->name_lookup, name);
    if(!m)
    {
	return 0;
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

duck_object_t *duck_construct(duck_type_t *type, struct duck_node_call *param, duck_stack_frame_t *stack)
{
    duck_object_t *result = duck_object_create(type);
    //wprintf(L"Creating new object of type %ls\n", type->name);
    assert(type->name);
    
    duck_object_t **constructor_ptr = duck_member_addr_get_mid(result, DUCK_MID_INIT_PAYLOAD);
    if(constructor_ptr)
    {
        duck_function_t *constructor = duck_function_unwrap(*constructor_ptr);
	//wprintf(L"First param is %ls type\n", param[1]->type->name);
	duck_function_invoke(constructor, result, param, stack, 0);
    }
    
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

duck_object_t *duck_i_member_call(duck_node_call_t *param, duck_stack_frame_t *stack)
{
/*
    wprintf(L"member_get on node at %d\n", param);
    duck_node_print((duck_node_t *)param);
    wprintf(L"\n");
*/  
    duck_object_t *obj = duck_node_invoke(param->child[0], stack);
    duck_node_lookup_t *name_node = node_cast_lookup(param->child[1]);
    //   wprintf(L"name node is %d, name is %ls\n", name_node, name_node->name);
    duck_object_t *member = *duck_member_addr_get_str(obj, name_node->name);
    
    if(member == null_object){
      return member;
    }
    
    return duck_function_wrapped_invoke(member, 0, param, stack);    
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
   for(i=0; i<function->body->child_count; i++) 
   {
       duck_node_validate(function->body->child[i], function->stack_template);
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
				      duck_stack_frame_t *parent_stack,
				      int return_pop_count)
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
    result->return_pop_count = return_pop_count;
    
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

duck_type_t *duck_type_intersect(duck_type_t *t1, duck_type_t *t2)
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

int duck_abides_fault_count(duck_type_t *contender, duck_type_t *role_model)
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
    wchar_t **members = calloc(sizeof(wchar_t *), role_model->member_count+role_model->static_member_count);
    duck_type_get_member_names(role_model, members);    
    assert(hash_get_count(&role_model->name_lookup) == role_model->member_count+role_model->static_member_count);
    
    for(i=0; i<role_model->member_count+role_model->static_member_count; i++)
    {
	assert(members[i]);
	duck_type_t *contender_subtype = duck_type_member_type_get(contender, members[i]);
	duck_type_t *role_model_subtype = duck_type_member_type_get(role_model, members[i]);
	if(!contender_subtype || !duck_abides(contender_subtype, role_model_subtype))
	{
	    res++;
	}	
    }
    free(members);
    return res;
}

int duck_abides(duck_type_t *contender, duck_type_t *role_model)
{
    return !duck_abides_fault_count(contender, role_model);
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
    duck_object_t *result = duck_int_create(this2->payload);
    return result;
}

/**
  This method is the best ever! All method calls on the null object run this
*/
duck_object_t *duck_i_null_function(duck_object_t **node_base)
{
  return null_object;
}

duck_type_t *duck_type_unwrap(duck_object_t *wrapper)
{
    duck_type_t *result;
    
    memcpy(&result, duck_member_addr_get_mid(wrapper, DUCK_MID_TYPE_WRAPPER_PAYLOAD), sizeof(duck_type_t *));
    return result;
}

static void duck_type_wrapper_create(duck_type_t *result)
{
  result->wrapper = duck_object_create(type_type);
  memcpy(duck_member_addr_get_mid(result->wrapper, DUCK_MID_TYPE_WRAPPER_PAYLOAD), &result, sizeof(duck_type_t *));  
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
    duck_type_wrapper_create(result);
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
    if(hash_get(&type->name_lookup, name))
    {
	wprintf(L"Redeclaring member %ls of type %ls\n",
		name, type->name);
	CRASH;	
    }
    
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

size_t duck_method_create(duck_type_t *type,
			  ssize_t mid,
			  wchar_t *name,
			  int flags,
			  duck_function_t *definition)		
{
    mid = duck_member_create(type, mid, name, 1, definition->type);
    duck_member_t *m = type->mid_lookup[mid];
    //wprintf(L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    type->static_member[m->offset] = definition->wrapper;
    return (size_t)mid;
}



static void duck_type_type_create_early()
{
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
  
  wchar_t *member_name = L"!null_member";
  duck_member_t *null_member;  
  null_member = malloc(sizeof(duck_member_t)+(sizeof(wchar_t*)*(1+wcslen(member_name))));
  //wprintf(L"Null member is %d\n", null_member);

  null_member->type = null_type;
  null_member->offset=0;
  null_member->is_static=1;
  wcscpy(null_member->name, member_name);

  /*  
    duck_native_method_create(list_type, -1, L"__getInt__", 0, (duck_native_t)&duck_list_getitem, object_type, 2, i_argv, i_argn);
  */
  duck_type_t *argv[]={null_type};
  wchar_t *argn[]={L"this"};
  
  null_type->static_member[0] = duck_native_create(L"!nullFunction", DUCK_FUNCTION_FUNCTION, 
						   (duck_native_t)&duck_i_null_function, 
						   null_type, 1, argv, argn)->wrapper;
  
  duck_object_t *null_function;  
  null_function = null_type->static_member[0];
  hash_init(&null_type->name_lookup, &hash_null_func, &hash_null_cmp);
  hash_put(&null_type->name_lookup, L"!null_member", null_member);
  
  for(i=0; i<64;i++) {
    null_type->mid_lookup[i] = null_member;
  }
  assert(*duck_static_member_addr_get_mid(null_type, 5) == null_function);    
}


static void duck_object_type_create_early()
{
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
    duck_mid_put(L"__init__", DUCK_MID_INIT_PAYLOAD);

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
    
    duck_type_type_create_early();
    duck_object_type_create_early();
    duck_null_type_create_early();

    duck_int_type_create(stack_global);
    duck_list_type_create(stack_global);
    duck_char_type_create(stack_global);
    duck_string_type_create();
    duck_float_type_create(stack_global);

    duck_type_wrapper_create(type_type);
    duck_type_wrapper_create(null_type);
    duck_type_wrapper_create(object_type);
    duck_type_wrapper_create(string_type);
        
    duck_stack_declare(stack_global, L"Object", object_type, object_type->wrapper);
    duck_stack_declare(stack_global, L"Null", null_type, null_type->wrapper);
    duck_stack_declare(stack_global, L"List", type_type, list_type->wrapper);
    duck_stack_declare(stack_global, L"String", type_type, string_type->wrapper);
    duck_stack_declare(stack_global, L"Float", type_type, float_type->wrapper);
    duck_stack_declare(stack_global, L"Type", type_type, type_type->wrapper);

    assert(duck_abides(int_type,object_type)==1);
    assert(duck_abides(list_type,object_type)==1);
    assert(duck_abides(object_type,int_type)==0);
    assert(duck_abides(int_type,int_type)==1);
    
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

duck_object_t *duck_function_invoke_values(duck_function_t *function, 
					   duck_object_t *this,
					   duck_object_t **param,
					   duck_stack_frame_t *outer)
{
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
	        duck_object_t **argv=param;
		int i;
		
		int offset=0;
		if(this)
		{
		    argv=malloc(sizeof(duck_object_t *)*function->input_count);
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
		duck_object_t *result = null_object;
		int i;
		duck_stack_frame_t *my_stack = duck_stack_clone(function->stack_template);//function->input_count+1);
		my_stack->parent = outer?outer:stack_global;
		/*
		  FIXME:
		  Support return statement
		*/
		//wprintf(L"Run non-native function %ls with %d params\n", function->name, function->input_count);
		int offset=0;
		if(this)
		{
		    offset=1;
		    duck_stack_declare(my_stack, 
				       function->input_name[0],
				       function->input_type[0],
				       this);
		}
		
		for(i=0; i<(function->input_count-offset); i++) 
		{
		  /*		    wprintf(L"Declare input variable %ls on stack\n",
			    function->input_name[i+offset]);
		  */
		    duck_stack_declare(my_stack, 
				       function->input_name[i+offset],
				       function->input_type[i+offset],
				       param[i]);
		}
		
		for(i=0; i<function->body->child_count && !my_stack->stop; i++)
		{
		    result = duck_node_invoke(function->body->child[i], my_stack);
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
}


duck_object_t *duck_function_invoke(duck_function_t *function, 
				    duck_object_t *this,
				    duck_node_call_t *param, 
				    duck_stack_frame_t *stack,
				    duck_stack_frame_t *outer) 
{
    if(!this)
    {
	this=function->this;
    }
    
    //wprintf(L"duck_function_invoke %ls %d; %d params\n", function->name, function, function->input_count);    
    if(likely(function->input_count < 8))
    {
	duck_object_t *argv[8];
	int i;
	
	int offset=0;
	if(this)
	{
	    
	    
	    offset=1;
	    argv[0]=this;		    
	}
	
	for(i=0; i<(function->input_count-offset); i++) 
	{
	    //wprintf(L"eval param %d of %d\n", i, function->input_count);
	    
	    argv[i+offset]=duck_node_invoke(param->child[i], stack);
	}      
	return duck_function_invoke_values(function, 0, argv, outer);
    }
    else
    {
	duck_object_t **argv=malloc(sizeof(duck_object_t *)*function->input_count);
	int i;
	
	int offset=0;
	if(this)
	{
	    offset=1;
	    argv[0]=this;		    
	}
	for(i=0; i<(function->input_count-offset); i++) 
	{
	    argv[i+offset]=duck_node_invoke(param->child[i], stack);
	}
	duck_object_t *result = duck_function_invoke_values(function, 0, argv, outer);
	free(argv);
	return result;
      
    }
  
}



void duck_error(duck_node_t *node, wchar_t *msg, ...)
{
    va_list va;
    va_start( va, msg );	
    fwprintf(stderr,L"Error in %ls, on line %d:\n", 
	     node->location.filename,
	     node->location.first_line);
    duck_node_print_code(node);
    fwprintf(stderr,L"\n");
    
    vfwprintf(stderr, msg, va);
    va_end( va );
    fwprintf(stderr, L"\n\n");
    duck_error_count++;
}

int main(int argc, char **argv)
{
  if(argc != 2)
    {
      wprintf(L"Error: Expected exactly one argument, a name of a file to run.\n");
      exit(1);
    }
    wchar_t *filename = str2wcs(argv[1]);

    wprintf(L"Initializing interpreter.\n");    
    duck_init();
    wprintf(L"Parsing program %ls.\n", filename);    
    duck_node_t *program = duck_parse(filename);
    
    if(!program) 
    {
       wprintf(L"Program failed to parse correctly; exiting.\n");
       exit(1);
    }
    
    wprintf(L"Parsed program:\n");    
    duck_node_print(program);
    wprintf(L"\n");
    
    wprintf(L"Validating program.\n");    
    /*
      The entire program is a __block__ call, which we use to create an anonymous function definition
     */
    duck_node_dummy_t *program_callable = 
	duck_node_dummy_create(&program->location,
			       duck_function_create(L"!program",
						    0,
						    node_cast_call(program),
						    null_type, 0, 0, 0, stack_global, 0)->wrapper,
			       0);
    /*
      Invoke the anonymous function, the return is a function_type_t->wrapper
    */
    duck_object_t *program_object = duck_node_invoke((duck_node_t *)program_callable, stack_global);
    if(duck_error_count)
    {
	wprintf(L"Found %d error(s), exiting\n", duck_error_count);
	exit(1);
    }
    /*
      Run the function
     */
    wprintf(L"Output:\n");    
    duck_function_t *func=duck_function_unwrap(program_object);    
    assert(func);
    duck_function_invoke(func, 0, 0, stack_global, stack_global);
    
    wprintf(L"\n");
}
