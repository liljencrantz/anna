#ifndef ANNA_H
#define ANNA_H

#include "util.h"
#include "anna_checks.h"
#include "anna_crash.h"

#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
#define ANNA_PREPARED(n) ((n)->prepared=1)
#define ANNA_UNPREPARED(n) ((n)->prepared=0)
#define ANNA_CHECK_NODE_PREPARED(n)  if(!(n)->prepared)			\
    {									\
	anna_error(n,L"Critical: Tried to invoke unprepared AST node");	\
	anna_node_print(n);						\
	wprintf(L"\n");							\
	CRASH;								\
    }
#else
#define ANNA_PREPARED(n) 
#define ANNA_UNPREPARED(n) 
#define ANNA_CHECK_NODE_PREPARED(n) 
#endif

#define likely(x) (x)
#define unlikely(x) (x)

struct anna_type;
struct anna_object;
struct anna_member;
struct anna_node_call;
struct anna_stack_frame;
struct anna_node_call;
struct anna_function;
struct anna_node_list;
struct anna_node_call;

typedef struct anna_object *(*anna_native_function_t)( struct anna_object ** );
typedef struct anna_node *(*anna_native_macro_t)( struct anna_node_call *);

#define ANNA_FUNCTION_REGISTERED 1
#define ANNA_FUNCTION_PREPARED_COMMON 2
#define ANNA_FUNCTION_PREPARED_INTERFACE 4
#define ANNA_FUNCTION_PREPARED_IMPLEMENTATION 8
#define ANNA_FUNCTION_VARIADIC 16
#define ANNA_FUNCTION_MODULE 32
#define ANNA_FUNCTION_MACRO 64
/**
   This function is anonymously declared, and should not be registered
   in any scope.
 */
#define ANNA_FUNCTION_ANONYMOUS 128
/**
   This function is a closure, and needs to have a pointer
   set up to the function invocation that encloses it.
*/
#define ANNA_FUNCTION_CLOSURE 256

#define ANNA_TYPE_REGISTERED 1
#define ANNA_TYPE_PREPARED_INTERFACE 2
#define ANNA_TYPE_PREPARED_IMPLEMENTATION 4

/*
#define ANNA_FUNCTION_CLOSURE 2
#define ANNA_FUNCTION_STANDALONE 4
*/

/*
  The preallocated MIDs
*/
#define ANNA_MID_CALL_PAYLOAD 0
#define ANNA_MID_INT_PAYLOAD 1
#define ANNA_MID_STRING_PAYLOAD 2
#define ANNA_MID_STRING_PAYLOAD_SIZE 3
#define ANNA_MID_CHAR_PAYLOAD 4
#define ANNA_MID_LIST_PAYLOAD 5
#define ANNA_MID_LIST_SIZE 6
#define ANNA_MID_LIST_CAPACITY 7
#define ANNA_MID_FLOAT_PAYLOAD 8
#define ANNA_MID_FUNCTION_WRAPPER_PAYLOAD 9
#define ANNA_MID_FUNCTION_WRAPPER_STACK 10
#define ANNA_MID_FUNCTION_WRAPPER_THIS 11
#define ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD 12
#define ANNA_MID_TYPE_WRAPPER_PAYLOAD 13
#define ANNA_MID_CALL 14
#define ANNA_MID_INIT_PAYLOAD 15
#define ANNA_MID_NODE_PAYLOAD 16
#define ANNA_MID_MEMBER_PAYLOAD 17
#define ANNA_MID_MEMBER_TYPE_PAYLOAD 18
#define ANNA_MID_STACK_PAYLOAD 19
#define ANNA_MID_STACK_TYPE_PAYLOAD 20
#define ANNA_MID_FROM 21
#define ANNA_MID_TO 22
#define ANNA_MID_STEP 23
#define ANNA_MID_FIRST_UNRESERVED 24

union anna_native
{
    anna_native_function_t function;
    anna_native_macro_t macro;
}
  ;
/**
   The struct representing an object type. 
 */
struct anna_type
{
    /**
       The number of non-static members in an object of this type.
     */
    size_t member_count;
    /**
       The number of properties in this type.
     */
    size_t property_count;
    /**
       The number of static member variables in an object of this
       type.
     */
    size_t static_member_count;
    /**
       The number of static members variables that can be allocated in
       the type without reallocating the static member array first.
     */
    size_t static_member_capacity;
    /**
       A mapping from a variable name to a anna_member struct
       containing information on a specific member.
     */
    hash_table_t name_identifier;
    /**
       A name for this type. A type does not inherently have one
       specific name, it can be renamed and copied just like any other
       variable. This name is here in order to provide better
       introspection ability for humans.
     */
    wchar_t *name;
    /**
       A bitfield containing various additional info on this type.
     */
    int flags;
    /**
       A stack frame. 

       FIXME: What is it for? I don't currently know. Namespacing,
       maybe?
     */
    struct anna_stack_frame *stack;
    /**
       An array containing all member structs. The offset is a mid.
     */
    struct anna_member **mid_identifier;
    /**
       The AST that defines this type.
     */
    struct anna_node_call *definition;
    /**
       The object that wraps this type. Used for
       reflection/introspection purposes.
     */
    struct anna_object *wrapper;
    /**
       An array containing all static members.
     */
    struct anna_object **static_member;
    /**
       An array list of all child functions of this function.
     */
    array_list_t child_function;
    /**
       Fixme: What is this?
     */
    array_list_t child_type;
};

struct anna_member
{
    /**
      The type object that describes the interface that any value
      stored in this member must implement.
     */
    struct anna_type *type;
    /**
       The offset into either the object->member ot the
       type->static_member array that this member is located at.
     */
    size_t offset;
    /**
       If true, this member is static, i.e. shared between all objects
       of this type. If so, it is stored in the type object's
       static_member array instead of in the objects member array.
     */
    int is_static;
    /**
       If true, this member is a property, i.e. a getter and/or setter
       method that act as a regular variable.
     */
    int is_property;
    /**
       If true, this member is a method. Methods are functions that,
       when accessed, are implicitly curried so that the this variable
       is bound to the owning object.
     */
    int is_method;
    /**
       Only used if the member is a property. Gives the offset of the
       setter method. Note that all non-static variables are
       implicitly properties.
     */
    size_t setter_offset;
    /**
       Only used if the member is a property. Gives the offset of the
       getter method. Note that all non-static variables are
       implicitly properties.
    */
    size_t getter_offset;    
    /**
       Points to the anna_object that can be used to introspect this
       member from within the anna code. 
     */
    struct anna_object *wrapper;
    /**
       The name of this member.
     */
    wchar_t name[];
};

/**
   The struct that represents any Anna object
 */
struct anna_object
{
    /**
       The type of this object
     */
    struct anna_type *type;
    /**
       The array of members. To decode what member lives at what
       offset, use the information stored in the type object.
     */
    struct anna_object *member[];
};

struct anna_function
{
    /**
       A name for this function. A function does not inherently have
       one specific name, it can be renamed and copied just like any
       other variable. This name is here in order to provide better
       introspection ability for humans.
     */
    wchar_t *name;
    /**
       The body of this function.
     */
    struct anna_node_call *body;  
    /**
       If this function is in fact a method belonging to a class, this
       is a pointer to the type in question. Otherwise, it's a null
       pointer.
    */
    struct anna_type *member_of;
    /**
       The mid this method has in the type it is a member of
     */
    size_t mid;
    /**
       The full AST that originally defined this function. 
    */
    struct anna_node_call *definition;
    /**
       If this is a native function, this union will contain the
       function pointer used for invocation. Otherwise, this will be a
       null function pointer.
     */
    union anna_native native;
    /**
       The return type of this function.
     */
    struct anna_type *return_type;
    /**
       Points to the anna_object that can be used to introspect this
       member from within the anna code.
    */
    struct anna_object *wrapper;
    /**
       The number of input arguments to this function.
     */
    size_t input_count;
    /**
       The name of each input argument
     */
    wchar_t **input_name;
    /**
       A bitfield containing various additional info on this type.
     */
    int flags;
    /**
       If this function is a bound method, this field contains a curried this pointer.
     */
    struct anna_object *this;
    /**
       The template stack frame of this function
    */
    struct anna_stack_frame *stack_template;
//    struct anna_stack_frame *closure_parent_stack_template;
    array_list_t child_function;
    array_list_t child_type;
    struct anna_type **input_type;    
};

#define ANNA_IS_MACRO(f) (!!((f)->flags & ANNA_FUNCTION_MACRO))
#define ANNA_IS_VARIADIC(f) (!!((f)->flags & ANNA_FUNCTION_VARIADIC))

struct anna_node_list
{
    struct anna_node *node;
    size_t idx;
    struct anna_node_list *parent;
};


typedef struct anna_type anna_type_t;
typedef struct anna_member anna_member_t;
typedef struct anna_object anna_object_t;
typedef struct anna_frame anna_frame_t;
typedef struct anna_function anna_function_t;
typedef union anna_native anna_native_t;
typedef struct anna_node_list anna_node_list_t;

typedef struct 
{
    anna_type_t *result; 
    size_t argc;
    int is_variadic;
    wchar_t **argn;
    anna_type_t *argv[];
} anna_function_type_key_t;

extern anna_type_t *type_type, *object_type, *int_type, *string_type, *char_type, *null_type,  *string_type, *char_type, *list_type, *float_type, *member_type;
extern anna_object_t *null_object;
extern int anna_error_count;
extern struct anna_stack_frame *stack_global;

/**
   Declare all global, native functions
 */
void anna_function_implementation_init(
    struct anna_stack_frame *stack);

/**
  Return the anna_type_t contained in the specified anna_type_t.wrapper
 */
anna_type_t *anna_type_unwrap(
    anna_object_t *wrapper);

anna_type_t *anna_type_for_function(
    anna_type_t *result, size_t argc,
    anna_type_t **argv, wchar_t **argn,
    int is_variadic);

anna_object_t *anna_function_wrapped_invoke(
    anna_object_t *function,
    anna_object_t *this,
    size_t param_count,
    struct anna_node **param, 
    struct anna_stack_frame *local);

anna_object_t *anna_function_invoke_values(
    anna_function_t *function, 
    anna_object_t *this,
    anna_object_t **param,
    struct anna_stack_frame *outer);

/**
   \param function the function to invoke
   \param this the value of the this parameter
   \param param the ast call that invoked the function, which containz the argument list for the function
   \param stack the stack
   \param Don't know?
 */
anna_object_t *anna_function_invoke(
    anna_function_t *function, 
    anna_object_t *this,
    size_t param_count,
    struct anna_node **param,
    struct anna_stack_frame *param_invoke_stack,
    struct anna_stack_frame *function_parent_stack);





/**
   \param macro the macro to invoke
   \param node the ast node to transform
   \param function the context of this ast node
 */
struct anna_node *anna_macro_invoke(
    anna_function_t *macro,
    struct anna_node_call *node);

void anna_function_type_key_print(
    anna_function_type_key_t *k);


anna_object_t *anna_construct(
    anna_type_t *type, struct anna_node_call *param,
    struct anna_stack_frame *stack);

anna_object_t **anna_static_member_addr_get_mid(
    anna_type_t *type, size_t mid);

//anna_object_t **anna_static_member_addr_get_str(anna_type_t *type, wchar_t *name);

anna_object_t **anna_member_addr_get_str(
    anna_object_t *obj, wchar_t *name);

anna_object_t **anna_member_addr_get_mid(
    anna_object_t *obj, size_t mid);

anna_object_t *anna_method_wrap(
    anna_object_t *method, 
    anna_object_t *owner);

void anna_member_redeclare(
    anna_type_t *type,
    ssize_t mid,
    anna_type_t *member_type);

void anna_member_add_node(
    struct anna_node_call *type,
    ssize_t mid,
    wchar_t *name,
    int is_static,
    struct anna_node *member_type);

size_t anna_native_method_create(
    anna_type_t *type,
    ssize_t mid,
    wchar_t *name,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn);

void anna_native_method_add_node(
    struct anna_node_call *type,
    ssize_t mid,
    wchar_t *name,
    int flags,
    anna_native_t func,
    struct anna_node *result,
    size_t argc,
    struct anna_node **argv,
    wchar_t **argn);

size_t anna_method_create(
    anna_type_t *type,
    ssize_t mid,
    wchar_t *name,
    int flags,
    anna_function_t *definition);


int anna_abides(
    anna_type_t *contender, anna_type_t *role_model);

int anna_abides_fault_count(
    anna_type_t *contender, anna_type_t *role_model);

anna_type_t *anna_type_intersect(
    anna_type_t *t1, anna_type_t *t2);

void anna_error(
    struct anna_node *node, wchar_t *msg, ...);

int anna_type_setup(
    anna_type_t *type, 
    anna_function_t *function, 
    anna_node_list_t *parent);

anna_object_t *anna_i_null_function(
    anna_object_t **node_base);

anna_object_t *anna_object_create(
    anna_type_t *type);
anna_object_t *anna_object_create_raw(
    size_t sz);

void anna_object_print(
    anna_object_t *obj);

void anna_mid_init();
/**
   Returns the mid (i.e. the offset in the type vtable) of the specified name. If there is no mid yet, create one.
 */
size_t anna_mid_get(wchar_t *name);
wchar_t *anna_mid_get_reverse(size_t mid);
void anna_mid_put(wchar_t *name, size_t mid);

#endif
