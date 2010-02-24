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
  
  Simple type checking
  Type template support
  Complex types
  Type support for lists
  Type checking
  
  Implement basic string methods
  Implement basic list methods
  Implement int comparison methods
  Implement string comparison methods
  Implement basic char methods
  Implement char comparison methods
  
  Function default argument values
  Named function arguments
  Inner functions with shared return flag, stack, etc.
  Variadic functions
  
  if function
  __type__ function
  each function
  extends function
  abides function
  __add__, __sub__, __mul__ and friends, full abides checking implementation
  __return__ function
  __assignReturn__ function  
  __templatize__ function
  template function
  __list__ function
  
  Done: 
  
  Sugar parser

  Variable declarations
  Function argument passing
  Method calls with proper this handling
  
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
  
  __block__ function
  __assign__ function
  __declare__ function
  __function__ function  
  __if__ function
  while function
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
#define DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD 10
#define DUCK_MID_TYPE_WRAPPER_PAYLOAD 11
#define DUCK_MID_CALL 12
#define DUCK_MID_FIRST_UNRESERVED 13

typedef struct 
{
    duck_type_t *result; 
    size_t argc;
    duck_type_t *argv[];
} duck_function_type_key_t;

    

static duck_type_t *type_type=0, *object_type=0, *int_type=0, *string_type=0, *char_type=0, *null_type=0, *call_type, *string_type, *char_type, *list_type, *float_type;
static duck_object_t *null_object=0;
static hash_table_t duck_type_for_function_lookup;

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

duck_object_t *duck_function_invoke(duck_function_t *function, duck_node_call_t *param, duck_stack_frame_t *stack);
static duck_type_t *duck_type_create(wchar_t *name, size_t static_member_count);
static duck_type_t *duck_type_create_raw(wchar_t *name, size_t static_member_count);
static void duck_type_wrapper_create(duck_type_t *result, int creatable);

duck_object_t *duck_i_function_wrapper_call(duck_node_call_t *node, duck_stack_frame_t *stack);
void duck_object_print(duck_object_t *obj, int level);


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
//     FIXME: Validity checking, etc.
  
}

static int duck_is_member_get(duck_node_t *node)
{
    if(node->node_type != DUCK_NODE_LOOKUP)
	return 0;
    duck_node_lookup_t *node2 = (duck_node_lookup_t *)node;
    return wcscmp(node2->name, L"__memberGet__")==0;
    
}


duck_object_t *duck_node_call_invoke(duck_node_call_t *this, duck_stack_frame_t *stack)
{
    duck_object_t *obj = duck_node_invoke(this->function, stack);
    if(obj == null_object){
	return obj;
    }
    
    duck_function_t *func=duck_function_unwrap(obj);
    
    return duck_function_invoke(func, this, stack);
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

duck_object_t *duck_node_invoke(duck_node_t *this, duck_stack_frame_t *stack)
{
//    wprintf(L"invoke %d\n", this->node_type);    
    switch(this->node_type)
    {
	case DUCK_NODE_CALL:
	   return duck_node_call_invoke((duck_node_call_t *)this, stack);

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

duck_type_t *duck_unwrap_type(duck_object_t *type_wrapper)
{
   /*
     FIXME: Add unwrapping implementation
    */
    wprintf(L"GULP\n");
    exit(1);    
}


duck_object_t *duck_i_construct(duck_object_t **param)
{
  duck_object_t *type_wrapper = param[0];
  duck_type_t *type = duck_unwrap_type(type_wrapper);
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
    memcpy(duck_member_addr_get_mid(function_copy->wrapper,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD), &function_copy, sizeof(duck_function_t *));
    return function_copy->wrapper;
}


duck_object_t *duck_i_member_get(duck_node_call_t *this, duck_stack_frame_t *stack)
{
/*
    wprintf(L"member_get on node at %d\n", this);
    duck_node_print((duck_node_t *)this);
    wprintf(L"\n");
*/  
    duck_object_t *obj = duck_node_invoke(this->child[0], stack);
    duck_node_lookup_t *name_node = node_cast_lookup(this->child[1]);
    duck_object_t *result = *duck_member_addr_get_str(obj, name_node->name);

    if (duck_static_member_addr_get_mid(result->type, DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD)) 
    {
	wprintf(L"I think %ls is a function!!!\n", name_node->name);
	return duck_method_wrap(result, obj);
    }

    return result;
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
    
    duck_function_t *func=duck_function_unwrap(obj);    
    return duck_function_invoke(func, this, stack);    
}

void duck_prepare_function(duck_function_t *code)
{
  /*
    FIXME:
    * Validate
    * Short circut name lookups
    * Replace memberGet + call with memberCall
    * 
    */

  
}


duck_function_t *duck_function_create(wchar_t *name,
				      int flags,
				      duck_node_call_t *body, 
				      duck_type_t *return_type,
				      size_t in_count,
				      duck_type_t **in_type,
				      wchar_t **in_name)				      
{
    if(!flags) {
	assert(return_type);
	if(in_count) {
	    assert(in_type);
	    assert(in_name);
	}
    }

    duck_function_t *result = calloc(1,sizeof(duck_function_t) + in_count*sizeof(duck_type_t *));
    result->native.function=0;
    result->flags=flags;
    result->name = name;
    result->body = body;
    result->return_type=return_type;
    result->input_count=in_count;
    memcpy(&result->input_type, in_type, sizeof(duck_type_t *)*in_count);
    result->input_name = in_name;

    duck_type_t *function_type = duck_type_for_function(return_type, in_count, in_type);
    result->type = function_type;    
    result->wrapper = duck_object_create(function_type);
    memcpy(duck_member_addr_get_mid(result->wrapper,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD), &result, sizeof(duck_function_t *));
    //wprintf(L"Function object is %d, wrapper is %d\n", result, result->wrapper);
    duck_prepare_function(result);
    
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
	duck_object_print(result->wrapper, 3);
	CRASH;
	
    }
    
    memcpy(duck_member_addr_get_mid(result->wrapper,DUCK_MID_FUNCTION_WRAPPER_PAYLOAD), &result, sizeof(duck_function_t *));
    return result;
}

duck_object_t *duck_i_block(duck_node_call_t *node, duck_stack_frame_t *stack)
{
  //wprintf(L"Create new block with %d elements at %d\n", node->child_count, node);
  return duck_function_create(L"!anonymous", 0, node, null_type, 0, 0, 0)->wrapper;
}

duck_type_t *duck_sniff_return_type(duck_node_call_t *body)
{
  return int_type;
  
}


duck_object_t *duck_i_function(duck_node_call_t *node, duck_stack_frame_t *stack)
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
	assert(out_type_wrapper->node_type == DUCK_NODE_LOOKUP);
	out_type = duck_type_unwrap(duck_node_invoke(out_type_wrapper, stack));
    }
    
/*
  FIXME: FILL THESE IN!!!
 */
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
	    duck_object_t *type_wrapper = duck_stack_get_str(stack, type_name->name);
	    assert(type_wrapper);
	    argv[i] = duck_type_unwrap(type_wrapper);
	    argn[i] = name->name;
	}
    }

    duck_object_t *result;
    
    if(body->node_type == DUCK_NODE_CALL) {
	result = duck_function_create(internal_name, 0, (duck_node_call_t *)body, null_type, argc, argv, argn)->wrapper;
    }
    else {
	result = null_object;
    }
    
    if(name) {
	duck_stack_declare(stack, name, duck_type_for_function(out_type, argc, argv), result);
    }
    return result;
}

int duck_abides(duck_type_t *contender, duck_type_t *role_model)
{
    return contender == role_model;
}


duck_node_t *duck_i_operator_wrapper(duck_node_call_t *in, duck_stack_frame_t *stack)
{
    duck_node_lookup_t *name_lookup = node_cast_lookup(in->function);
    wchar_t *name = wcsdup(name_lookup->name+2);
    name[wcslen(name)-2] = 0;
    wprintf(L"Calling operator_wrapper as %ls\n", name);
    assert(in->child_count == 2);
    duck_type_t * t1 = duck_node_get_return_type(in->child[0], stack);
    duck_type_t * t2 = duck_node_get_return_type(in->child[1], stack);
    
    wprintf(L"Calling with types %ls and %ls\n", t1->name, t2->name);

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
	exit(1);
    }
    /*
      __add__(foo, bar)
      =>
      __memberGet(foo,__addInt__)(bar)
     */
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
    
    return (duck_node_t *)duck_node_call_create(in->source_filename,
				 in->source_position,
				 (duck_node_t *)duck_node_call_create(in->source_filename,
						       in->source_position,
						       (duck_node_t *)duck_node_lookup_create(in->source_filename,
									       in->source_position,
									       L"__memberGet__"),
						       2,
						       mg_param),
				 1,
	c_param);
}


duck_object_t *duck_i_declare(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    assert(node->child_count == 3);
    duck_node_lookup_t *name_lookup = node_cast_lookup(node->child[0]);
    duck_object_t *value = duck_node_invoke(node->child[2], stack);
    duck_type_t *type;
    switch(node->child[1]->node_type) 
    {
	case DUCK_NODE_LOOKUP:
	{
	    duck_node_lookup_t *type_lookup;
	    type_lookup = node_cast_lookup(node->child[1]);
	    duck_object_t *type_wrapper = duck_stack_get_str(stack, type_lookup->name);
	    assert(type_wrapper);
	    type = duck_type_unwrap(type_wrapper);
	    break;
	}
	
	case DUCK_NODE_NULL:	
	    type = duck_node_get_return_type(node->child[2], stack);
	    wprintf(L"Implicit var dec type: %ls\n", type->name);
	    break;

	default:
	    wprintf(L"Dang, wrong type thing\n");
	    exit(1);
    }
    
    assert(type);
    duck_stack_declare(stack, name_lookup->name, type, value);
    
    return value;
}

duck_object_t *duck_i_assign(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    assert(node->child_count == 2);
    duck_node_lookup_t *name_lookup = node_cast_lookup(node->child[0]);
    duck_object_t *value = duck_node_invoke(node->child[1], stack);
    duck_stack_set_str(stack, name_lookup->name, value);
    
    return value;
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
	duck_function_t *func=duck_function_unwrap(body_object);    
	result = duck_function_invoke(func, 0, stack_global);
      }
    return result;
}

duck_object_t *duck_i_if(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    assert(node->child_count == 3);
    duck_object_t *result = null_object;
    duck_object_t *test = duck_node_invoke(node->child[0], stack);
    duck_object_t *body_object = duck_node_invoke(node->child[test!=null_object?1:2], stack_global);
    duck_function_t *func=duck_function_unwrap(body_object);    
    return duck_function_invoke(func, 0, stack_global);
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

duck_object_t *duck_i_int_gt(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    duck_object_t *this = duck_node_invoke(node->child[0], stack);
    int v1 = duck_i_int_get(this);
    int v2 = duck_i_int_get(duck_node_invoke(node->child[1], stack));
    return v1>v2?this:null_object;
}

duck_object_t *duck_i_int_lt(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    duck_object_t *this = duck_node_invoke(node->child[0], stack);
    int v1 = duck_i_int_get(this);
    int v2 = duck_i_int_get(duck_node_invoke(node->child[1], stack));
    return v1<v2?this:null_object;
}

duck_object_t *duck_i_int_eq(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    duck_object_t *this = duck_node_invoke(node->child[0], stack);
    int v1 = duck_i_int_get(this);
    int v2 = duck_i_int_get(duck_node_invoke(node->child[1], stack));
    return v1==v2?this:null_object;
}

duck_object_t *duck_i_int_lte(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    duck_object_t *this = duck_node_invoke(node->child[0], stack);
    int v1 = duck_i_int_get(this);
    int v2 = duck_i_int_get(duck_node_invoke(node->child[1], stack));
    return v1<=v2?this:null_object;
}

duck_object_t *duck_i_int_gte(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    duck_object_t *this = duck_node_invoke(node->child[0], stack);
    int v1 = duck_i_int_get(this);
    int v2 = duck_i_int_get(duck_node_invoke(node->child[1], stack));
    return v1>=v2?this:null_object;
}

duck_object_t *duck_i_int_neq(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    duck_object_t *this = duck_node_invoke(node->child[0], stack);
    int v1 = duck_i_int_get(this);
    int v2 = duck_i_int_get(duck_node_invoke(node->child[1], stack));
    return v1!=v2?this:null_object;
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

duck_object_t *duck_i_function_wrapper_call(duck_node_call_t *node, duck_stack_frame_t *stack)
{
    assert(node->node_type == DUCK_NODE_CALL);    
    duck_object_t *result = null_object;
    int i;
    duck_stack_frame_t *my_stack = duck_stack_create(64, stack_global);
    wprintf(L"function_wrapper_call; %d expressions; %d\n", node->child_count, node);    
    
    /*
      FIXME:
      Do param assignments
      Handle inner function stacks correctly
      Set stack size correctly
      Support return statement
    */
    
    for(i=0; i<node->child_count; i++)
    {
	result = duck_node_invoke(node->child[i], my_stack);
    }
    
    return result;
    
    //wprintf(L"Called a non-native function. Don't know what to do about that, though. :-/");
    /*
      duck_node_print(node);
    */
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
	duck_native_method_create(result, DUCK_MID_TYPE_WRAPPER_PAYLOAD, L"__call__",
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

size_t duck_allocate_mid()
{
    return mid_pos++;
}

static size_t duck_member_create(duck_type_t *type,
				 ssize_t mid,
				 wchar_t *name,
				 int is_static,
				 duck_type_t *member_type)
{
    duck_member_t * member = calloc(1,sizeof(duck_member_t) + sizeof(wchar_t) * (wcslen(name)+1));

    wcscpy(member->name, name);
    
    if (mid == (ssize_t)-1) {
	mid = duck_allocate_mid();
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
    duck_member_create(int_type, DUCK_MID_INT_PAYLOAD,  L"!int_payload", 0, null_type);
    
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
    /*    
    duck_add_method(int_type, int_type, 2, argv, L"__subtractInt__", -1, &duck_i_int_subtract);
    duck_add_method(int_type, int_type, 2, argv, L"__multiplyInt__", -1, &duck_i_int_multiply);
    duck_add_method(int_type, int_type, 2, argv, L"__divideInt__", -1, &duck_i_int_divide);
    
    duck_add_method(int_type, int_type, 2, argv, L"__gtInt__", -1, &duck_i_int_gt);
    duck_add_method(int_type, int_type, 2, argv, L"__ltInt__", -1, &duck_i_int_lt);
    duck_add_method(int_type, int_type, 2, argv, L"__eqInt__", -1, &duck_i_int_eq);
    duck_add_method(int_type, int_type, 2, argv, L"__gteInt__", -1, &duck_i_int_gte);
    duck_add_method(int_type, int_type, 2, argv, L"__lteInt__", -1, &duck_i_int_lte);
    duck_add_method(int_type, int_type, 2, argv, L"__neqInt__", -1, &duck_i_int_neq);
    */
}

static void duck_float_type_create()
{
    /*
      FIXME, UGLY: Count and add payload twice, because it is twice the size of ptr. *ugh*
    */
    float_type->member_count = 2;
    duck_member_create(float_type, DUCK_MID_FLOAT_PAYLOAD,  L"!float_payload", 0, null_type);
    duck_member_create(float_type, -1,  L"!float_payload2", 0, null_type);

/*    
    duck_add_method(float_type, null_type, L"__addFloat__", -1, &duck_i_float_add);
    duck_add_method(float_type, null_type, L"__subtractFloat__", -1, &duck_i_float_subtract);
    duck_add_method(float_type, null_type, L"__multiplyFloat__", -1, &duck_i_float_multiply);
    duck_add_method(float_type, null_type, L"__divideFloat__", -1, &duck_i_float_divide);
*/  
}

static void duck_call_type_create()
{
   call_type->member_count = 1;
   duck_member_create(call_type, DUCK_MID_CALL_PAYLOAD,  L"!call_payload", 0, null_type);
}

static void duck_string_type_create()
{
    string_type->member_count = 2;

    duck_member_create(string_type, DUCK_MID_STRING_PAYLOAD,  L"!string_payload", 0, null_type);
    duck_member_create(string_type, DUCK_MID_STRING_PAYLOAD_SIZE,  L"!string_payload_size", 0, null_type);
}

static void duck_char_type_create()
{
    char_type->member_count = 1;
    duck_member_create(char_type, DUCK_MID_CHAR_PAYLOAD,  L"!char_payload", 0, null_type);
}

static void duck_object_type_create()
{
    object_type->member_count = 0;
}

static void duck_list_type_create()
{
    list_type->member_count = 0;
}

/*
static duck_object_t *duck_call_object_create(duck_node_call_t *node)
{
   duck_object_t * obj = duck_object_create(function_call_type);
   obj->
}
*/

static void duck_i_init()
{
    int i;
    
    hash_init(&duck_type_for_function_lookup, &hash_function_type_func, &hash_function_type_comp);
    
    stack_global = duck_stack_create(4096, 0);
    
    /*
      Create lowest level stuff. Bits of magic, be careful with
      ordering here. A lot of intricate dependencies going on between
      the various calls...
    */
    type_type = duck_type_create_raw(L"Type", 64);
    object_type = duck_type_create_raw(L"Object", 64);
    call_type = duck_type_create_raw(L"Call", 64);
    null_type = duck_type_create_raw(L"Null", 1);
    int_type = duck_type_create_raw(L"Int", 64);
    string_type = duck_type_create_raw(L"String", 64);
    char_type = duck_type_create_raw(L"Char", 64);
    list_type = duck_type_create_raw(L"List", 64);
    float_type = duck_type_create_raw(L"Float", 64);
    
    duck_type_type_create();
    duck_call_type_create();
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
    duck_type_wrapper_create(call_type, 1);
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
    duck_stack_declare(stack_global, L"Call", type_type, call_type->wrapper);
    duck_stack_declare(stack_global, L"Float", type_type, float_type->wrapper);
    duck_stack_declare(stack_global, L"Int", type_type, int_type->wrapper);
    duck_stack_declare(stack_global, L"Type", type_type, type_type->wrapper);
    
    null_object = duck_object_create(null_type);
    
    static wchar_t *p_argn[]={L"object"};
    duck_native_declare(stack_global, L"print", DUCK_FUNCTION_FUNCTION, (duck_native_t)&duck_i_print, null_type, 1, &object_type, p_argn);
    
    duck_native_declare(stack_global, L"__block__", DUCK_FUNCTION_SEMI_MACRO, (duck_native_t)&duck_i_block, object_type, 0, 0, 0);
    
    duck_type_t *mg_argv[]={object_type,0};
    static wchar_t *mg_argn[]={L"this",L"name"};    
    duck_native_declare(stack_global, L"__memberGet__", DUCK_FUNCTION_SEMI_MACRO, (duck_native_t)&duck_i_member_get, object_type, 2, mg_argv, mg_argn);
    
    duck_type_t *a_argv[]={0,object_type};
    static wchar_t *a_argn[]={L"variable",L"value"};    
    duck_native_declare(stack_global, L"__assign__", DUCK_FUNCTION_SEMI_MACRO, (duck_native_t)&duck_i_assign, object_type, 2, a_argv, a_argn);
    
    duck_type_t *d_argv[]={0,type_type,object_type};
    static wchar_t *d_argn[]={L"name", L"type", L"value"};    
    duck_native_declare(stack_global, L"__declare__", DUCK_FUNCTION_SEMI_MACRO, (duck_native_t)&duck_i_declare, object_type, 3, d_argv, d_argn);
    
    duck_type_t *f_argv[]={0,0,0,0};
    static wchar_t *f_argn[]={L"name", L"", L"", L"body"};    
    duck_native_declare(stack_global, L"__function__", DUCK_FUNCTION_SEMI_MACRO, (duck_native_t)&duck_i_function, type_type, 4, f_argv, f_argn);

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
    duck_native_declare(stack_global, L"__if__", &duck_i_if, object_type, 0, 0);
    */
    /*
      FIXME: Init duck_node_null to something nice and sane.
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
				    duck_stack_frame_t *stack) 
{
    //wprintf(L"duck_function_invoke %ls %d; %d params\n", function->name, function, function->input_count);    
    
    switch(function->flags) 
    {
	case DUCK_FUNCTION_SEMI_MACRO:
	{
	    return function->native.semi_macro(param, stack);
	}
	case DUCK_FUNCTION_MACRO:
	{
	    return duck_node_invoke(function->native.macro(param, stack), stack);
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
		duck_stack_frame_t *my_stack = duck_stack_create(64, stack_global);
		
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
		    duck_object_t *value=duck_node_invoke(param->child[0], stack);
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
    
    wprintf(L"Initializing...\n");    
    duck_i_init();
    wprintf(L"Parsing...\n");    
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
    
    duck_object_t *program_object = duck_node_invoke(program, stack_global);
    
    //duck_object_print(program_object, 3);
    
    duck_function_t *func=duck_function_unwrap(program_object);    
    duck_function_invoke(func, 0, stack_global);
    
    wprintf(L"\n");
}
