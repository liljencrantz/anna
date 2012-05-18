#ifndef ANNA_H
#define ANNA_H

#include "anna/util.h"
#include "anna/checks.h"
#include "anna/crash.h"
#include "anna/preproc.h"

struct anna_node;
struct anna_type;
struct anna_object;
struct anna_member;
struct anna_node_call;
struct anna_stack_template;
struct anna_function;
struct anna_context;

typedef void (*anna_native_t)( struct anna_context *stack);
typedef void (*anna_finalizer_t)( struct anna_object *victim);
typedef void (*anna_mark_entry_t)( struct anna_object *this);
typedef void (*anna_mark_type_t)( struct anna_type *this);
typedef int mid_t;

/*
  These values are used by the flags param of the various GC:ed memory
  allocation types in order to determine what type of allocation a
  pointer points to. The memory allocator reserves the first 8 bits of
  every allocation.
*/

#define ANNA_OBJECT 0
#define ANNA_ACTIVATION_FRAME 1
#define ANNA_TYPE 2
#define ANNA_STACK_TEMPLATE 3
#define ANNA_NODE 4
#define ANNA_FUNCTION 5
#define ANNA_BLOB 6
#define ANNA_ALLOC_TYPE_COUNT 7
#define ANNA_FUNCTION_TYPE 8

#define ANNA_ALLOC_FLAGS_SIZE 4
#define ANNA_ALLOC_MASK 15
#define ANNA_MOVE 16
#define ANNA_USED 32

#define ANNA_OBJECT_LIST 512
#define ANNA_OBJECT_HASH 1024

#define ANNA_ACTIVATION_FRAME_STATIC 512
#define ANNA_ACTIVATION_FRAME_BREAK 1024

/**
  If set, this is a static member.
*/
#define ANNA_MEMBER_STATIC 1

/**
  If set, this member has no in-memory representation, i.e. it is a
  property.  
*/
#define ANNA_MEMBER_PROPERTY 2

/**
  If this bit is set, this member points to a data blob that has been
  allocated usin garbage collected memory, and must be marked by the
  appropriate function call during garbage collection.
 */
#define ANNA_MEMBER_ALLOC 4

/**
   If set, this member is a bound method. 
*/
#define ANNA_MEMBER_BOUND 8

/**
   If set, this member is internal
 */
#define ANNA_MEMBER_INTERNAL 16

/**
   Size of the statically allocated chunk of memory used for
   activation records before they are referenced and need to move to
   the heap.
*/
#define ANNA_CONTEXT_STATIC_SZ (8192*32)

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
    ANNA_MID_NEXT_ASSIGN_INT,
    ANNA_MID_PREV_ASSIGN_INT,
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
    ANNA_MID_BUFFER_PAYLOAD,
    ANNA_MID_BUFFER_SIZE,
    ANNA_MID_BUFFER_CAPACITY,
    ANNA_MID_CSTRUCT_PAYLOAD,

    ANNA_MID_CONTINUATION_CALL_COUNT,
    ANNA_MID_CONTINUATION_STACK_COUNT,    
    ANNA_MID_CONTINUATION_ACTIVATION_FRAME,
    ANNA_MID_NAME, 
    ANNA_MID_OUTPUT_TYPE, 
    ANNA_MID_INPUT_TYPE, 
    ANNA_MID_INPUT_NAME, 
    ANNA_MID_DEFAULT_VALUE, 
    ANNA_MID_ATTRIBUTE, 
    ANNA_MID_FILENAME, 

    ANNA_MID_CONTINUATION_VARIABLE_OFFSET,
    ANNA_MID_CONTINUATION_VARIABLE_CONTINUATION,
    ANNA_MID_CHANNEL_READ,
    ANNA_MID_CHANNEL_WRITE,
    ANNA_MID_CHANNEL_SYNC,

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

#define null_entry ((anna_entry_t *)null_object)

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
       A name for this type. A type does not inherently have one
       specific name, it can be renamed and copied just like any other
       variable. This name is here in order to provide better
       introspection ability for humans.
    */
    wchar_t *name;
    /**
       The parent stack of this type, usually the module it was
       defined in.
    */
    struct anna_stack_template *stack;
    /**
      The stack used for macro expansion in this type. Can be
      different from the regular stack because of use vs. expand
      calls.
    */
    struct anna_stack_template *stack_macro;
    /**
       The maximum offset in the mid_identifier array
     */
    size_t mid_count;
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
    hash_table_t specialization;
    /**
       An array list containing all anna_member_t structs of this
       type. Contains exactly the same information as the
       mid_identifier array, but without any gaps, so iterating over
       this list is quicker.
    */
    array_list_t member_list;
    /**
       The number of finalizers for this type.
     */
    size_t finalizer_count;
    /**
       A list of all finalizers to run when deallocating objects of
       this type.
     */
    anna_finalizer_t *finalizer;
    /**
       The function to run in order to mark objects of this type as in
       use during the mark phase of the GC.
    */
    anna_mark_entry_t mark_object;
    /**
       The function to run in order to mark this type as in use during
       the mark phase of the GC.
    */
    anna_mark_type_t mark_type;

    /**
       Internal variable that may be used by the mark_object function
    */
    int *mark_entry;
    /**
       Internal variable that may be used by the mark_object function
    */
    int mark_entry_count;
    /**
       Internal variable that may be used by the mark_object function
    */
    int *mark_blob;
    /**
       Internal variable that may be used by the mark_object function
    */
    int mark_blob_count;

    /**
       Internal variable that may be used by the mark_type function
    */
    int *static_mark_entry;
    /**
       Internal variable that may be used by the mark_type function
    */
    int static_mark_entry_count;
    /**
       Internal variable that may be used by the mark_type function
    */
    int *static_mark_blob;
    /**
       Internal variable that may be used by the mark_type function
    */
    int static_mark_blob_count;
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
       The storage flag for this member
     */
    int storage;
    
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
    
    struct anna_node_call *attribute;
    /**
       The name of this member.
    */
    wchar_t *name;
    /**
       A (possible) documentation strings for this member. If only one
       documentation string is present, we add it here and don't
       create a real attribute AST, which saves us a fair bit of
       memory.
    */
    wchar_t *doc;
};

/**
   The struct that represents any Anna object.

   If we really want to conserve memory, we could probably turn the
   flags variable into a short and store a short int offset into an
   array of types instead of an actual type pointer. Should save us
   one word of space per allocated object at the cost of an ever so
   slightly more expensive object type lookup, a maximum of 65000
   types and only sixteen possible flag values.

   Given that both int:s and floats are usually in their native
   non-object forms, this shouldn't be that meaningful.
*/
struct anna_object
{
    /**
       Various bit flags for this object. The first 8 bits of this
       flag are reserved by the memory allocator and should not be
       touched.
    */
    int flags;
    /**
       The type of this object
     */
    struct anna_type *type;
    /**
       All the non-static member values of this object. To decode what
       member lives at what offset, use the information stored in the
       type object.
    */
    anna_entry_t *member[];
};

/**
   Line pairs are pairs of integers used to represent a code
   offset. These are used in order to provide the user with source
   code lines in stack traces.
 */
struct anna_line_pair
{
    /**
       The code offset
    */
    int offset;
    /**
       The line number 
    */
    int line;
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
       The body of this function. Only used during compilation.
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
       If this function is a method, the mid this method has in the
       type it is a member of
    */
    mid_t mid;
    /**
       The full AST that originally defined this function. Not macro
       expanded. Needed for template specialization. Only used during
       compilation.
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
      The default value (if any) for each input argument
    */
    struct anna_node **input_default;
    /**
       Bytecode
    */
    char *code;
    /**
       The memory size required to hold an activation frame for
       executing this function
    */
    size_t frame_size;
    /**
       The number of variables this function has, including input parameters
    */
    size_t variable_count;
    /**
       Number of elements in the line_offset array
    */
    size_t line_offset_count;
    /**
      A mapping from code offsets to line numbers
    */
    struct anna_line_pair *line_offset;
    /**
       The file in which this function was definied
    */
    wchar_t *filename;
    /**
       A hash of all template specializations of this function.
    */
    hash_table_t specialization;
};

/**
   All the variables of a function call instance are stored in an
   activation frame (sometimes also called activation record or stack
   frame), together with some other data about the current execution
   context.
 */
struct anna_activation_frame
{
    int flags;
    /**
       The static context of this frame is the frame holds the parent
       scope of this frame. It is used for variable lookup. (lexical
       scoping)
    */
    struct anna_activation_frame *static_frame;
    /**
       The dynamic context of this frame is the frame that
       called/created this frame. It is used when returning from
       a function call.
    */
    struct anna_activation_frame *dynamic_frame;
    
    /**
       The function this frame is executing
    */
    struct anna_function *function;
    /**
       The current code position
    */
    char *code;
    /**
       The top element of the temp stack at the time this frame was
       created. It should be restored upon return.
    */
    anna_entry_t **return_stack_top;
    
    /**
       Slots for all input parameters and member variables.
    */
    anna_entry_t *slot[];
};

/**
   An execution context for an Anna thread. It contains a pointer to
   the current activation frame, a stack used as a temporary scratch
   area for incomplete function parameter lists and a few bookkeeping
   members.
 */
struct anna_context
{
    /**
      The total amount of memory used by this object
    */
    size_t size;
    
    /**
      The activation frame holds the values of all variables of the
      currently executing function, as well as a pointer to the
      current position in the source code.
    */
    struct anna_activation_frame *frame;
    /**
      function_object is the anna object representing the function
      currently being executed. Use anna_function_unwrap to unwrap it
      and access the actual anna_function_t. This value is rarely useful.
    */
    struct anna_object *function_object;

    char *static_frame_ptr;
    char static_frame_data[ANNA_CONTEXT_STATIC_SZ];
    
    /**
      The top of the scratch stack. The scratch stack in anna is not used
      for passing parameters, it is used purely as a scratch space for
      storing output of function calls that will in turn be used as
      input to future function calls.
    */
    anna_entry_t **top;
    /**
      The base of the scratch stack.
    */
    anna_entry_t *stack[];
};

typedef struct anna_activation_frame anna_activation_frame_t;
typedef struct anna_context anna_context_t;
typedef struct anna_type anna_type_t;
typedef struct anna_member anna_member_t;
typedef struct anna_object anna_object_t;
typedef struct anna_function anna_function_t;
typedef struct anna_line_pair anna_line_pair_t;

/**
  This struct describes the signature of a function, which includes
  the return type, the number and types of inputs and any default
  parameter values. 

  The flags variable tracks whether this function is variadic and whether
  it can be used as a macro using the regular ANNA_FUNCTION_XXX constants.
*/
typedef struct 
{
    int flags;
    anna_type_t *return_type;
    size_t input_count;
    wchar_t **input_name;
    struct anna_node **input_default;
    anna_type_t *input_type[];
} anna_function_type_t;

/**
   A bunch of internal types. Might be better to move these to e.g. lib/lang/lang.h.
*/
extern anna_type_t *type_type, *object_type, *int_type, *string_type, 
    *mutable_string_type, *imutable_string_type, *char_type, *null_type,
    *string_type, *char_type, *float_type, *member_type, *range_type, 
    *complex_type, *hash_type, *pair_type, *buffer_type, 
    *function_type_base, *continuation_type, *continuation_variable_type, *sendable_type;
extern anna_object_t *null_object, *anna_wrap_method;
/**
   Error counter. Every time the parser or compiler detects a
   non-critical error, this counter is increased.
 */
extern int anna_error_count;
/**
   The global namespace
 */
extern struct anna_stack_template *stack_global;

/**
   The number of arguments given to the anna process
 */
extern int anna_argc;

/**
   The arguments given to the anna process
 */
extern char **anna_argv;

static inline int anna_member_is_static(anna_member_t *member)
{
    return !!(member->storage & ANNA_MEMBER_STATIC);
}

/**
   Round the size up to the nearest even 8 bytes
 */
static inline size_t anna_align(size_t sz)
{
    return (((sz-1)/8)+1)*8;
}

/**
   Declare all global, native functions
 */
__cold void anna_function_implementation_init(
    struct anna_stack_template *stack);

/**
   \param macro the macro to invoke
   \param node the ast node to transform
*/
__cold struct anna_node *anna_macro_invoke(
    anna_function_t *macro,
    struct anna_node_call *node);

/**
   Prints a description of the specified function type on standard out
 */
__cold void anna_function_type_print(
    anna_function_type_t *k);

/**
   Return the memory address location for the object entry with the
   specified mid in the specified object, or a null pointer if it does
   not exist.
*/
static __pure inline anna_entry_t **anna_entry_get_addr(
    anna_object_t *obj, mid_t mid)
{
    if(mid >= obj->type->mid_count)
    {
	return 0;
    }
    anna_member_t *m = obj->type->mid_identifier[mid];
    if(!m)
    {
	return 0;
    }

//    wprintf(L"Get member %ls in object of type %ls\n", anna_mid_get_reverse(mid), obj->type->name);
    
    if(anna_member_is_static(m)) {
	return &obj->type->static_member[m->offset];
    } else {
	return &(obj->member[m->offset]);
    }
}

/**
   Assigne the specified value to the member with the specified mid in
   the specified object.

   No error checking performed; this will cause a crash if no such
   member exist.
 */
static inline void anna_entry_set(
    anna_object_t *obj, mid_t mid, anna_entry_t *value)
{
    *anna_entry_get_addr(obj, mid) = value;
}

/**
   Return the object entry with the specified mid in the specified
   object. Causes undefined behavior if the object has no such
   member. If you need to check if the entry exists, use
   anna_entry_get_addr instead.
 */
static __pure inline anna_entry_t *anna_entry_get(
    anna_object_t *obj, mid_t mid)
{
    anna_member_t *m = obj->type->mid_identifier[mid];
    if(anna_member_is_static(m)) {
	return obj->type->static_member[m->offset];
    } else {
	return (obj->member[m->offset]);
    }
}

/**
   Return the memory address location for the object entry with the
   specified mid in the specified type, or a null pointer if it does
   not exist. If the entry does not exist, or is not static, null is returned.
 */
static __pure inline anna_entry_t **anna_entry_get_addr_static(
    anna_type_t *type, mid_t mid)
{
    if(mid >= type->mid_count)
    {
	return 0;
    }
    anna_member_t *m = type->mid_identifier[mid];
    if(!m)
    {
	return 0;
    }

    if(anna_member_is_static(m)) {
	return &type->static_member[m->offset];
    } else {
	return 0;
    }
}

/**
   Return the object entry with the specified mid in the specified
   type. Causes undefined behavior if the object has no such member or
   if the member is not static. If you need to check if the entry
   exists, use anna_entry_get_addr_static instead.
 */
static __pure inline anna_entry_t *anna_entry_get_static(
    anna_type_t *type, mid_t mid)
{
    anna_member_t *m = type->mid_identifier[mid];
    return type->static_member[m->offset];
}

/**
   Returns non-zero if the specified contender implements all the
   members of role_model, e.g. if it can be used in place of
   role_model using structural typing.
 */
int anna_abides(
    anna_type_t *contender, anna_type_t *role_model);

/**
  Search the specified list of function types and return the index of
  a function matching the specified call parameter list. If there are
  multiple matches, the match that has smallest difference in number
  of non-matching members. If there are no matching members, -1 is
  returned.
*/
int anna_abides_search(
    struct anna_node_call *call,
    anna_function_type_t **function,
    size_t function_count);

/**
   Like anna_abised, except it counts the number of members that are
   not implemented. Useful when wanting to find out how far away from
   fully implementing an interface a given type is.
 */
int anna_abides_fault_count(
    anna_type_t *contender, anna_type_t *role_model);
/**
   Init function for the abides lib. 
 */
__cold void anna_abides_init(void);

/**
   Print the specified error message and increase the error counter. 
 */
__cold void anna_error(
    struct anna_node *node, wchar_t *msg, ...);

/**
   Create and return a new object of the specified type.
 */
__hot __malloc anna_object_t *anna_object_create(
    anna_type_t *type);
/**
   Same as anna_object_create, but the members of the object are not assigned null values,
 */
__hot __malloc anna_object_t *anna_object_create_raw(
    size_t sz);

/**
   Print a description of the specified object
 */
__cold void anna_object_print(
    anna_object_t *obj);

#endif
