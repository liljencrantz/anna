#ifndef ANNA_H
#define ANNA_H

#include "util.h"
#include "anna_checks.h"
#include "anna_crash.h"
#include "anna_preproc.h"

struct anna_type;
struct anna_object;
struct anna_member;
struct anna_node_call;
struct anna_stack_template;
struct anna_function;
struct anna_node_list;
struct anna_vmstack;

typedef struct anna_vmstack *(*anna_native_t)( struct anna_vmstack *, struct anna_object *);
typedef ssize_t mid_t;

/*
  These values are used by the flags param of the various GC:ed memory
  allocation types in order to determine what type of allocation a
  pointer points to. The memory allocator reserves the first 8 bits of every allocation.
 */

#define ANNA_TYPE 0
#define ANNA_OBJECT 1
#define ANNA_STACK_TEMPLATE 2
#define ANNA_NODE 3
#define ANNA_VMSTACK 4
#define ANNA_FUNCTION 5
#define ANNA_BLOB 6
#define ANNA_ALLOC_FLAGS_SIZE 4
#define ANNA_ALLOC_MASK 15
#define ANNA_MOVE 16
#define ANNA_USED 32

#define ANNA_OBJECT_LIST 512

#define ANNA_VMSTACK_STATIC 512
#define ANNA_VMSTACK_BREAK 1024

/*
  The preallocated MIDs
*/
enum anna_mid_enum
{
    ANNA_MID_INT_PAYLOAD=0,
    ANNA_MID_STRING_PAYLOAD,
    ANNA_MID_STRING_PAYLOAD_SIZE,
    ANNA_MID_CHAR_PAYLOAD,
    ANNA_MID_LIST_PAYLOAD,
    ANNA_MID_LIST_SIZE,
    ANNA_MID_LIST_CAPACITY,
    ANNA_MID_LIST_SPECIALIZATION,
    ANNA_MID_FUNCTION_WRAPPER_PAYLOAD,
    ANNA_MID_FUNCTION_WRAPPER_STACK,

    ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD,
    ANNA_MID_TYPE_WRAPPER_PAYLOAD,
    ANNA_MID_INIT_PAYLOAD,
    ANNA_MID_NODE_PAYLOAD,
    ANNA_MID_MEMBER_PAYLOAD,
    ANNA_MID_MEMBER_TYPE_PAYLOAD,
    ANNA_MID_STACK_PAYLOAD,
    ANNA_MID_STACK_TYPE_PAYLOAD,
    ANNA_MID_FROM,
    ANNA_MID_TO,

    ANNA_MID_STEP,
    ANNA_MID_FLOAT_PAYLOAD,
    ANNA_MID_EQ,
    ANNA_MID_NEQ,
    ANNA_MID_LT,
    ANNA_MID_LTE,
    ANNA_MID_GTE,
    ANNA_MID_GT,
    ANNA_MID_RANGE_FROM,
    ANNA_MID_RANGE_TO,

    ANNA_MID_RANGE_STEP,
    ANNA_MID_RANGE_OPEN,
    ANNA_MID_DEL,
    ANNA_MID_COMPLEX_PAYLOAD,
    ANNA_MID_HASH_CODE,
    ANNA_MID_CMP,
    ANNA_MID_HASH_PAYLOAD,
    ANNA_MID_HASH_SPECIALIZATION1,
    ANNA_MID_HASH_SPECIALIZATION2,
    ANNA_MID_TO_STRING,

    ANNA_MID_PAIR_SPECIALIZATION1,
    ANNA_MID_PAIR_SPECIALIZATION2,
    ANNA_MID_PAIR_FIRST,
    ANNA_MID_PAIR_SECOND,
    ANNA_MID_CONTINUATION_STACK,
    ANNA_MID_CONTINUATION_CODE_POS,
    ANNA_MID_THIS,
    ANNA_MID_METHOD,
    ANNA_MID_ADD_INT,
    ANNA_MID_SUB_INT,

    ANNA_MID_MUL_INT,
    ANNA_MID_DIV_INT,
    ANNA_MID_INCREASE_ASSIGN_INT,
    ANNA_MID_DECREASE_ASSIGN_INT,
    ANNA_MID_BITAND_INT,
    ANNA_MID_BITOR_INT,
    ANNA_MID_BITXOR_INT,
    ANNA_MID_ADD_FLOAT,
    ANNA_MID_SUB_FLOAT,
    ANNA_MID_MUL_FLOAT,

    ANNA_MID_DIV_FLOAT,
    ANNA_MID_EXP_FLOAT,
    ANNA_MID_INCREASE_ASSIGN_FLOAT,
    ANNA_MID_DECREASE_ASSIGN_FLOAT,
    ANNA_MID_FIRST_UNRESERVED,
};

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

typedef struct {} anna_entry_t;

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

    size_t object_size;
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
    anna_entry_t **static_member;
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
       If true, this member is a bound method. 
    */
    int is_bound_method;
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
    anna_entry_t *member[];
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
       Node describing the return type of this function. Only used during compilation.
     */
    struct anna_node *return_type_node;  
    /**
       Node describing the input types of this function. Only used during compilation.
     */
    struct anna_node_call *input_type_node;  
    /**
       The mid this method has in the type it is a member of
     */
    mid_t mid;
    /**
       The full AST that originally defined this function. Not macro
       expanded. Needed for template specialization.
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
    anna_native_t native;
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
    anna_entry_t **top;
    anna_entry_t *base[];
};

typedef struct anna_vmstack anna_vmstack_t;


typedef struct anna_type anna_type_t;
typedef struct anna_member anna_member_t;
typedef struct anna_object anna_object_t;
typedef struct anna_frame anna_frame_t;
typedef struct anna_function anna_function_t;
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
extern anna_type_t *function_type_base;
extern anna_object_t *null_object, *anna_wrap_method;
extern int anna_error_count;
extern struct anna_stack_template *stack_global;

extern int anna_argc;
extern char **anna_argv;


static inline size_t anna_align(size_t sz)
{
    return (((sz-1)/8)+1)*8;
}


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

static __pure inline anna_entry_t **anna_entry_get_addr(
    anna_object_t *obj, mid_t mid)
{
    
    anna_member_t *m = obj->type->mid_identifier[mid];
//    wprintf(L"Get member %ls in object of type %ls\n", anna_mid_get_reverse(mid), obj->type->name);
    
    if(unlikely(!m)) 
    {
	return 0;
    }
    if(m->is_static) {
	return &obj->type->static_member[m->offset];
    } else {
	return &(obj->member[m->offset]);
    }
}

static __pure inline anna_entry_t *anna_entry_get(
    anna_object_t *obj, mid_t mid)
{
    return *anna_entry_get_addr(obj, mid);
}


static __pure inline anna_entry_t **anna_entry_get_addr_static(
    anna_type_t *type, mid_t mid)
{
    anna_member_t *m = type->mid_identifier[mid];
    if(unlikely(!m)) 
    {
	return 0;
    }

    if(m->is_static) {
	return &type->static_member[m->offset];
    } else {
	return 0;
    }
}

static __pure inline anna_entry_t *anna_entry_get_static(
    anna_type_t *type, mid_t mid)
{
    return *anna_entry_get_addr_static(type, mid);
}

int anna_abides(
    anna_type_t *contender, anna_type_t *role_model);

int anna_abides_fault_count(
    anna_type_t *contender, anna_type_t *role_model);
void anna_abides_init(void);


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

void anna_add_subtype(
    anna_type_t *parent,
    anna_type_t *child);

#endif
