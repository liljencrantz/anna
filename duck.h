#ifndef DUCK_H
#define DUCK_H

#include "util.h"

#define CRASH					\
    {						\
    int *__tmp=0;				\
    *__tmp=0;					\
    }


struct duck_type;
struct duck_object;
struct duck_member;
struct duck_node_call;
struct duck_stack_frame;
struct duck_node_call;
struct duck_function;
struct duck_node_list;
struct duck_node_call;


typedef struct duck_object *(*duck_native_function_t)( struct duck_object ** );
typedef struct duck_node *(*duck_native_macro_t)( struct duck_node_call *, struct duck_function *, struct duck_node_list *);

#define DUCK_FUNCTION_FUNCTION 0
#define DUCK_FUNCTION_MACRO 1
/*
#define DUCK_FUNCTION_CLOSURE 2
#define DUCK_FUNCTION_STANDALONE 4
*/

/*
  The preallocated MIDs
*/
#define DUCK_MID_CALL_PAYLOAD 0
#define DUCK_MID_INT_PAYLOAD 1
#define DUCK_MID_STRING_PAYLOAD 2
#define DUCK_MID_STRING_PAYLOAD_SIZE 3
#define DUCK_MID_CHAR_PAYLOAD 4
#define DUCK_MID_LIST_PAYLOAD 5
#define DUCK_MID_LIST_SIZE 6
#define DUCK_MID_LIST_CAPACITY 7
#define DUCK_MID_FLOAT_PAYLOAD 8
#define DUCK_MID_FUNCTION_WRAPPER_PAYLOAD 10
#define DUCK_MID_FUNCTION_WRAPPER_STACK 11
#define DUCK_MID_FUNCTION_WRAPPER_THIS 12
#define DUCK_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD 13
#define DUCK_MID_TYPE_WRAPPER_PAYLOAD 14
#define DUCK_MID_CALL 15
#define DUCK_MID_FIRST_UNRESERVED 16

union duck_native
{
    duck_native_function_t function;
    duck_native_macro_t macro;
}
  ;

struct duck_type
{
    size_t member_count;
    size_t static_member_count;
    hash_table_t name_lookup;
    wchar_t *name;
    struct duck_member **mid_lookup;
    struct duck_object *wrapper;
    struct duck_object *static_member[];
};

struct duck_member
{
  struct duck_type *type;
  size_t offset;
  int is_static;
  wchar_t name[];
};

struct duck_object
{
    struct duck_type *type;
    struct duck_object *member[];
};

struct duck_function
{
    wchar_t *name;
    struct duck_type *type;
    struct duck_node_call *body;  
    union duck_native native;
    struct duck_type *return_type;
    struct duck_object *wrapper;
    size_t input_count;
    wchar_t **input_name;
    int flags;
    struct duck_object *this;
    struct duck_stack_frame *stack_template;
    struct duck_type *input_type[];
};

struct duck_node_list
{
    struct duck_node *node;
    size_t idx;
    struct duck_node_list *parent;
};


typedef struct duck_type duck_type_t;
typedef struct duck_member duck_member_t;
typedef struct duck_object duck_object_t;
typedef struct duck_frame duck_frame_t;
typedef struct duck_function duck_function_t;
typedef union duck_native duck_native_t;
typedef struct duck_node_list duck_node_list_t;

typedef struct 
{
    duck_type_t *result; 
    size_t argc;
    duck_type_t *argv[];
} duck_function_type_key_t;
   

extern duck_type_t *type_type, *object_type, *int_type, *string_type, *char_type, *null_type,  *string_type, *char_type, *list_type, *float_type;
extern duck_object_t *null_object;
extern int duck_error_count;

/**
   Declare all global, native functions
 */
void duck_function_implementation_init(struct duck_stack_frame *stack);
/**
   Declare all native macros
 */
void duck_macro_init(struct duck_stack_frame *stack);

/**
  Returns the type of the specified member in the specified type
 */
duck_type_t *duck_type_member_type_get(duck_type_t *type, wchar_t *name);
/**
  Return the duck_type_t contained in the specified duck_type_t.wrapper
 */
duck_type_t *duck_type_unwrap(duck_object_t *wrapper);
duck_type_t *duck_type_for_function(duck_type_t *result, size_t argc, duck_type_t **argv);
duck_type_t *duck_type_create(wchar_t *name, size_t static_member_count);

duck_function_t *duck_function_unwrap(duck_object_t *type);
duck_object_t *duck_function_wrapped_invoke(duck_object_t *function, struct duck_node_call *param, struct duck_stack_frame *local);
duck_function_t *duck_function_create(wchar_t *name,
				      int flags,
				      struct duck_node_call *body, 
				      duck_type_t *return_type,
				      size_t argc,
				      duck_type_t **argv,
				      wchar_t **argn,
				      struct duck_stack_frame *parent_stack);


duck_object_t **duck_static_member_addr_get_mid(duck_type_t *type, size_t mid);
duck_object_t **duck_member_addr_get_str(duck_object_t *obj, wchar_t *name);
duck_object_t **duck_member_addr_get_mid(duck_object_t *obj, size_t mid);
duck_object_t *duck_method_wrap(duck_object_t *method, duck_object_t *owner);
size_t duck_member_create(duck_type_t *type,
			  ssize_t mid,
			  wchar_t *name,
			  int is_static,
			  duck_type_t *member_type);
size_t duck_native_method_create(duck_type_t *type,
				 ssize_t mid,
				 wchar_t *name,
				 int flags,
				 duck_native_t func,
				 duck_type_t *result,
				 size_t argc,
				 duck_type_t **argv,
				 wchar_t **argn);
duck_function_t *duck_native_create(wchar_t *name,
				    int flags,
				    duck_native_t native, 
				    duck_type_t *return_type,
				    size_t argc,
				    duck_type_t **argv,
				    wchar_t **argn);




duck_object_t *duck_object_create(duck_type_t *type);
int duck_abides(duck_type_t *contender, duck_type_t *role_model);
int duck_abides_fault_count(duck_type_t *contender, duck_type_t *role_model);
duck_type_t *duck_type_intersect(duck_type_t *t1, duck_type_t *t2);


duck_function_type_key_t *duck_function_unwrap_type(duck_type_t *type);

void duck_error(struct duck_node *node, wchar_t *msg, ...);


#endif
