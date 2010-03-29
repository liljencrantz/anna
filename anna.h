#ifndef ANNA_H
#define ANNA_H

#include "util.h"
#include "anna_checks.h"



#ifdef ANNA_CRASH_ON_CRITICAL_ENABLED
#define CRASH					\
    {						\
    int *__tmp=0;				\
    *__tmp=0;					\
    exit(1);					\
    }
#else
define CRASH exit(1)
#endif

#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
#define ANNA_PREPARED(n) ((n)->prepared=1)
#define ANNA_CHECK_NODE_PREPARED(n)  if(!(n)->prepared)			\
    {									\
	anna_error(n,L"Critical: Tried to invoke unprepared AST node");	\
	anna_node_print(n);						\
	wprintf(L"\n");							\
	CRASH;								\
    }
#else
#define ANNA_PREPARED(n) 
#define ANNA_CHECK_NODE_PREPARED(n) 
#endif

#define likely(x) (x)

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
typedef struct anna_node *(*anna_native_macro_t)( struct anna_node_call *, struct anna_function *, struct anna_node_list *);

#define ANNA_FUNCTION_MACRO 1
#define ANNA_FUNCTION_VARIADIC 2
#define ANNA_FUNCTION_PREPARED 4

#define ANNA_TYPE_PREPARED 1

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
#define ANNA_MID_FUNCTION_WRAPPER_PAYLOAD 10
#define ANNA_MID_FUNCTION_WRAPPER_STACK 11
#define ANNA_MID_FUNCTION_WRAPPER_THIS 12
#define ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD 13
#define ANNA_MID_TYPE_WRAPPER_PAYLOAD 14
#define ANNA_MID_CALL 15
#define ANNA_MID_INIT_PAYLOAD 16
#define ANNA_MID_NODE_PAYLOAD 17
#define ANNA_MID_FIRST_UNRESERVED 18

union anna_native
{
    anna_native_function_t function;
    anna_native_macro_t macro;
}
  ;

struct anna_type
{
    size_t member_count;
    size_t property_count;
    size_t static_member_count;
    hash_table_t name_identifier;
    wchar_t *name;
    int flags;
    struct anna_member **mid_identifier;
    struct anna_node_call *definition;
    struct anna_object *wrapper;
    struct anna_object *static_member[];
};

struct anna_member
{
    struct anna_type *type;
    size_t offset;
    int is_static;
    int is_property;
    int is_method;
    size_t setter_offset;
    size_t getter_offset;    
    wchar_t name[];
};

struct anna_object
{
    struct anna_type *type;
    struct anna_object *member[];
};

struct anna_function
{
    wchar_t *name;
    struct anna_type *type;
    struct anna_node_call *body;  
    union anna_native native;
    struct anna_type *return_type;
    struct anna_object *wrapper;
    size_t input_count;
    wchar_t **input_name;
    int flags;
    int return_pop_count;
    struct anna_object *this;
    struct anna_stack_frame *stack_template;
    array_list_t child_function;
    array_list_t child_type;
    struct anna_type *input_type[];    
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
   

extern anna_type_t *type_type, *object_type, *int_type, *string_type, *char_type, *null_type,  *string_type, *char_type, *list_type, *float_type;
extern anna_object_t *null_object;
extern int anna_error_count;

/**
   Declare all global, native functions
 */
void anna_function_implementation_init(struct anna_stack_frame *stack);

/**
   Declare all native macros
 */
void anna_macro_init(struct anna_stack_frame *stack);

/**
  Returns the type of the specified member in the specified type
 */
anna_type_t *anna_type_member_type_get(anna_type_t *type, wchar_t *name);

/**
  Return the anna_type_t contained in the specified anna_type_t.wrapper
 */
anna_type_t *anna_type_unwrap(anna_object_t *wrapper);

anna_type_t *anna_type_for_function(anna_type_t *result, size_t argc, anna_type_t **argv, wchar_t **argn, int is_variadic);

anna_type_t *anna_type_create(wchar_t *name, size_t static_member_count, int fake_definition);

anna_object_t *anna_function_wrapped_invoke(anna_object_t *function,
					    anna_object_t *this,
					    struct anna_node_call *param, 
					    struct anna_stack_frame *local);

anna_object_t *anna_function_invoke_values(anna_function_t *function, 
					   anna_object_t *this,
					   anna_object_t **param,
					   struct anna_stack_frame *outer);

anna_object_t *anna_function_invoke(anna_function_t *function, 
				    anna_object_t *this,
				    struct anna_node_call *param,
				    struct anna_stack_frame *stack,
				    struct anna_stack_frame *outer);

struct anna_node *anna_macro_invoke(
    anna_function_t *macro, 
    struct anna_node *node,
    anna_function_t *function,
    anna_node_list_t *parent);

anna_function_t *anna_function_create(wchar_t *name,
				      int flags,
				      struct anna_node_call *body, 
				      anna_type_t *return_type,
				      size_t argc,
				      anna_type_t **argv,
				      wchar_t **argn,
				      struct anna_stack_frame *parent_stack,
				      int return_pop_count);

anna_object_t *anna_construct(anna_type_t *type, struct anna_node_call *param, struct anna_stack_frame *stack);

anna_object_t **anna_static_member_addr_get_mid(anna_type_t *type, size_t mid);

//anna_object_t **anna_static_member_addr_get_str(anna_type_t *type, wchar_t *name);

anna_object_t **anna_member_addr_get_str(anna_object_t *obj, wchar_t *name);

anna_object_t **anna_member_addr_get_mid(anna_object_t *obj, size_t mid);

anna_object_t *anna_method_wrap(anna_object_t *method, anna_object_t *owner);

size_t anna_member_create(anna_type_t *type,
			  ssize_t mid,
			  wchar_t *name,
			  int is_static,
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

anna_function_t *anna_native_create(wchar_t *name,
				    int flags,
				    anna_native_t native, 
				    anna_type_t *return_type,
				    size_t argc,
				    anna_type_t **argv,
				    wchar_t **argn);

size_t anna_method_create(anna_type_t *type,
			  ssize_t mid,
			  wchar_t *name,
			  int flags,
			  anna_function_t *definition);

size_t anna_mid_get(wchar_t *name);
wchar_t *anna_mid_get_reverse(size_t mid);

anna_object_t *anna_object_create(anna_type_t *type);

int anna_abides(anna_type_t *contender, anna_type_t *role_model);

int anna_abides_fault_count(anna_type_t *contender, anna_type_t *role_model);

anna_type_t *anna_type_intersect(anna_type_t *t1, anna_type_t *t2);

void anna_error(struct anna_node *node, wchar_t *msg, ...);

int anna_type_setup(anna_type_t *type, 
		    anna_function_t *function, 
		    anna_node_list_t *parent);

anna_object_t *anna_i_null_function(anna_object_t **node_base);

void anna_native_declare(struct anna_stack_frame *stack,
			 wchar_t *name,
			 int flags,
			 anna_native_t func,
			 anna_type_t *result,
			 size_t argc, 
			 anna_type_t **argv,
			 wchar_t **argn);


#endif
