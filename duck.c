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

/*
  List type
  ComparisonMap type
  HashMap type
  Range type
  Pair type
    
  Simple type checking
  Type template support
  Complex types
  Type support for lists
  Type checking
  General purpose currying
  Namespaces

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
  
  if macro
  else macro
  elif macro
  __type__ function
  each function
  extends function
  abides function
  __add__, __sub__, __mul__ and friends: full abides/reverse checking implementation
  __return__ function
  __assignReturn__ function  
  __templatize__ function
  template function
  __list__ function
  while macro
  __or__ macro
  __and__ macro
  
  Done: 
  
  Sugar parser
  
  Variable declarations
  Function argument passing
  Method calls with proper this handling
  Inner functions with access to outer scope
  
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
*/

/*
  The mid of the payload
*/
#define DUCK_MID_CALL_PAYLOAD 0
#define DUCK_MID_INT_PAYLOAD 1
#define DUCK_MID_STRING_PAYLOAD 2
#define DUCK_MID_STRING_PAYLOAD_SIZE 3
#define DUCK_MID_CHAR_PAYLOAD 4
#define DUCK_MID_LIST_PAYLOAD 5
#define DUCK_MID_LIST_PAYLOAD_SIZE 6
#define DUCK_MID_FLOAT_PAYLOAD 7
#define DUCK_MID_FUNCTION_WRAPPER_PAYLOAD 9
#define DUCK_MID_FUNCTION_WRAPPER_STACK 10
#define DUCK_MID_FUNCTION_WRAPPER_THIS 11
#define DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD 12
#define DUCK_MID_TYPE_WRAPPER_PAYLOAD 13
#define DUCK_MID_CALL 14
#define DUCK_MID_FIRST_UNRESERVED 15

typedef struct 
{
    duck_type_t *result; 
    size_t argc;
    duck_type_t *argv[];
} duck_function_type_key_t;


   
static duck_type_t *type_type=0, *object_type=0, *int_type=0, *string_type=0, *char_type=0, *null_type=0,  *string_type, *char_type, *list_type, *float_type;
static duck_object_t *null_object=0;
static hash_table_t duck_type_for_function_lookup;
static hash_table_t duck_mid_lookup;

duck_node_t *duck_node_null=0;

static duck_stack_frame_t *stack_global;
static size_t mid_pos = DUCK_MID_FIRST_UNRESERVED;

static duck_object_t *duck_object_create(duck_type_t *type);
duck_object_t *duck_i_float_create();
duck_object_t *duck_i_int_create();
duck_object_t *duck_i_string_create();
duck_object_t *duck_i_char_create();
void duck_i_int_set(duck_object_t *this, int value);
void duck_i_char_set(duck_object_t *this, wchar_t value);

static duck_member_t **duck_mid_lookup_create();
duck_object_t *duck_node_invoke(duck_node_t *this, duck_stack_frame_t *stack);
duck_object_t *duck_i_print(duck_object_t **param);
void duck_node_print(duck_node_t *this);
int duck_i_int_get(duck_object_t *this);
double duck_i_float_get(duck_object_t *this);
wchar_t *duck_i_string_get_payload(duck_object_t *this);
size_t duck_i_string_get_payload_size(duck_object_t *this);
wchar_t duck_i_char_get(duck_object_t *this);
void duck_i_float_set(duck_object_t *this, double value);
duck_type_t *duck_type_unwrap(duck_object_t *wrapper);
duck_function_t *duck_native_create(wchar_t *name,
				    int flags,
				    duck_native_t native, 
				    duck_type_t *return_type,
				    size_t argc,
				    duck_type_t **argv,
				    wchar_t **argn);

duck_object_t *duck_function_invoke(duck_function_t *function, duck_node_call_t *param, duck_stack_frame_t *stack, duck_stack_frame_t *outer);
duck_object_t *duck_function_wrapped_invoke(duck_object_t *function, duck_node_call_t *param, duck_stack_frame_t *local);
static duck_type_t *duck_type_create(wchar_t *name, size_t static_member_count);
static duck_type_t *duck_type_create_raw(wchar_t *name, size_t static_member_count);
static void duck_type_wrapper_create(duck_type_t *result, int creatable);

duck_object_t *duck_i_function_wrapper_call(duck_node_call_t *node, duck_stack_frame_t *stack);
void duck_object_print(duck_object_t *obj, int level);
duck_node_t *duck_node_prepare(duck_node_t *this, duck_function_t *function, duck_node_list_t *parent);duck_object_t *duck_method_wrap(duck_object_t *method, duck_object_t *owner);




/*
static size_t duck_add_method(duck_type_t *type,
			      duck_type_t *result,
			      size_t argc,
			      duck_type_t **argv,
			      wchar_t *name,
			      ssize_t mid,
			      int flags,
			      duck_native_t func);

*/
static size_t duck_native_method_create(duck_type_t *type,
					ssize_t mid,
					wchar_t *name,
					int flags,
					duck_native_t func,
					duck_type_t *result,
					size_t argc,
					duck_type_t **argv,
					wchar_t **argn);

static size_t duck_member_create(duck_type_t *type,
				 ssize_t mid,
				 wchar_t *name,
				 int is_static,
				 duck_type_t *member_type);



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

duck_node_call_t *node_cast_call(duck_node_t *node) {
    assert(node->node_type==DUCK_NODE_CALL);
    return (duck_node_call_t *)node;
}

duck_node_lookup_t *node_cast_lookup(duck_node_t *node) {
    assert(node->node_type==DUCK_NODE_LOOKUP);
    return (duck_node_lookup_t *)node;
}

duck_node_int_literal_t *node_cast_int_literal(duck_node_t *node) {
    assert(node->node_type==DUCK_NODE_INT_LITERAL);
    return (duck_node_int_literal_t *)node;
}

duck_node_string_literal_t *node_cast_string_literal(duck_node_t *node) {
    assert(node->node_type==DUCK_NODE_STRING_LITERAL);
    return (duck_node_string_literal_t *)node;
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
    //wprintf(L"Tralala, make type for function %d %d, hash is %d\n", result, argc, hash_function_type_func(key));
    
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


duck_function_t *duck_function_unwrap(duck_object_t *type)
{
    duck_function_t **function_ptr = (duck_function_t **)duck_member_addr_get_mid(type, DUCK_MID_FUNCTION_WRAPPER_PAYLOAD);
    if(function_ptr) 
    {
//	    wprintf(L"Got __call__ member\n");
	return *function_ptr;
    }
    else 
    {
	wprintf(L"DANG!!!");
	exit(1);
    }
//     FIXME: Is there any validity checking we could do here?
  
}

duck_object_t *duck_function_wrapped_invoke(duck_object_t *type, duck_node_call_t *param, duck_stack_frame_t *local)
{
    duck_function_t **function_ptr = (duck_function_t **)duck_member_addr_get_mid(type, DUCK_MID_FUNCTION_WRAPPER_PAYLOAD);
    duck_stack_frame_t **stack_ptr = (duck_stack_frame_t **)duck_member_addr_get_mid(type, DUCK_MID_FUNCTION_WRAPPER_STACK);
    if(function_ptr) 
    {
	return duck_function_invoke(*function_ptr, param, local, *stack_ptr);
    }
    else 
    {
	wprintf(L"DANG!!!");
	exit(1);
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


duck_node_t *duck_node_call_prepare(duck_node_call_t *node, duck_function_t *function, duck_node_list_t *parent)
{
   duck_node_list_t list = 
      {
	 (duck_node_t *)node, 0, parent
      }
   ;
/*
   wprintf(L"Prepare call node:\n");
   duck_node_print((duck_node_t *)node);
   wprintf(L"\n");
*/
   if(node->function->node_type == DUCK_NODE_LOOKUP)
   {
      duck_node_lookup_t *name=(duck_node_lookup_t *)node->function;      
      duck_object_t *obj = duck_node_invoke(node->function, function->stack_template);
      duck_function_t *func=duck_function_unwrap(obj);
      
      if(func->flags == DUCK_FUNCTION_MACRO)
      {
	 return duck_node_prepare(func->native.macro(node, function, parent), function, parent);
      }
   }
   else 
   {
      node->function = duck_node_prepare(node->function, function, parent);	 
   }
   
   //wprintf(L"Regular function, prepare the kids\n");
   int i;
   for(i=0; i<node->child_count; i++)
   {
      list.idx = i;
      node->child[i] = duck_node_prepare(node->child[i], function, &list);	 
   }
   return (duck_node_t *)node;
}

duck_object_t *duck_node_call_invoke(duck_node_call_t *this, duck_stack_frame_t *stack)
{
    //wprintf(L"duck_node_call_invoke with stack %d\n", stack);
    duck_object_t *obj = duck_node_invoke(this->function, stack);
    if(obj == null_object){
	return obj;
    }
    
    return duck_function_wrapped_invoke(obj, this, stack);
}

duck_object_t *duck_node_int_literal_invoke(duck_node_int_literal_t *this, duck_stack_frame_t *stack)
{
    return duck_i_int_create(this->payload);
}

duck_object_t *duck_node_float_literal_invoke(duck_node_float_literal_t *this, duck_stack_frame_t *stack)
{
    return duck_i_float_create(this->payload);
}

duck_object_t *duck_node_string_literal_invoke(duck_node_string_literal_t *this, duck_stack_frame_t *stack)
{
    return duck_i_string_create(this->payload_size, this->payload);
}

duck_object_t *duck_node_char_literal_invoke(duck_node_char_literal_t *this, duck_stack_frame_t *stack)
{
   return duck_i_char_create(this->payload);
}

duck_object_t *duck_node_lookup_invoke(duck_node_lookup_t *this, duck_stack_frame_t *stack)
{
    //wprintf(L"Lookup on string \"%ls\"\n", this->name);
    
    return duck_stack_get_str(stack, this->name);
}

duck_type_t *duck_type_member_type_get(duck_type_t *type, wchar_t *name)
{
   duck_member_t *m = (duck_member_t *)hash_get(&type->name_lookup, name);
   return m->type;
}


duck_object_t *duck_node_assign_invoke(duck_node_assign_t *this, duck_stack_frame_t *stack)
{
   duck_object_t *result = duck_node_invoke(this->value, stack);
   duck_stack_set_sid(stack, this->sid, result);
   return result;
}

duck_type_t *duck_node_get_return_type(duck_node_t *this, duck_stack_frame_t *stack)
{
    switch(this->node_type)
    {
	case DUCK_NODE_CALL:
	{
	    duck_node_call_t *this2 =(duck_node_call_t *)this;	    
	    duck_type_t *func_type = duck_node_get_return_type(this2->function, stack);
	    
	    duck_function_type_key_t *function_data = (duck_function_type_key_t *)*duck_static_member_addr_get_mid(func_type, DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
	    return function_data->result;
	}
	
	case DUCK_NODE_TRAMPOLINE:
	case DUCK_NODE_DUMMY:
	{
	   duck_node_dummy_t *this2 =(duck_node_dummy_t *)this;	    
	   return this2->payload->type;   
	}
	
	case DUCK_NODE_INT_LITERAL:
	    return int_type;

	case DUCK_NODE_FLOAT_LITERAL:
	    return float_type;

	case DUCK_NODE_LOOKUP:
	{
	    duck_node_lookup_t *this2 =(duck_node_lookup_t *)this;	    
	    return duck_stack_get_type(stack, this2->name);
	}
	case DUCK_NODE_STRING_LITERAL:
	    return string_type;

	case DUCK_NODE_CHAR_LITERAL:
	    return char_type;

	case DUCK_NODE_NULL:
	    return null_type;

	default:
	    wprintf(L"SCRAP!\n");
	    exit(1);
    }
}

duck_node_t *duck_node_prepare(duck_node_t *this, duck_function_t *function, duck_node_list_t *parent)
{
   
   switch(this->node_type)
    {
	case DUCK_NODE_CALL:
	   return duck_node_call_prepare((duck_node_call_t *)this, function, parent);

	case DUCK_NODE_TRAMPOLINE:
	case DUCK_NODE_DUMMY:
	case DUCK_NODE_INT_LITERAL:
	case DUCK_NODE_FLOAT_LITERAL:
	case DUCK_NODE_LOOKUP:
	case DUCK_NODE_STRING_LITERAL:
	case DUCK_NODE_CHAR_LITERAL:
	case DUCK_NODE_NULL:
	case DUCK_NODE_ASSIGN:
	case DUCK_NODE_MEMBER_GET:
	case DUCK_NODE_MEMBER_GET_WRAP:
	    return this;
	    
	default:
	    wprintf(L"HULP %d\n", this->node_type);
	    exit(1);
    }
   

}

duck_object_t *duck_node_member_get_invoke(duck_node_member_get_t *this, duck_stack_frame_t *stack)
{
   return *duck_member_addr_get_mid(duck_node_invoke(this->object, stack), this->mid);
}

duck_object_t *duck_node_member_get_wrap_invoke(duck_node_member_get_t *this, duck_stack_frame_t *stack)
{
    duck_object_t *obj = duck_node_invoke(this->object, stack);
    return duck_method_wrap(*duck_member_addr_get_mid(obj, this->mid), obj);
}

duck_object_t *duck_trampoline(duck_object_t *orig, duck_stack_frame_t *stack)
{
    duck_object_t *res = duck_object_create(orig->type);
    memcpy(duck_member_addr_get_mid(res,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD),
	   duck_member_addr_get_mid(orig,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD),
	   sizeof(duck_function_t *));    
    memcpy(duck_member_addr_get_mid(res,DUCK_MID_FUNCTION_WRAPPER_STACK),
	   &stack,
	   sizeof(duck_stack_frame_t *));
    return res;
}


duck_object_t *duck_node_invoke(duck_node_t *this, duck_stack_frame_t *stack)
{
    //wprintf(L"duck_node_invoke with stack %d\n", stack);
//    wprintf(L"invoke %d\n", this->node_type);    
    switch(this->node_type)
    {
	case DUCK_NODE_CALL:
	    return duck_node_call_invoke((duck_node_call_t *)this, stack);

	case DUCK_NODE_DUMMY:
	{
	   duck_node_dummy_t *node = (duck_node_dummy_t *)this;
	   return node->payload;
	}
	
	case DUCK_NODE_TRAMPOLINE:
	{
	    duck_node_dummy_t *node = (duck_node_dummy_t *)this;
	    return duck_trampoline(node->payload, stack);
	}
	
	case DUCK_NODE_INT_LITERAL:
	    return duck_node_int_literal_invoke((duck_node_int_literal_t *)this, stack);

	case DUCK_NODE_FLOAT_LITERAL:
	   return duck_node_float_literal_invoke((duck_node_float_literal_t *)this, stack);

	case DUCK_NODE_LOOKUP:
	    return duck_node_lookup_invoke((duck_node_lookup_t *)this, stack);	    

	case DUCK_NODE_STRING_LITERAL:
	   return duck_node_string_literal_invoke((duck_node_string_literal_t *)this, stack);

	case DUCK_NODE_CHAR_LITERAL:
	   return duck_node_char_literal_invoke((duck_node_char_literal_t *)this, stack);

	case DUCK_NODE_NULL:
	    return null_object;

	case DUCK_NODE_ASSIGN:
	   return duck_node_assign_invoke((duck_node_assign_t *)this, stack);

	case DUCK_NODE_MEMBER_GET:
	   return duck_node_member_get_invoke((duck_node_member_get_t *)this, stack);

	case DUCK_NODE_MEMBER_GET_WRAP:
	   return duck_node_member_get_wrap_invoke((duck_node_member_get_t *)this, stack);

	default:
	    wprintf(L"HOLP\n");
	    exit(1);
    }
    
}

/*
duck_object_t *duck_call(duck_stack_frame_t *stack, duck_object_t *obj)
{
    duck_object *call = *duck_get_member_addr_int(obj, DUCK_MID_CALL);
    duck_node *node = (duck_node *)*duck_get_member_addr_int(obj, DUCK_MID_NODE);
    return node->invoke(node, stack);
}
*/

duck_object_t *duck_i_construct(duck_object_t **param)
{
  duck_object_t *type_wrapper = param[0];
  duck_type_t *type = duck_type_unwrap(type_wrapper);
  duck_object_t *result = duck_object_create(type);
    
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

duck_node_t *duck_i_member_get(duck_node_call_t *in, duck_function_t *func, duck_node_list_t *parent)
{
/*
   wprintf(L"member_get on node at %d\n", in);
   duck_node_print((duck_node_t *)in);
   wprintf(L"\n");
*/
   assert(in->child_count == 2);
   duck_prepare_children(in, func, parent);

   duck_type_t *object_type = duck_node_get_return_type(in->child[0], func->stack_template);
   duck_node_lookup_t *name_node = node_cast_lookup(in->child[1]);
   size_t mid = duck_mid_get(name_node->name);
   duck_type_t *member_type = duck_type_member_type_get(object_type, name_node->name);
   
   int wrap = !!duck_static_member_addr_get_mid(member_type, DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
   
   return (duck_node_t *)duck_node_member_get_create(in->source_filename, 
						     in->source_position,
						     in->child[0], 
						     mid,
						     member_type,
						     wrap);
//      return duck_method_wrap(result, obj);
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

duck_node_t *duck_i_block(duck_node_call_t *node, duck_function_t *func, duck_node_list_t *parent)
{
    //wprintf(L"Create new block with %d elements at %d\n", node->child_count, node);
    return (duck_node_t *)duck_node_dummy_create(node->source_filename,
						      node->source_position,
						 duck_function_create(L"!anonymous", 0, node, null_type, 0, 0, 0, func->stack_template)->wrapper,
	1);
}

duck_type_t *duck_sniff_return_type(duck_node_call_t *body)
{
    /*
      FIXME: Actually do some sniffing...
     */
  return int_type;
  
}


duck_node_t *duck_i_function(duck_node_call_t *node, duck_function_t *func, duck_node_list_t *parent)
{
    wchar_t *name=0;
    wchar_t *internal_name=0;
    assert(node->child_count==5);
    
    if (node->child[0]->node_type == DUCK_NODE_LOOKUP) {
	duck_node_lookup_t *name_lookup = (duck_node_lookup_t *)node->child[0];
	internal_name = name = name_lookup->name;
    }
    else {
	assert(node->child[0]->node_type == DUCK_NODE_NULL);
	internal_name = L"!anonymous";
    }
    
    duck_node_t *body = node->child[3];
    
    assert(body->node_type == DUCK_NODE_NULL ||body->node_type == DUCK_NODE_CALL);
    
    duck_type_t *out_type=0;
    duck_node_t *out_type_wrapper = node->child[1];
    if(out_type_wrapper->node_type == DUCK_NODE_NULL) 
    {
	assert(body->node_type == DUCK_NODE_CALL);
	out_type = duck_sniff_return_type((duck_node_call_t *)body);
    }
    else
    {
	duck_node_lookup_t *type_lookup;
	type_lookup = node_cast_lookup(out_type_wrapper);
	duck_object_t *type_wrapper = duck_stack_get_str(func->stack_template, type_lookup->name);
	assert(type_wrapper);
	out_type = duck_type_unwrap(type_wrapper);
    }
    
    size_t argc=0;
    duck_type_t **argv=0;
    wchar_t **argn=0;
    
    duck_node_call_t *declarations = node_cast_call(node->child[2]);
    int i;
    if(declarations->child_count > 0)
    {
	argc = declarations->child_count;
	argv = malloc(sizeof(duck_type_t *)*declarations->child_count);
	argn = malloc(sizeof(wchar_t *)*declarations->child_count);
	
	for(i=0; i<declarations->child_count; i++)
	{
	    duck_node_call_t *decl = node_cast_call(declarations->child[i]);
	    duck_node_lookup_t *name = node_cast_lookup(decl->child[0]);
	    duck_node_lookup_t *type_name = node_cast_lookup(decl->child[1]);
	    duck_object_t *type_wrapper = duck_stack_get_str(func->stack_template, type_name->name);
	    assert(type_wrapper);
	    argv[i] = duck_type_unwrap(type_wrapper);
	    argn[i] = name->name;
	}
    }

    duck_object_t *result;
    if(body->node_type == DUCK_NODE_CALL) {
	result = duck_function_create(internal_name, 0, (duck_node_call_t *)body, null_type, argc, argv, argn, func->stack_template)->wrapper;
    }
    else {
	result = null_object;
    }
    
    if(name) {
	duck_stack_declare(func->stack_template, name, duck_type_for_function(out_type, argc, argv), result);
    }
    return (duck_node_t *)duck_node_dummy_create(node->source_filename,
						 node->source_position,
						 result,
						 1);
}

int duck_abides(duck_type_t *contender, duck_type_t *role_model)
{
    return contender == role_model;
}


duck_node_t *duck_i_operator_wrapper(duck_node_call_t *in, duck_function_t *func, duck_node_list_t *parent)
{
   assert(in->child_count == 2);
   duck_prepare_children(in, func, parent);
    duck_node_lookup_t *name_lookup = node_cast_lookup(in->function);
    wchar_t *name = wcsdup(name_lookup->name+2);
    name[wcslen(name)-2] = 0;
    //wprintf(L"Calling operator_wrapper as %ls\n", name);
    assert(in->child_count == 2);
    
    duck_type_t * t1 = duck_node_get_return_type(in->child[0], func->stack_template);
    duck_type_t * t2 = duck_node_get_return_type(in->child[1], func->stack_template);
    
//    wprintf(L"Calling with types %ls and %ls\n", t1->name, t2->name);

    string_buffer_t buff;
    sb_init(&buff);
    sb_append(&buff, L"__");
    sb_append(&buff, name);
    sb_append(&buff, t2->name);
    sb_append(&buff, L"__");

    wchar_t *method_name = (wchar_t *)buff.buff;
    int res = !!hash_get(&t1->name_lookup, method_name);
    //sb_destroy(&buff);
    if(!res)
    {
	wprintf(L"Error: __%ls__: No support for call with objects of types %ls and %ls\n",
		name, t1->name, t2->name);
	duck_stack_print(func->stack_template);
	exit(1);
    }

    duck_node_t *mg_param[2]=
	{
	    in->child[0], (duck_node_t *)duck_node_lookup_create(in->source_filename, in->source_position, method_name)
	}
    ;

    duck_node_t *c_param[1]=
	{
	    in->child[1]
	}
    ;

    return (duck_node_t *)
	duck_node_call_create(in->source_filename,
			      in->source_position,
			      (duck_node_t *)
			      duck_node_call_create(in->source_filename,
						    in->source_position,
						    (duck_node_t *)
						    duck_node_lookup_create(in->source_filename,
									    in->source_position,
									    L"__memberGet__"),
						    2,
						    mg_param),
			      1,
			      c_param);
}

size_t duck_parent_count(struct duck_node_list *parent)
{
   return parent?1+duck_parent_count(parent->parent):0;
}

duck_node_t *duck_i_declare(struct duck_node_call *node, 
			    struct duck_function *function,
			    struct duck_node_list *parent)
{
   assert(node->child_count == 3);
   assert(duck_parent_count(parent)==1);
   duck_prepare_children(node, function, parent);
   duck_node_lookup_t *name_lookup = node_cast_lookup(node->child[0]);
   duck_type_t *type;
    switch(node->child[1]->node_type) 
    {
	case DUCK_NODE_LOOKUP:
	{
	    duck_node_lookup_t *type_lookup;
	    type_lookup = node_cast_lookup(node->child[1]);
	    duck_object_t *type_wrapper = duck_stack_get_str(function->stack_template, type_lookup->name);
	    assert(type_wrapper);
	    type = duck_type_unwrap(type_wrapper);
	    break;
	}
	
       case DUCK_NODE_NULL:	
	   type = duck_node_get_return_type(node->child[2], function->stack_template);
	   //wprintf(L"Implicit var dec type: %ls\n", type->name);
	   break;

       default:
	  wprintf(L"Dang, wrong type thing\n");
	  exit(1);
    }
    assert(type);
    duck_stack_declare(function->stack_template, name_lookup->name, type, null_object);

    duck_node_t *a_param[2]=
	{
	   node->child[0],
	   node->child[2]
	}
    ;
    
    return (duck_node_t *)
       duck_node_call_create(node->source_filename, 
			     node->source_position,
			     (duck_node_t *)duck_node_lookup_create(node->source_filename,
								    node->source_position,
								    L"__assign__"),
			     2,
			     a_param);
}

duck_node_t *duck_i_assign(struct duck_node_call *node, 
			   struct duck_function *function,
			   struct duck_node_list *parent)
{
   assert(node->child_count == 2);
   assert(duck_parent_count(parent)==1);
   duck_prepare_children(node, function, parent);

   duck_node_lookup_t *name_lookup = node_cast_lookup(node->child[0]);
   duck_sid_t sid = duck_stack_sid_create(function->stack_template, name_lookup->name);
   
   return (duck_node_t *)
      duck_node_assign_create(node->source_filename, 
			      node->source_position,
			      sid,
			      node->child[1]);
}

duck_object_t *duck_i_while(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    assert(node->child_count == 2);
    duck_object_t *result = null_object;
    duck_object_t *body_object = duck_node_invoke(node->child[1], stack_global);
    while(1)
      {
	duck_object_t *test = duck_node_invoke(node->child[0], stack);
	if(test == null_object) {
	    break;
	}
	result = duck_function_wrapped_invoke(body_object, 0, stack);
      }
    return result;
}

duck_object_t *duck_i_if(duck_object_t **param)
{
    duck_object_t *body_object;
    if(param[0]!=null_object)
    {
	body_object=param[1];
    }
    else
    {
	body_object=param[2];
    }
    
    return duck_function_wrapped_invoke(body_object, 0, stack_global);
}

duck_object_t *duck_i_print(duck_object_t **param)
{
    int i;
    //    for(i=0; i<node->child_count; i++) {
    duck_object_t *value = param[0];
	if(value->type == int_type) {
	    int val = duck_i_int_get(value);
	    wprintf(L"%d", val);
	}
	else if(value->type == float_type) {
	    double val = duck_i_float_get(value);
	    wprintf(L"%f", val);
	}
	else if(value->type == string_type) {
	    wchar_t *payload = duck_i_string_get_payload(value);
	    size_t payload_size = duck_i_string_get_payload_size(value);
	    wprintf(L"%.*ls", payload_size, payload);
	}
	else if(value->type == char_type) {
	    wchar_t payload = duck_i_char_get(value);
	    wprintf(L"%lc", payload);
	}
	else if(value == null_object) {
	    wprintf(L"null");
	}
	else 
	{
	    wprintf(L"WAAAH\n");
	}
	/*
	  FIXME: Print other things than just ints!
	*/
	//    }
    return null_object;
}
/*
duck_object_t *duck_i_member_set(duck_node_t *node_base, duck_stack_frame_t *stack)
{
    duck_node_call_t *node = node_cast_call(node_base);
    duck_object_t *obj = duck_node_invoke(node->child[0], stack);
    duck_node_lookup_t *name_node = node_cast_lookup(node->child[1]);
    wchar_t *name = this->child[1]->name;
    duck_object_t **addr = duck_member_addr_get_str(obj, name);
    *addr = this->children[2]->invoke(this->children[2], stack);
}
*/
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
    duck_object_t *result = duck_i_int_create();
    duck_i_int_set(result, this2->payload);
    return result;
}

void duck_node_print(duck_node_t *this)
{
    switch(this->node_type)
    {
	case DUCK_NODE_INT_LITERAL:
	{
	    duck_node_int_literal_t *this2 = (duck_node_int_literal_t *)this;
	    wprintf(L"%d", this2->payload);
	    break;
	}
	
	case DUCK_NODE_FLOAT_LITERAL:
	{
	    duck_node_float_literal_t *this2 = (duck_node_float_literal_t *)this;
	    wprintf(L"%f", this2->payload);
	    break;
	}
	
	case DUCK_NODE_STRING_LITERAL:
	{
	    duck_node_string_literal_t *this2 = (duck_node_string_literal_t *)this;
	    wprintf(L"\"%.*ls\"", this2->payload_size, this2->payload);
	    break;
	}
	
	case DUCK_NODE_CHAR_LITERAL:
	{
	    duck_node_char_literal_t *this2 = (duck_node_char_literal_t *)this;
	    wprintf(L"'%lc'", this2->payload);
	    break;
	}
	
	case DUCK_NODE_LOOKUP:
	{
	    duck_node_lookup_t *this2 = (duck_node_lookup_t *)this;
	    wprintf(L"%ls", this2->name);
	    break;
	}
	
	case DUCK_NODE_NULL:
	{
	    wprintf(L"null");
	    break;
	}
	
	case DUCK_NODE_CALL:
	{
	    duck_node_call_t *this2 = (duck_node_call_t *)this;	    
	    int i;
	    duck_node_print(this2->function);
	    wprintf(L"(");
	    for(i=0; i<this2->child_count; i++)
	    {
		if(i!=0) 
		{
		    wprintf(L"; ");
		}
		duck_node_print(this2->child[i]);
	    }
	    
	    wprintf(L")" );
	    break;
	}
	
	default:
	{
	    wprintf(L"HILF");
	    break;
	}
    }
}

duck_object_t *duck_i_int_gt(duck_object_t **param)
{
    int v1 = duck_i_int_get(param[0]);
    int v2 = duck_i_int_get(param[1]);
    return v1>v2?param[0]:null_object;
}


duck_object_t *duck_i_int_lt(duck_object_t **param)
{
    int v1 = duck_i_int_get(param[0]);
    int v2 = duck_i_int_get(param[1]);
    return v1<v2?param[0]:null_object;
}


duck_object_t *duck_i_int_eq(duck_object_t **param)
{
    int v1 = duck_i_int_get(param[0]);
    int v2 = duck_i_int_get(param[1]);
    return v1==v2?param[0]:null_object;
}


duck_object_t *duck_i_int_gte(duck_object_t **param)
{
    int v1 = duck_i_int_get(param[0]);
    int v2 = duck_i_int_get(param[1]);
    return v1>=v2?param[0]:null_object;
}


duck_object_t *duck_i_int_lte(duck_object_t **param)
{
    int v1 = duck_i_int_get(param[0]);
    int v2 = duck_i_int_get(param[1]);
    return v1<=v2?param[0]:null_object;
}


duck_object_t *duck_i_int_neq(duck_object_t **param)
{
    int v1 = duck_i_int_get(param[0]);
    int v2 = duck_i_int_get(param[1]);
    return v1!=v2?param[0]:null_object;
}


duck_object_t *duck_i_int_add(duck_object_t **node)
{
  duck_object_t *result = duck_i_int_create();
  int v1 = duck_i_int_get(node[0]);
  int v2 = duck_i_int_get(node[1]);
  duck_i_int_set(result, v1+v2);
  return result;
}
duck_object_t *duck_i_int_sub(duck_object_t **node)
{
  duck_object_t *result = duck_i_int_create();
  int v1 = duck_i_int_get(node[0]);
  int v2 = duck_i_int_get(node[1]);
  duck_i_int_set(result, v1-v2);
  return result;
}
duck_object_t *duck_i_int_mul(duck_object_t **node)
{
  duck_object_t *result = duck_i_int_create();
  int v1 = duck_i_int_get(node[0]);
  int v2 = duck_i_int_get(node[1]);
  duck_i_int_set(result, v1*v2);
  return result;
}
duck_object_t *duck_i_int_div(duck_object_t **node)
{
  duck_object_t *result = duck_i_int_create();
  int v1 = duck_i_int_get(node[0]);
  int v2 = duck_i_int_get(node[1]);
  duck_i_int_set(result, v1/v2);
  return result;
}


duck_object_t *duck_i_int_create(int value)
{
    duck_object_t *obj= duck_object_create(int_type);
    duck_i_int_set(obj, value);
    return obj;
}

duck_object_t *duck_i_float_create(double value)
{
    duck_object_t *obj= duck_object_create(float_type);
    duck_i_float_set(obj, value);
    return obj;
}

duck_object_t *duck_i_char_create(wchar_t value)
{
    duck_object_t *obj= duck_object_create(char_type);
    duck_i_char_set(obj, value);
    return obj;
}

duck_object_t *duck_i_string_create(size_t sz, wchar_t *data)
{
    duck_object_t *obj= duck_object_create(string_type);
    memcpy(duck_member_addr_get_mid(obj,DUCK_MID_STRING_PAYLOAD_SIZE), &sz, sizeof(size_t));
    wchar_t *res = malloc(sizeof(wchar_t) * sz);
    memcpy(res, data, sizeof(wchar_t) * sz);
    memcpy(duck_member_addr_get_mid(obj,DUCK_MID_STRING_PAYLOAD), &res, sizeof(wchar_t *));
    return obj;
}

void duck_i_int_set(duck_object_t *this, int value)
{
    memcpy(duck_member_addr_get_mid(this,DUCK_MID_INT_PAYLOAD), &value, sizeof(int));
}

void duck_i_float_set(duck_object_t *this, double value)
{
    memcpy(duck_member_addr_get_mid(this,DUCK_MID_FLOAT_PAYLOAD), &value, sizeof(double));
}

void duck_i_char_set(duck_object_t *this, wchar_t value)
{
    memcpy(duck_member_addr_get_mid(this,DUCK_MID_CHAR_PAYLOAD), &value, sizeof(wchar_t));
}

int duck_i_int_get(duck_object_t *this)
{
  int result;
  memcpy(&result, duck_member_addr_get_mid(this,DUCK_MID_INT_PAYLOAD), sizeof(int));
  return result;
}

double duck_i_float_get(duck_object_t *this)
{
    double result;
    memcpy(&result, duck_member_addr_get_mid(this,DUCK_MID_FLOAT_PAYLOAD), sizeof(double));
    return result;
}

wchar_t *duck_i_string_get_payload(duck_object_t *this)
{
    wchar_t *result;
    memcpy(&result, duck_member_addr_get_mid(this,DUCK_MID_STRING_PAYLOAD), sizeof(wchar_t *));
    return result;
}

size_t duck_i_string_get_payload_size(duck_object_t *this)
{
    size_t result;
    memcpy(&result, duck_member_addr_get_mid(this,DUCK_MID_STRING_PAYLOAD_SIZE), sizeof(size_t));
    return result;
}

wchar_t duck_i_char_get(duck_object_t *this)
{
    wchar_t result;
    memcpy(&result, duck_member_addr_get_mid(this,DUCK_MID_CHAR_PAYLOAD), sizeof(wchar_t));
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
    if(creatable) 
    {
	duck_native_method_create(result, DUCK_MID_CALL_PAYLOAD, L"__call__",
				  0, (duck_native_t)&duck_i_construct,
				  result, 0, 0, 0);
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

static duck_type_t *duck_type_create(wchar_t *name, size_t static_member_count)
{
    duck_type_t *result = duck_type_create_raw(name, static_member_count);
    duck_type_wrapper_create(result, 1);
}
			  
static duck_object_t *duck_object_create(duck_type_t *type) {
   duck_object_t *result = calloc(1,sizeof(duck_object_t)+sizeof(duck_object_t *)*type->member_count);
   result->type = type;
  
   return result;
}

static size_t duck_member_create(duck_type_t *type,
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
	  wprintf(L"Error, multiple mids for name %ls\n", name);
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

static size_t duck_native_method_create(duck_type_t *type,
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

static void duck_type_type_create()
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

static void duck_null_type_create()
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

static void duck_int_type_create()
{
    int_type->member_count = 1;
    duck_member_create(int_type, DUCK_MID_INT_PAYLOAD,  L"!intPayload", 0, null_type);
    
    duck_type_t *argv[]=
	{
	    int_type, int_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L"this", L"param"
	}
    ;
    
    duck_native_method_create(int_type, -1, L"__addInt__", 0, (duck_native_t)&duck_i_int_add, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__subInt__", 0, (duck_native_t)&duck_i_int_sub, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__mulInt__", 0, (duck_native_t)&duck_i_int_mul, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__divInt__", 0, (duck_native_t)&duck_i_int_div, int_type, 2, argv, argn);
    
    duck_native_method_create(int_type, -1, L"__gtInt__", 0, (duck_native_t)&duck_i_int_gt, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__ltInt__", 0, (duck_native_t)&duck_i_int_lt, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__eqInt__", 0, (duck_native_t)&duck_i_int_eq, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__gteInt__", 0, (duck_native_t)&duck_i_int_gte, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__lteInt__", 0, (duck_native_t)&duck_i_int_lte, int_type, 2, argv, argn);
    duck_native_method_create(int_type, -1, L"__neqInt__", 0, (duck_native_t)&duck_i_int_neq, int_type, 2, argv, argn);

}

static void duck_float_type_create()
{
    /*
      FIXME, UGLY: Count and add payload twice, because it is twice the size of ptr. *ugh*
    */
    float_type->member_count = 2;
    duck_member_create(float_type, DUCK_MID_FLOAT_PAYLOAD,  L"!floatPayload", 0, null_type);
    duck_member_create(float_type, -1,  L"!floatPayload2", 0, null_type);

/*    
    duck_add_method(float_type, null_type, L"__addFloat__", -1, &duck_i_float_add);
    duck_add_method(float_type, null_type, L"__subtractFloat__", -1, &duck_i_float_subtract);
    duck_add_method(float_type, null_type, L"__multiplyFloat__", -1, &duck_i_float_multiply);
    duck_add_method(float_type, null_type, L"__divideFloat__", -1, &duck_i_float_divide);
*/  
}

static void duck_string_type_create()
{
    string_type->member_count = 2;

    duck_member_create(string_type, DUCK_MID_STRING_PAYLOAD,  L"!stringPayload", 0, null_type);
    duck_member_create(string_type, DUCK_MID_STRING_PAYLOAD_SIZE,  L"!stringPayloadSize", 0, null_type);
}

static void duck_char_type_create()
{
    char_type->member_count = 1;
    duck_member_create(char_type, DUCK_MID_CHAR_PAYLOAD,  L"!charPayload", 0, null_type);
}

static void duck_object_type_create()
{
    object_type->member_count = 0;
}

static void duck_list_type_create()
{
    list_type->member_count = 0;
}


static void duck_i_init()
{
    int i;
    
    hash_init(&duck_mid_lookup, &hash_wcs_func, &hash_wcs_cmp);
    hash_init(&duck_type_for_function_lookup, &hash_function_type_func, &hash_function_type_comp);
    
    duck_mid_put(L"!typeWrapperPayload", DUCK_MID_TYPE_WRAPPER_PAYLOAD);
    duck_mid_put(L"!callPayload", DUCK_MID_CALL_PAYLOAD);
    duck_mid_put(L"!stringPayload", DUCK_MID_STRING_PAYLOAD);
    duck_mid_put(L"!stringPayloadSize", DUCK_MID_STRING_PAYLOAD_SIZE);
    duck_mid_put(L"!charPayload", DUCK_MID_CHAR_PAYLOAD);
    duck_mid_put(L"!intPayload", DUCK_MID_INT_PAYLOAD);
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
    int_type = duck_type_create_raw(L"Int", 64);
    string_type = duck_type_create_raw(L"String", 64);
    char_type = duck_type_create_raw(L"Char", 64);
    list_type = duck_type_create_raw(L"List", 64);
    float_type = duck_type_create_raw(L"Float", 64);
    
    duck_type_type_create();
    duck_object_type_create();
    duck_null_type_create();
    duck_int_type_create();
    duck_char_type_create();
    duck_string_type_create();
    duck_list_type_create();
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
    duck_stack_declare(stack_global, L"Int", type_type, int_type->wrapper);
    duck_stack_declare(stack_global, L"Type", type_type, type_type->wrapper);
    

    null_object = duck_object_create(null_type);
    
    static wchar_t *p_argn[]={L"object"};
    duck_native_declare(stack_global, L"print", DUCK_FUNCTION_FUNCTION, (duck_native_t)&duck_i_print, null_type, 1, &object_type, p_argn);
    
    duck_native_declare(stack_global, L"__block__", DUCK_FUNCTION_MACRO, (duck_native_t)&duck_i_block, object_type, 0, 0, 0);

    duck_native_declare(stack_global, L"__memberGet__", DUCK_FUNCTION_MACRO, (duck_native_t)&duck_i_member_get, 0, 0, 0, 0);

    duck_native_declare(stack_global, L"__assign__", DUCK_FUNCTION_MACRO, (duck_native_t)&duck_i_assign, 0, 0, 0, 0);

    duck_native_declare(stack_global, L"__declare__", DUCK_FUNCTION_MACRO, (duck_native_t)&duck_i_declare, 0, 0, 0, 0);
    
    duck_type_t *if_argv[]={object_type, object_type, object_type};
    static wchar_t *if_argn[]={L"condition", L"trueBlock", L"falseBlock"};    
    duck_native_declare(stack_global, L"__if__", DUCK_FUNCTION_FUNCTION, (duck_native_t)&duck_i_if, object_type, 3, if_argv, if_argn);

    duck_native_declare(stack_global, L"__function__", DUCK_FUNCTION_MACRO, (duck_native_t)&duck_i_function, 0, 0, 0, 0);
    
    wchar_t *op_names[] = 
       {
	    L"__join__",
	    L"__or__",
	    L"__and__",
	    L"__append__",
	    L"__format__",
	    L"__add__",
	    L"__sub__",
	    L"__mul__",
	    L"__div__",
	    L"__join__",
	    L"__gt__",
	    L"__lt__",
	    L"__eq__",
	    L"__gte__",
	    L"__lte__",
	    L"__neq__"
	}
    ;

    for(i =0; i<sizeof(op_names)/sizeof(wchar_t *); i++)
    {
	duck_native_declare(stack_global, op_names[i], DUCK_FUNCTION_MACRO, (duck_native_t)&duck_i_operator_wrapper,0,0,0,0);
    }

    /*
      duck_native_declare(stack_global, L"while", &duck_i_while, object_type, 0, 0);
    */
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
	    wprintf(L"AAAAAAAAA, Macro %ls at invoke!!!!\n", function->name);
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
		
		//wprintf(L"Create new stack @%d, with parent %d\n", my_stack, stack);
		
		//duck_stack_print(stack);
		
		/*
		  FIXME:
		  Handle inner function stacks correctly
		  Set stack size correctly
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
		    //wprintf(L"Set %ls on stack %d\n", function->input_name[i], my_stack);
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
    duck_i_init();
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
    
    duck_node_t *program_callable = (duck_node_t *)
       duck_node_dummy_create(program->source_filename,
			      program->source_position,
			      duck_function_create(L"!anonymous",
						   0,
						   node_cast_call(program),
						   null_type, 0, 0, 0, stack_global)->wrapper,
			      0);
    
    duck_object_t *program_object = duck_node_invoke(program_callable, stack_global);
    
    //duck_object_t *program_object = duck_node_invoke(program, stack_global);
    
    //duck_object_print(program_object, 3);
    
    duck_function_t *func=duck_function_unwrap(program_object);    
    assert(func);
    duck_function_invoke(func, 0, stack_global, stack_global);
    
    wprintf(L"\n");
}
