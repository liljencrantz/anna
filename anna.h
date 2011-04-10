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

#if __GNUC__ >= 3
/* Tell the compiler which outcome in an if block is the likelier, so
 * that the code can be laid out in an optimal manner */
# define likely(x) __builtin_expect((x),1)
/* Tell the compiler which outcome in an if block is the likelier, so
 * that the code can be laid out in an optimal manner */
# define unlikely(x) __builtin_expect((x),0)
/* No side effects */
# define __pure		__attribute__ ((pure))
/* Like __pure, but stricteer. Not even read-only checking of globals or pointers */
# define __const	__attribute__ ((const))
/* Function never returns */
# define __noreturn	__attribute__ ((noreturn))
/* Return value can not be aliased */
# define __malloc	__attribute__ ((malloc))
/* Warn if return value is not used */
# define __must_check	__attribute__ ((warn_unused_result))
/* Warn if function is used */
# define __deprecated	__attribute__ ((deprecated))
/* Don't watn if static function never called, still compile */
# define __used		__attribute__ ((used))
/* Don't warn if specified function parameter is never used */
# define __unused	__attribute__ ((unused))
/* Ignore alignment of struct */
# define __packed	__attribute__ ((packed))
# define __sentinel	__attribute__ ((sentinel))
#else
# define __pure		/* no pure */
# define __const	/* no const */
# define __noreturn	/* no noreturn */
# define __malloc	/* no malloc */
# define __must_check	/* no warn_unused_result */
# define __deprecated	/* no deprecated */
# define __used		/* no used */
# define __unused	/* no unused */
# define __packed	/* no packed */
# define __sentinel	/* no sentinel */
# define likely(x)	(x)
# define unlikely(x)	(x)
#endif
struct anna_type;
struct anna_object;
struct anna_member;
struct anna_node_call;
struct anna_stack_template;
struct anna_function;
struct anna_node_list;
struct anna_vmstack;

typedef struct anna_vmstack *(*anna_native_function_t)( struct anna_vmstack *, struct anna_object *);
typedef struct anna_node *(*anna_native_macro_t)( struct anna_node_call *);
typedef ssize_t mid_t;

#define ANNA_TYPE 0
#define ANNA_OBJECT 1
#define ANNA_STACK_TEMPLATE 2
#define ANNA_NODE 3
#define ANNA_VMSTACK 4
#define ANNA_FUNCTION 5
#define ANNA_ALLOC_MASK 15
#define ANNA_MOVE 16
#define ANNA_USED 32

#define ANNA_FUNCTION_VARIADIC 512
#define ANNA_FUNCTION_MACRO 1024
#define ANNA_FUNCTION_PREPARED_INTERFACE 2048
#define ANNA_FUNCTION_PREPARED_BODY 4096
#define ANNA_FUNCTION_BLOCK 8192
#define ANNA_FUNCTION_CONTINUATION (8192*2)

#define ANNA_TYPE_REGISTERED 512
#define ANNA_TYPE_PREPARED_INTERFACE 1024
#define ANNA_TYPE_PREPARED_IMPLEMENTATION 2048
#define ANNA_TYPE_COMPILED 4096


/*
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
#define ANNA_MID_LIST_SPECIALIZATION 8
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
#define ANNA_MID_FLOAT_PAYLOAD 24
#define ANNA_MID_EQ 25
#define ANNA_MID_RANGE_FROM 26
#define ANNA_MID_RANGE_TO 27
#define ANNA_MID_RANGE_STEP 28
#define ANNA_MID_RANGE_OPEN 29
#define ANNA_MID_DEL 30
#define ANNA_MID_COMPLEX_PAYLOAD 31
#define ANNA_MID_HASH_CODE 32
#define ANNA_MID_CMP 33
#define ANNA_MID_HASH_PAYLOAD 34
#define ANNA_MID_HASH_SPECIALIZATION1 35
#define ANNA_MID_HASH_SPECIALIZATION2 36
#define ANNA_MID_TO_STRING 37
#define ANNA_MID_PAIR_SPECIALIZATION1 38
#define ANNA_MID_PAIR_SPECIALIZATION2 39
#define ANNA_MID_PAIR_FIRST 40
#define ANNA_MID_PAIR_SECOND 41
#define ANNA_MID_CONTINUATION_STACK 42
#define ANNA_MID_CONTINUATION_CODE_POS 43
#define ANNA_MID_FIRST_UNRESERVED 44

/**
   type->member_blob value. If set, the value of this member is a
   regular object.
  */
#define ANNA_GC_OBJECT 0
/**
   type->member_blob value. If set, the value of this blob is opaque
   and should not be traversed by thr GC.
  */
#define ANNA_GC_BLOB 1
/**
   type->member_blob value. If set, the value of this blob is a
   pointer that shoud be traversed by the GC
*/
#define ANNA_GC_ALLOC 2


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
       A bitfield containing various additional info on this type.
    */
    int flags;
    
    int *member_blob;
    int *static_member_blob;
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
       A stack template view of this type
    */
    struct anna_stack_template *stack;
    struct anna_stack_template *stack_macro;
    /**
       An array containing all member structs. The offset is a mid.
    */
    struct anna_member **mid_identifier;
    
    /**
       The full AST that originally defined this type, before
       templating, macro expansion, etc. This is the whole AST,
       including atributes, type name, etc.
       
       Native types do not have a definition.
    */
    struct anna_node_call *definition;
    /**
       The AST that defines the members of this type. This is the
       parsed, macro expanded version of the AST, and only includes
       the actual body block.
       
       Native types do not have a body.
    */
    struct anna_node_call *body;
    /**
       Attribute list for this type
     */
    struct anna_node_call *attribute;
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
       A hash of all template specializations of this type.
    */
    hash_table_t specializations;
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
    ssize_t offset;
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

       -1 if member is a property but unused;
     */
    ssize_t setter_offset;
    /**
       Only used if the member is a property. Gives the offset of the
       getter method. Note that all non-static variables are
       implicitly properties.

       -1 if member is a property but unused;
    */
    ssize_t getter_offset;    
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
    int flags;
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
       A bitmask containing various additional info on this type. For
       possible values, see the ANNA_TYPE_* constants.
    */
    int flags;
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
       The mid this method has in the type it is a member of
     */
    mid_t mid;
    /**
       The full AST that originally defined this function. 
    */
    struct anna_node_call *definition;
    /**
       The attribute list for this function
     */
    struct anna_node_call *attribute;
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
       If this function is a bound method, this field contains a curried this pointer.
    */
    struct anna_object *this;
    /**
       The template stack frame of this function
    */
    struct anna_stack_template *stack_template;
    /**
       The number of input arguments to this function.
     */
    size_t input_count;
    /**
       The name of each input argument
     */
    wchar_t **input_name;
    /**
       The type of each input argument
     */
    struct anna_type **input_type;    
    /**
       Bytecode
     */
    char *code;
    size_t frame_size;
    size_t variable_count;
};

#define ANNA_IS_MACRO(f) (!!((f)->flags & ANNA_FUNCTION_MACRO))
#define ANNA_IS_VARIADIC(f) (!!((f)->flags & ANNA_FUNCTION_VARIADIC))

struct anna_node_list
{
    struct anna_node *node;
    size_t idx;
    struct anna_node_list *parent;
};

struct anna_vmstack
{
    int flags;
    struct anna_vmstack *parent;    
    struct anna_vmstack *caller;    
    struct anna_function *function;
    char *code;    
    struct anna_object **top;
    struct anna_object *base[];
};

typedef struct anna_vmstack anna_vmstack_t;


typedef struct anna_type anna_type_t;
typedef struct anna_member anna_member_t;
typedef struct anna_object anna_object_t;
typedef struct anna_frame anna_frame_t;
typedef struct anna_function anna_function_t;
typedef union anna_native anna_native_t;
typedef struct anna_node_list anna_node_list_t;

typedef struct 
{
    anna_type_t *return_type;
    size_t input_count;
    int flags;
    wchar_t **input_name;
    anna_type_t *input_type[];
} anna_function_type_t;

extern anna_type_t *type_type, *object_type, *int_type, *string_type, *char_type, *null_type,  *string_type, *char_type, *list_type, *float_type, *member_type, *range_type, *complex_type, *hash_type, *pair_type;
extern anna_object_t *null_object;
extern int anna_error_count;
extern struct anna_stack_template *stack_global;

/**
   Declare all global, native functions
 */
void anna_function_implementation_init(
    struct anna_stack_template *stack);

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
    struct anna_stack_template *local);

anna_object_t *anna_function_invoke_values(
    anna_function_t *function, 
    anna_object_t *this,
    anna_object_t **param,
    struct anna_stack_template *outer);

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
    struct anna_stack_template *param_invoke_stack,
    struct anna_stack_template *function_parent_stack);





/**
   \param macro the macro to invoke
   \param node the ast node to transform
*/
struct anna_node *anna_macro_invoke(
    anna_function_t *macro,
    struct anna_node_call *node);

void anna_function_type_print(
    anna_function_type_t *k);

anna_object_t *anna_construct(
    anna_type_t *type, struct anna_node_call *param,
    struct anna_stack_template *stack);

__pure anna_object_t **anna_static_member_addr_get_mid(
    anna_type_t *type, mid_t mid);

__pure anna_object_t **anna_member_addr_get_mid(
    anna_object_t *obj, mid_t mid);

anna_object_t *anna_method_wrap(
    anna_object_t *method, 
    anna_object_t *owner);

size_t anna_native_method_create(
    anna_type_t *type,
    mid_t mid,
    wchar_t *name,
    int flags,
    anna_native_function_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn);

size_t anna_method_create(
    anna_type_t *type,
    mid_t mid,
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

anna_object_t *anna_object_create(
    anna_type_t *type);
anna_object_t *anna_object_create_raw(
    size_t sz);

void anna_object_print(
    anna_object_t *obj);

void anna_mid_init(void);
void anna_mid_destroy(void);

/**
   Returns the mid (i.e. the offset in the type vtable) of the
   specified name. If there is no mid yet, create one.
 */
size_t anna_mid_get(wchar_t *name);
wchar_t *anna_mid_get_reverse(mid_t mid);
void anna_mid_put(wchar_t *name, mid_t mid);
size_t anna_mid_max_get(void);
anna_member_t **anna_mid_identifier_create(void);
size_t anna_mid_get_count(void);




#endif
