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

#define likely(x) (x)
/*
  Ideas for apps written in anna:
  Documentation generator: Introspect all data types and generate html documentation on them.
  live: Debug a running application using command line or web browser.
  Compiler front end.
  Asynchronous continuation based app/web server
*/
/*
  Templating plan:
  
  anna_macro_type must keep a copy of the original ast for every type created (done)
  All native types must have an ast of their original definition (done)
  AST nodes for creating native types need to use name lookups when refering to types  (done)
  Use plain AST nodes instead of special, magical nodes for representing native types in AST (done)
  Implement a function that searches and replaces identifier nodes (done)
  __templateAttribute__ will do search and replaceg (done)
  __templatize__ will clone the AST, modify the arguments to __tempalteAttribute__, and let __templateAttribute__ do the replacing. (done)
  __templatize__ needs to have a cache ot already templatized types (done)
  Move node prepare calls to their own pass
  Update attribute call syntax to make it easy to use the same attribute for both types and functions

  Code layout plan:

  - Move object and type code to individual .[ch] files.
  - Split up anna_macro.c into multiple files, one of operators, one for 
  function/block/type handling, one for functional construct support, 
  etc..
  - All node types should have a head var which is an anna_node_t, to reduce the amount of casting needed.
  
  ComparisonMap type
  HashMap type
  Range type
  Pair type
  Node type
  Better Function type
  Better Type type  
  Stack type
  Byte type
  Buffer type
  Complex type

  Make abides check properly check method signatures
  Make abides check handle dependency cycles
  Cache abides checks. Do all checks at type creation time and stow away the results somewhere?
  Move the rest of the native types to use the AST node creation style from List
  Object constructor needs to set all members to null
  Identifier invokation should use sid instead of name lookup
  Split type namespace from type object
  Properties
  Code validator
  Type checking on function types
  General purpose currying
  Namespaces
  Subfunction/block tracking in function
  Functions that don't return an Int (depends on block tracking)
  Make comments nest
  
  Implement basic string methods
  Implement string comparison methods
  List arithmetic
  
  Function default argument values
  Named function arguments
  Variadic functions
  Garbage collection  
  Proper intersection/union of types
  static member identifier and assignment
  static function calls

  cast function (depends on type namespace/type object splittingx)
  import macro
  __macro__ macro
  elif macro
  __extendsAttribute__ macro
  is function
  as function
  __returnAssign__ macro
  __list__ macro (depends on variadic functions and templates)
  use macro
  __memberCall__ macro
  __staticMemberGet__ macro
  __staticMemberSet__ macro
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
  class member macros
  Type support for lists
  Move uncommon operators to become generic operators (bit ops, sign, abs, etc.)
  Removed macro methods, all macros are global
  
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
  Parse identifiers
  Parse calls
  Parse string literals
  Parse chars literals
  Parse float literals
  
  Implement basic Int methods
  Implement basic Float methods
  Implement Int comparison methods
  Implement List getter and setter
  Implement basic Char methods
  Implement Char comparison methods
  Implement basic List methods
  
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
  __templateAttribute__ macro
  __templatize__ macro
  each, map, filter, first

*/
/*
  Cool macro feature. Create a complex AST by wrapping the equivalent
  code in a call to the AST function.

  var n = AST(1+x);  
*/

anna_type_t *type_type=0, *object_type=0, *int_type=0, *string_type=0, *char_type=0, *null_type=0,  *string_type, *char_type, *list_type, *float_type;
anna_object_t *null_object=0;

static hash_table_t anna_type_for_function_identifier;
static hash_table_t anna_mid_identifier;
static array_list_t anna_mid_identifier_reverse;

anna_node_t *anna_node_null=0;

static anna_stack_frame_t *stack_global;
static size_t mid_pos = ANNA_MID_FIRST_UNRESERVED;

int anna_error_count=0;

static anna_member_t **anna_mid_identifier_create();

static anna_type_t *anna_type_create_raw(wchar_t *name, size_t static_member_count, int fake_location);
static void anna_type_wrapper_create(anna_type_t *result);

anna_object_t *anna_i_function_wrapper_call(anna_node_call_t *node, anna_stack_frame_t *stack);
void anna_object_print(anna_object_t *obj, int level);
static void anna_sniff_return_type(anna_function_t *f);
void anna_function_setup_type(anna_function_t *f);


void anna_native_declare(anna_stack_frame_t *stack,
			 wchar_t *name,
			 int flags,
			 anna_native_t func,
			 anna_type_t *result,
			 size_t argc, 
			 anna_type_t **argv,
			 wchar_t **argn)
{
    anna_function_t *f = anna_native_create(name, flags, func, result, argc, argv, argn);
    anna_stack_declare(stack, name, f->type, f->wrapper);
}


anna_object_t **anna_member_addr_get_str(anna_object_t *obj, wchar_t *name)
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
    
    anna_function_type_key_t *key1 = (anna_function_type_key_t *)a;
    anna_function_type_key_t *key2 = (anna_function_type_key_t *)b;

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

anna_type_t *anna_type_for_function(anna_type_t *result, size_t argc, anna_type_t **argv)
{
    //  static int count=0;
    //if((count++)==10) {CRASH};
  
    int i;
    static anna_function_type_key_t *key = 0;
    static size_t key_sz = 0;
    size_t new_key_sz = sizeof(anna_function_type_key_t) + sizeof(anna_type_t *)*argc;
    
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
    
    anna_type_t *res = hash_get(&anna_type_for_function_identifier, key);
    if(!res)
    {
	anna_function_type_key_t *new_key = malloc(new_key_sz);
	memcpy(new_key, key, new_key_sz);	
	res = anna_type_create_raw(L"!FunctionType", 64, 1);	
	hash_put(&anna_type_for_function_identifier, new_key, res);
	anna_member_create(res, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD, L"!functionTypePayload",
			   1, null_type);
	anna_member_create(res, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD, L"!functionPayload", 
			   0, null_type);
	anna_member_create(res, ANNA_MID_FUNCTION_WRAPPER_STACK, L"!functionStack", 
			   0, null_type);
	anna_type_wrapper_create(res);
	/*
	  FIXME: Add children to the definition tree!
	*/


	(*anna_static_member_addr_get_mid(res, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD)) = (anna_object_t *)new_key;
    }
    else 
    {
	//wprintf(L"YAY\n");
    }
    
    return res;
}


anna_function_type_key_t *anna_function_unwrap_type(anna_type_t *type)
{
    if(!type)
    {
	wprintf(L"Critical: Tried to get function from non-existing type\n");
	CRASH;
	
    }
    
    //wprintf(L"Find function signature for call %ls\n", type->name);
    
    anna_function_type_key_t **function_ptr = (anna_function_type_key_t **)anna_static_member_addr_get_mid(type, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    if(function_ptr) 
    {
	//wprintf(L"Got member, has return type %ls\n", (*function_ptr)->result->name);
	return *function_ptr;
    }
    else 
    {
	//wprintf(L"Not a direct function, check for __call__ member\n");
	anna_object_t **function_wrapper_ptr = anna_static_member_addr_get_mid(type, ANNA_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    //wprintf(L"Found, we're unwrapping it now\n");
	    return anna_function_unwrap_type((*function_wrapper_ptr)->type);	    
	}
	return 0;	
    }
//     FIXME: Is there any validity checking we could do here?
  
}

anna_function_t *anna_function_unwrap(anna_object_t *obj)
{
    assert(obj);    
    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(obj, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    if(function_ptr) 
    {
	//wprintf(L"Got object of type %ls with native method payload\n", obj->type->name);
	return *function_ptr;
    }
    else 
    {
	anna_object_t **function_wrapper_ptr = anna_static_member_addr_get_mid(obj->type, ANNA_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    //wprintf(L"Got object with __call__ member\n");
	    return anna_function_unwrap(*function_wrapper_ptr);	    
	}
	return 0;	
    }
//     FIXME: Is there any validity checking we could do here?
  
}

anna_object_t *anna_function_wrapped_invoke(anna_object_t *obj, 
					    anna_object_t *this,
					    anna_node_call_t *param,
					    anna_stack_frame_t *local)
{
    //wprintf(L"Wrapped invoke of function %ls\n", obj->type->name);
  
    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(obj, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(obj, ANNA_MID_FUNCTION_WRAPPER_STACK);
    if(function_ptr) 
    {
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
	anna_object_print(obj, 0);
	
	CRASH;
    }
    return 0;
}

anna_type_t *anna_type_member_type_get(anna_type_t *type, wchar_t *name)
{
    assert(type);
    assert(name);
    anna_member_t *m = (anna_member_t *)hash_get(&type->name_identifier, name);
    if(!m)
    {
	return 0;
    }
    
    return m->type;
}



/*
  anna_object_t *anna_call(anna_stack_frame_t *stack, anna_object_t *obj)
  {
  anna_object *call = *anna_get_member_addr_int(obj, ANNA_MID_CALL);
  anna_node *node = (anna_node *)*anna_get_member_addr_int(obj, ANNA_MID_NODE);
  return node->invoke(node, stack);
  }
*/

anna_object_t *anna_construct(anna_type_t *type, struct anna_node_call *param, anna_stack_frame_t *stack)
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
    size_t func_size = sizeof(anna_function_t) + function_original->input_count*sizeof(anna_type_t *);
    anna_function_t *function_copy = malloc(func_size);
    memcpy(function_copy, function_original, func_size);
    function_copy->this = owner;
    function_copy->wrapper = anna_object_create(function_copy->type);
    memcpy(anna_member_addr_get_mid(function_copy->wrapper,ANNA_MID_FUNCTION_WRAPPER_PAYLOAD), 
	   &function_copy, sizeof(anna_function_t *));
    memcpy(anna_member_addr_get_mid(function_copy->wrapper,ANNA_MID_FUNCTION_WRAPPER_STACK),
	   anna_member_addr_get_mid(function_original->wrapper,ANNA_MID_FUNCTION_WRAPPER_STACK),
	   sizeof(anna_stack_frame_t *));
    return function_copy->wrapper;
}

void anna_prepare_children(anna_node_call_t *in, anna_function_t *func, anna_node_list_t *parent)
{
    int i;
    for(i=0; i< in->child_count; i++)
	in->child[i] = anna_node_prepare(in->child[i], func, parent);
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

void anna_function_prepare(anna_function_t *function)
{
    int i;
    anna_node_list_t list = 
	{
	    (anna_node_t *)function->body, 0, 0
	}
    ;

    for(i=0; i<function->body->child_count; i++) 
    {
	list.idx=i;
	function->body->child[i] = anna_node_prepare(function->body->child[i], function, &list);
    }
    for(i=0; i<function->body->child_count; i++) 
    {
	anna_node_validate(function->body->child[i], function->stack_template);
    }
/*   for(i=0; i<al_get_count(&function->child_function); i++) 
     {
     al_set(&function->child_function, i,
     anna_node_prepare(al_get(&function->child_function, i), function, &list));
     }
*/ 
    /*
      wprintf(L"Function after preparations:\n");
      anna_node_print(function->body);
      wprintf(L"\n");
    */
}


anna_function_t *anna_function_create(wchar_t *name,
				      int flags,
				      anna_node_call_t *body, 
				      anna_type_t *return_type,
				      size_t argc,
				      anna_type_t **argv,
				      wchar_t **argn,
				      anna_stack_frame_t *parent_stack,
				      int return_pop_count)
{
    if(!flags) {
	//assert(return_type);
	if(argc) {
	    assert(argv);
	    assert(argn);
	}
    }
    
    int i;
    
    for(i=0;i<argc; i++)
    {
	assert(argv[i]);
	assert(argn[i]);
    }
    
    
    anna_function_t *result = calloc(1,sizeof(anna_function_t) + argc*sizeof(anna_type_t *));
    result->native.function=0;
    result->flags=flags;
    result->name = name;
    result->body = body;
    result->return_type=return_type;
    result->input_count=argc;
    result->return_pop_count = return_pop_count;
    
    memcpy(&result->input_type, argv, sizeof(anna_type_t *)*argc);
    result->input_name = malloc(argc*sizeof(wchar_t *));;
    memcpy(result->input_name, argn, sizeof(wchar_t *)*argc);

    result->stack_template = anna_stack_create(64, parent_stack);
    for(i=0; i<argc;i++)
    {
	anna_stack_declare(result->stack_template, argn[i], argv[i], null_object);	
    }    

    anna_function_prepare(result);

    if(!return_type)
	anna_sniff_return_type(result);
    
    if(!flags) {
	assert(result->return_type);
    }
    
    anna_function_setup_type(result);
    
    memcpy(anna_member_addr_get_mid(result->wrapper,ANNA_MID_FUNCTION_WRAPPER_PAYLOAD), &result, sizeof(anna_function_t *));
    memcpy(anna_member_addr_get_mid(result->wrapper,ANNA_MID_FUNCTION_WRAPPER_STACK), &stack_global, sizeof(anna_stack_frame_t *));
    //wprintf(L"Function object is %d, wrapper is %d\n", result, result->wrapper);

    return result;
}

void anna_function_setup_type(anna_function_t *f)
{
    anna_type_t *function_type = anna_type_for_function(f->return_type, f->input_count, f->input_type);
    f->wrapper = anna_object_create(function_type);
    f->type = function_type;    
}

static void sniff(array_list_t *lst, anna_function_t *f, int level)
{
    if(f->return_pop_count == level)
    {
	int i;
	for(i=0;i<f->body->child_count;i++)
	{
	    if(f->body->child[i]->node_type == ANNA_NODE_RETURN)
	    {
		anna_node_return_t *r = (anna_node_return_t *)f->body->child[i];
		al_push(lst, anna_node_get_return_type(r->payload, f->stack_template));
	    }	    
	}
	for(i=0;i<al_get_count(&f->child_function);i++)
	{
	    sniff(lst, al_get(&f->child_function, i), level+1);
	}
    }
}


static void anna_sniff_return_type(anna_function_t *f)
{
    array_list_t types;
    al_init(&types);
    sniff(&types, f, 0);
    int i;
    
    if(al_get_count(&types) >0)
    {
	//wprintf(L"Got the following %d return types for function %ls, create intersection:\n", al_get_count(&types), f->name);
	anna_type_t *res = al_get(&types, 0);
	for(i=1;i<al_get_count(&types); i++)
	{
	    anna_type_t *t = al_get(&types, i);
	    res = anna_type_intersect(res,t);
	}
	f->return_type = res;
	anna_node_call_add_child(f->body, anna_node_null_create(&f->body->location));
    }
    else
    {
	if(f->body->child_count)
	    f->return_type = anna_node_get_return_type(f->body->child[f->body->child_count-1], f->stack_template);
	else
	    f->return_type = null_type;
	//wprintf(L"Implicit return type is %ls\n", f->return_type->name);
    }

}




anna_function_t *anna_native_create(wchar_t *name,
				    int flags,
				    anna_native_t native, 
				    anna_type_t *return_type,
				    size_t argc,
				    anna_type_t **argv,
				    wchar_t **argn)				
{
    if(!flags) {
	assert(return_type);
	if(argc) {
	    assert(argv);
	    assert(argn);
	}
    }
  
    anna_function_t *result = calloc(1, sizeof(anna_function_t) + argc*sizeof(anna_type_t *));
    result->flags=flags;
    result->native = native;
    result->name = name;
    result->return_type=return_type;
    result->input_count=argc;
    memcpy(&result->input_type, argv, sizeof(anna_type_t *)*argc);
    result->input_name = argn;
    anna_type_t *function_type = anna_type_for_function(return_type, argc, argv);
    result->type = function_type;
    result->wrapper = anna_object_create(function_type);
    
    anna_object_t **member_ptr = anna_member_addr_get_mid(result->wrapper,ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    if(!member_ptr) {
	wprintf(L"Error: function_wrapper_type for function %ls does not have a payload!!!\n",
		name);
	//anna_object_print(result->wrapper, 3);
	CRASH;	
    }
    
    memcpy(anna_member_addr_get_mid(result->wrapper,ANNA_MID_FUNCTION_WRAPPER_PAYLOAD), &result, sizeof(anna_function_t *));
    //wprintf(L"Creating function %ls @ %d with macro flag %d\n", result->name, result, result->flags);

    return result;
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
    wchar_t **members = calloc(sizeof(wchar_t *), role_model->member_count+role_model->static_member_count);
    anna_type_get_member_names(role_model, members);    
    assert(hash_get_count(&role_model->name_identifier) == role_model->member_count+role_model->static_member_count);
    
    for(i=0; i<role_model->member_count+role_model->static_member_count; i++)
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

anna_type_t *anna_type_unwrap(anna_object_t *wrapper)
{
    anna_type_t *result;
    
    memcpy(&result, anna_member_addr_get_mid(wrapper, ANNA_MID_TYPE_WRAPPER_PAYLOAD), sizeof(anna_type_t *));
    return result;
}

static void anna_type_wrapper_create(anna_type_t *result)
{
    result->wrapper = anna_object_create(type_type);
    memcpy(anna_member_addr_get_mid(result->wrapper, ANNA_MID_TYPE_WRAPPER_PAYLOAD), &result, sizeof(anna_type_t *));  
}

static anna_type_t *anna_type_create_raw(wchar_t *name, size_t static_member_count, int fake_definition)
{
    anna_type_t *result = calloc(1,sizeof(anna_type_t)+sizeof(anna_object_t *)*static_member_count);
    result->static_member_count = 0;
    result->member_count = 0;
    hash_init(&result->name_identifier, &hash_wcs_func, &hash_wcs_cmp);
    result->mid_identifier = anna_mid_identifier_create();
    result->name = name;
    
    return result;  
}

anna_type_t *anna_type_create(wchar_t *name, size_t static_member_count, int fake_definition)
{
    anna_type_t *result = anna_type_create_raw(name, static_member_count, fake_definition);
    anna_type_wrapper_create(result);
    return result;
}
			  
anna_object_t *anna_object_create(anna_type_t *type) {
    anna_object_t *result = calloc(1,sizeof(anna_object_t)+sizeof(anna_object_t *)*type->member_count);
    result->type = type;
  
    return result;
}

size_t anna_member_create(anna_type_t *type,
			  ssize_t mid,
			  wchar_t *name,
			  int is_static,
			  anna_type_t *member_type)
{
    assert(member_type);
    if(hash_get(&type->name_identifier, name))
    {
	wprintf(L"Redeclaring member %ls of type %ls\n",
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
	member->offset = type->static_member_count++;
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


void anna_native_method_add_node(anna_node_call_t *definition,
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


static anna_member_t **anna_mid_identifier_create()
{
    /*
      FIXME: Track, reallocate when we run out of space, etc.
    */
    return calloc(1,4096);
}

size_t anna_native_method_create(anna_type_t *type,
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
    
    mid = anna_member_create(type, mid, name, 1, anna_type_for_function(result, argc, argv));
    anna_member_t *m = type->mid_identifier[mid];
    //wprintf(L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    type->static_member[m->offset] = anna_native_create(name, flags, func, result, argc, argv, argn)->wrapper;
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
    //wprintf(L"Create method named %ls with offset %d on type %d\n", m->name, m->offset, type);
    type->static_member[m->offset] = definition->wrapper;
    return (size_t)mid;
}



static void anna_type_type_create_early()
{
    anna_member_create(type_type, ANNA_MID_TYPE_WRAPPER_PAYLOAD, L"!typeWrapperPayload",
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
  
    null_type->static_member[0] = anna_native_create(L"!nullFunction", ANNA_FUNCTION_FUNCTION, 
						     (anna_native_t)&anna_i_null_function, 
						     null_type, 1, argv, argn)->wrapper;
  
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

    stack_global = anna_stack_create(4096, 0);
    
    /*
      Create lowest level stuff. Bits of magic, be careful with
      ordering here. A lot of intricate dependencies going on between
      the various calls...
    */
    
    type_type = anna_type_create_raw(L"Type", 64, 1);
    object_type = anna_type_create_raw(L"Object", 64, 1);
    null_type = anna_type_create_raw(L"Null", 1, 1);
    
    anna_type_type_create_early();
    anna_null_type_create_early();
    anna_object_type_create_early();

    anna_type_wrapper_create(null_type);
    anna_type_wrapper_create(object_type);
    anna_type_wrapper_create(type_type);

    anna_stack_declare(stack_global, L"Object", object_type, object_type->wrapper);
    anna_stack_declare(stack_global, L"Null", null_type, null_type->wrapper);
    
    anna_macro_init(stack_global);

    anna_int_type_create(stack_global);
    anna_list_type_create(stack_global);
    anna_char_type_create(stack_global);
    anna_string_type_create(stack_global);
    anna_float_type_create(stack_global);

    anna_function_implementation_init(stack_global);

    assert(anna_abides(int_type,object_type)==1);
    assert(anna_abides(list_type,object_type)==1);
    assert(anna_abides(object_type,int_type)==0);
    assert(anna_abides(int_type,int_type)==1);
    
    null_object = anna_object_create(null_type);
    
}

void anna_print_member(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    wchar_t *key = (wchar_t *)key_ptr;
    anna_member_t *member = (anna_member_t *)val_ptr;
    //anna_object_t *obj = (anna_object_t *)aux_ptr;
    //anna_object_t *value = member->is_static?obj->type->static_member[member->offset]:obj->member[member->offset];    
    wprintf(L"  %ls: %ls\n", key, member->type?member->type->name:L"?");
}

void anna_object_print(anna_object_t *obj, int level)
{
    wprintf(L"%ls:\n", obj->type->name);
    hash_foreach2(&obj->type->name_identifier, &anna_print_member, obj);
}

anna_object_t *anna_function_invoke_values(anna_function_t *function, 
					   anna_object_t *this,
					   anna_object_t **param,
					   anna_stack_frame_t *outer)
{
    switch(function->flags) 
    {
	default:
	{
	    wprintf(L"FATAL: Macro %ls at invoke!!!!\n", function->name);
	    CRASH;
	}
	case ANNA_FUNCTION_FUNCTION:
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
		  FIXME:
		  Support return statement
		*/
		//wprintf(L"Run non-native function %ls with %d params\n", function->name, function->input_count);
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
		    anna_stack_declare(my_stack, 
				       function->input_name[i+offset],
				       function->input_type[i+offset],
				       param[i]);
		}
		
		for(i=0; i<function->body->child_count && !my_stack->stop; i++)
		{
		    result = anna_node_invoke(function->body->child[i], my_stack);
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


anna_object_t *anna_function_invoke(anna_function_t *function, 
				    anna_object_t *this,
				    anna_node_call_t *param, 
				    anna_stack_frame_t *stack,
				    anna_stack_frame_t *outer) 
{
    if(!this)
    {
	this=function->this;
    }
    
    //wprintf(L"anna_function_invoke %ls %d; %d params\n", function->name, function, function->input_count);    
    if(likely(function->input_count < 8))
    {
	anna_object_t *argv[8];
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
	    
	    argv[i+offset]=anna_node_invoke(param->child[i], stack);
	}      
	return anna_function_invoke_values(function, 0, argv, outer);
    }
    else
    {
	anna_object_t **argv=malloc(sizeof(anna_object_t *)*function->input_count);
	int i;
	
	int offset=0;
	if(this)
	{
	    offset=1;
	    argv[0]=this;		    
	}
	for(i=0; i<(function->input_count-offset); i++) 
	{
	    argv[i+offset]=anna_node_invoke(param->child[i], stack);
	}
	anna_object_t *result = anna_function_invoke_values(function, 0, argv, outer);
	free(argv);
	return result;
      
    }
  
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
	wprintf(L"Error: Expected exactly one argument, a name of a file to run.\n");
	exit(1);
    }
    wchar_t *filename = str2wcs(argv[1]);

    wprintf(L"Initializing interpreter...\n");    
    anna_init();

    if(anna_error_count)
    {
	wprintf(L"Found %d error(s) during initialization, exiting\n", anna_error_count);
	exit(1);
    }
    wprintf(L"Parsing file %ls...\n", filename);    
    anna_node_t *program = anna_parse(filename);
    
    if(!program) 
    {
	wprintf(L"Program failed to parse correctly; exiting.\n");
	exit(1);
    }
    
    wprintf(L"Parsed program:\n");    
    anna_node_print(program);
    wprintf(L"\n");
    
    wprintf(L"Validating program...\n");    
    
    /*
      The entire program is a __block__ call, which we use to create an anonymous function definition
    */
    anna_node_dummy_t *program_callable = 
	anna_node_dummy_create(&program->location,
			       anna_function_create(L"!program",
						    0,
						    node_cast_call(program),
						    null_type, 0, 0, 0, stack_global, 0)->wrapper,
			       0);
    ANNA_PREPARED(program_callable);
    /*
      Invoke the anonymous function, the return is a function_type_t->wrapper
    */
    anna_object_t *program_object = anna_node_invoke((anna_node_t *)program_callable, stack_global);
    if(anna_error_count)
    {
	wprintf(L"Found %d error(s) during program validation, exiting\n", anna_error_count);
	exit(1);
    }
    /*
      Run the function
    */
    wprintf(L"Validated program:\n");    
    anna_node_print(program);
    wprintf(L"\n");
    wprintf(L"Output:\n");    

    anna_function_t *func=anna_function_unwrap(program_object);    
    assert(func);
    anna_function_invoke(func, 0, 0, stack_global, stack_global);
    
    wprintf(L"\n");
}
