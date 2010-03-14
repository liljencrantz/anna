#ifndef ANNA_NODE_H
#define ANNA_NODE_H

#include <stdio.h>

#include "anna.h"
#include "anna_stack.h"

#define ANNA_NODE_CALL 0
#define ANNA_NODE_IDENTIFIER 1
#define ANNA_NODE_INT_LITERAL 2
#define ANNA_NODE_STRING_LITERAL 3
#define ANNA_NODE_CHAR_LITERAL 4
#define ANNA_NODE_FLOAT_LITERAL 5
#define ANNA_NODE_NULL 6
#define ANNA_NODE_DUMMY 7
#define ANNA_NODE_TRAMPOLINE 8
#define ANNA_NODE_ASSIGN 9
#define ANNA_NODE_MEMBER_GET 10
#define ANNA_NODE_MEMBER_GET_WRAP 11
#define ANNA_NODE_MEMBER_SET 12
#define ANNA_NODE_CONSTRUCT 13
#define ANNA_NODE_RETURN 14

struct YYLTYPE
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
    wchar_t *filename;
};

typedef struct YYLTYPE anna_location_t;
typedef struct YYLTYPE YYLTYPE;
#define yyltype YYLTYPE
#define YYLTYPE YYLTYPE

#define ANNA_CHECK_NODE_PREPARED_ENABLED 

#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
#define ANNA_PREPARED(n) n->prepared=1
#define ANNA_CHECK_NODE_PREPARED(n)  if(!n->prepared)			\
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

struct anna_node
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
};

struct anna_node_identifier
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    wchar_t *name;
    anna_sid_t sid;
};

struct anna_node_assign
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    anna_sid_t sid;
    struct anna_node *value;
};

struct anna_node_member_get
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    struct anna_node *object;
    size_t mid;
    struct anna_type *type;
};

struct anna_node_member_set
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    struct anna_node *object;
    struct anna_node *value;
    size_t mid;
    struct anna_type *type;
};


struct anna_node_call
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    struct anna_node *function;
    size_t child_count;
    size_t child_capacity;
    struct anna_node **child;
};

struct anna_node_string_literal
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    size_t payload_size;
    wchar_t *payload;
};

struct anna_node_char_literal
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    wchar_t payload;
};

struct anna_node_int_literal
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    int payload;
};

struct anna_node_dummy
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    struct anna_object *payload;
};

struct anna_node_return
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    struct anna_node *payload;
    int steps;  
};

struct anna_node_float_literal
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
#ifdef ANNA_CHECK_NODE_PREPARED_ENABLED
    int prepared;
#endif    
    double payload;
};


typedef struct anna_node anna_node_t;
typedef struct anna_node_call anna_node_call_t;
typedef struct anna_node_dummy anna_node_dummy_t;
typedef struct anna_node_return anna_node_return_t;
typedef struct anna_node_member_get anna_node_member_get_t;
typedef struct anna_node_member_set anna_node_member_set_t;
typedef struct anna_node_assign anna_node_assign_t;
typedef struct anna_node_identifier anna_node_identifier_t;
typedef struct anna_node_int_literal anna_node_int_literal_t;
typedef struct anna_node_float_literal anna_node_float_literal_t;
typedef struct anna_node_string_literal anna_node_string_literal_t;
typedef struct anna_node_char_literal anna_node_char_literal_t;


extern int anna_yacc_error_count;

void anna_node_set_location(anna_node_t *node, 
			    anna_location_t *l);
anna_node_dummy_t *anna_node_dummy_create(anna_location_t *loc, 
					  struct anna_object *val, 
					  int is_trampoline);
anna_node_return_t *anna_node_return_create(anna_location_t *loc,
					    struct anna_node *val, 
					    int steps);
anna_node_member_get_t *anna_node_member_get_create(anna_location_t *loc, 
						    struct anna_node *object, 
						    size_t mid, 
						    struct anna_type *type, 
						    int wrap);
anna_node_member_set_t *anna_node_member_set_create(anna_location_t *loc, 
						    struct anna_node *object, 
						    size_t mid, 
						    struct anna_node *value, 
						    struct anna_type *type);
anna_node_int_literal_t *anna_node_int_literal_create(anna_location_t *loc, int val);
anna_node_float_literal_t *anna_node_float_literal_create(anna_location_t *loc, double val);
anna_node_char_literal_t *anna_node_char_literal_create(anna_location_t *loc, wchar_t val);
anna_node_string_literal_t *anna_node_string_literal_create(anna_location_t *loc, 
							    size_t sz, 
							    wchar_t *str);
anna_node_call_t *anna_node_call_create(anna_location_t *loc, 
					anna_node_t *function, 
					size_t argc, 
					anna_node_t **argv);
anna_node_identifier_t *anna_node_identifier_create(anna_location_t *loc, 
						    wchar_t *name);
anna_node_t *anna_node_null_create(anna_location_t *loc);
anna_node_assign_t *anna_node_assign_create(anna_location_t *loc, 
					    anna_sid_t sid, 
					    struct anna_node *value);

anna_node_call_t *anna_node_native_method_declare_create(
    anna_location_t *loc,
    ssize_t mid,
    wchar_t *name,
    int flags,
    anna_native_t func,
    anna_node_t *result,
    size_t argc,
    anna_node_t **argv,
    wchar_t **argn);

anna_node_call_t *anna_node_member_declare_create(
    anna_location_t *loc,
    ssize_t mid,
    wchar_t *name,
    int is_static,
    anna_node_t *member_type);


anna_node_t *anna_node_function_declaration_create(
    anna_location_t *loc,
    anna_node_t *result,
    size_t argc,
    anna_node_t **argv,
    wchar_t **argn);

anna_node_t *anna_node_templated_type_create(
    anna_location_t *loc,
    anna_node_t *type,
    size_t argc,
    anna_node_t **argv);

void anna_node_call_add_child(anna_node_call_t *call, anna_node_t *child);
void anna_node_call_prepend_child(anna_node_call_t *call, anna_node_t *child);
void anna_node_call_set_function(anna_node_call_t *call, anna_node_t *function);

/**
   If the specified node is a call, return it properly casted. Otherwise, exit the program with an error message.
 */
anna_node_call_t *node_cast_call(anna_node_t *node);

/**
   If the specified node is an identifier, return it properly casted. Otherwise, exit the program with an error message.
 */
anna_node_identifier_t *node_cast_identifier(anna_node_t *node);

/**
   If the specified node is an int literal, return it properly casted. Otherwise, exit the program with an error message.
 */
anna_node_int_literal_t *node_cast_int_literal(anna_node_t *node);

/**
   If the specified node is a string literal, return it properly casted. Otherwise, exit the program with an error message.
 */
anna_node_string_literal_t *node_cast_string_literal(anna_node_t *node);

/**
   Prepare the specified code for execution. This includes running
   macros, declaring variables, changing name based identifiers into
   offset identifiers, etc.
 */
anna_node_t *anna_node_prepare(anna_node_t *this, 
			       anna_function_t *function, 
			       anna_node_list_t *parent);

/**
   Invoke the specified AST tree. It must have first been prepared.
 */
anna_object_t *anna_node_invoke(anna_node_t *this, 
				anna_stack_frame_t *stack);

/**
   Check the validity of the code. This should only be run after the
   AST has been prepared, or any macros will make it cry.
 */
void anna_node_validate(anna_node_t *this, anna_stack_frame_t *stack);

/**
   Returns the return type of the specified AST node
 */
anna_type_t *anna_node_get_return_type(anna_node_t *this, anna_stack_frame_t *stack);

/**
   Prints a simple graphical representation of the specified AST
   tree. For an unprepared AST tree, the output should be readily
   rereadable by the parser, and can hence be used as a serialization
   format. This is not the case for a prepared AST tree.
 */
void anna_node_print(anna_node_t *this);

/**
  Parse the specified file and return an unprepared AST tree that
  represents the file content.
 */
anna_node_t *anna_parse(wchar_t *name);

/**
  Print the source code that lead to the creation of the specified AST
  node. This usually involves opening the source code file.

  Does a bit of fancy markup with line numbers and color coding of the
  exact source part of the node.
 */
void anna_node_print_code(anna_node_t *node);

/**
   Create an identical copy of the specified AST node.
 */
anna_node_t *anna_node_clone_shallow(anna_node_t *);

/**
   Create an identical copy of the specified AST subtree.
 */
anna_node_t *anna_node_clone_deep(anna_node_t *n);

/**
   Returns 0 if the two trees differ, 1 otherwise
 */
int anna_node_compare(anna_node_t *node1, anna_node_t *node2);

anna_function_t *anna_node_macro_get(anna_node_call_t *node, anna_stack_frame_t *stack);

anna_node_t *anna_node_replace(anna_node_t *tree, anna_node_identifier_t *from, anna_node_t *to);

#endif

