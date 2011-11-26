#ifndef ANNA_NODE_H
#define ANNA_NODE_H

#include <stdio.h>
#include <gmp.h>

#include "anna/base.h"
#include "anna/stack.h"

enum anna_node_enum
{
/* These are the node types visible to macros */
    ANNA_NODE_CALL,
    ANNA_NODE_IDENTIFIER,
    ANNA_NODE_INT_LITERAL,
    ANNA_NODE_STRING_LITERAL,
    ANNA_NODE_CHAR_LITERAL,
    ANNA_NODE_FLOAT_LITERAL,
    ANNA_NODE_NULL,

/* These are internal AST node types that are created by one of the
 * later AST transformation passes. */
    ANNA_NODE_DUMMY,
    ANNA_NODE_CLOSURE,
    ANNA_NODE_ASSIGN,
    ANNA_NODE_MEMBER_GET,
    ANNA_NODE_MEMBER_BIND,
    ANNA_NODE_MEMBER_SET,
    ANNA_NODE_CONSTRUCT,
    ANNA_NODE_DECLARE,
    ANNA_NODE_CONST,
    ANNA_NODE_OR,
    ANNA_NODE_AND,
    ANNA_NODE_WHILE,
    ANNA_NODE_MEMBER_CALL,
    ANNA_NODE_IF,
    ANNA_NODE_SPECIALIZE,
    ANNA_NODE_TYPE_OF,
    ANNA_NODE_RETURN_TYPE_OF,
    ANNA_NODE_INPUT_TYPE_OF,
    ANNA_NODE_TYPE,
    ANNA_NODE_CAST,
    ANNA_NODE_MAPPING,
    ANNA_NODE_INTERNAL_IDENTIFIER,
    ANNA_NODE_RETURN,
    ANNA_NODE_BREAK,
    ANNA_NODE_CONTINUE,
    ANNA_NODE_USE,
    ANNA_NODE_STATIC_MEMBER_GET,
    ANNA_NODE_STATIC_MEMBER_SET,
    ANNA_NODE_STATIC_MEMBER_CALL,

    /* This needs to be the last element of the enum! */
    ANNA_NODE_TYPE_COUNT
};

#define ANNA_NODE_DONT_EXPAND 512
#define ANNA_NODE_MERGE 1024

#define ANNA_NODE_TYPE_IN_TRANSIT ((anna_type_t *)1)

#define ANNA_NODE_ACCESS_STATIC_MEMBER 1

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

struct anna_node
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;
};


struct anna_node_identifier
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    wchar_t *name;
    anna_sid_t sid;
};

struct anna_node_cond
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    struct anna_node *arg1;
    struct anna_node *arg2;
};

struct anna_node_if
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    struct anna_node *cond;
    struct anna_node_call *block1;
    struct anna_node_call *block2;
    int has_else;
};

struct anna_node_assign
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    wchar_t *name;
    anna_sid_t sid;
    struct anna_node *value;
};

struct anna_node_declare
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    wchar_t *name;
    anna_sid_t sid;
    struct anna_node *value;    
    struct anna_node *type;
    struct anna_node_call *attribute;
};

struct anna_node_member_access
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;
    
    mid_t mid;
    struct anna_node *object;
    struct anna_node *value;
    int access_type;
};

struct anna_node_call
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    mid_t mid;
    union
    {
	struct anna_node *object;
	struct anna_node *function;
    };
    size_t child_count;
    size_t child_capacity;
    struct anna_node **child;
    int access_type;
};

struct anna_node_string_literal
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    size_t payload_size;
    wchar_t *payload;
    int free;
};

struct anna_node_char_literal
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    wchar_t payload;
};

struct anna_node_int_literal
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    mpz_t payload;
};

struct anna_node_dummy
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    struct anna_object *payload;
};

struct anna_node_closure
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    struct anna_function *payload;
};

struct anna_node_type
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    struct anna_type *payload;
};

struct anna_node_float_literal
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;

    double payload;
};

struct anna_node_wrapper
{
    int flags;
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_type_t *return_type;
    anna_stack_template_t *stack;    
    struct anna_node *transformed;
    
    struct anna_node *payload;
    int steps;
};

typedef struct anna_node anna_node_t;
typedef struct anna_node_call anna_node_call_t;
typedef struct anna_node_dummy anna_node_dummy_t;
typedef struct anna_node_closure anna_node_closure_t;
typedef struct anna_node_type anna_node_type_t;
typedef struct anna_node_member_access anna_node_member_access_t;
typedef struct anna_node_assign anna_node_assign_t;
typedef struct anna_node_identifier anna_node_identifier_t;
typedef struct anna_node_int_literal anna_node_int_literal_t;
typedef struct anna_node_float_literal anna_node_float_literal_t;
typedef struct anna_node_string_literal anna_node_string_literal_t;
typedef struct anna_node_char_literal anna_node_char_literal_t;
typedef struct anna_node_declare anna_node_declare_t;
typedef struct anna_node_cond anna_node_cond_t;
typedef struct anna_node_if anna_node_if_t;
typedef struct anna_node_wrapper anna_node_wrapper_t;

extern int anna_yacc_error_count;

void anna_node_set_location(
    anna_node_t *node, 
    anna_location_t *l);

void anna_node_call_add_child(anna_node_call_t *call, anna_node_t *child);
void anna_node_call_prepend_child(anna_node_call_t *call, anna_node_t *child);
void anna_node_call_set_function(anna_node_call_t *call, anna_node_t *function);

/**
   If the specified node is a call, return it properly casted. Otherwise, exit the program with an error message.
 */
anna_node_call_t *node_cast_call(anna_node_t *node);
/**
   If the specified node is a mapping node, return it properly casted. Otherwise, exit the program with an error message.
 */
anna_node_cond_t *node_cast_mapping(anna_node_t *node);

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
   Node preparation phase 1: Expand all macros
 */
anna_node_t *anna_node_macro_expand(
    anna_node_t *this,
    anna_stack_template_t *stack);

/**
   Node preparation phase 2: Merge in all nodes marked as such into previous call
 */
anna_node_t *anna_node_merge(
    anna_node_t *this);

/**
   Node preparation phase 3: Register all variables
 */

void anna_node_register_declarations(
    anna_node_t *this,
    anna_stack_template_t *stack);

void anna_node_set_stack(
    anna_node_t *this,
    anna_stack_template_t *stack);

void anna_node_resolve_identifiers(
    anna_node_t *this);


/**
   Node preparation phase 3: Calculate all variable types
 */
anna_node_t *anna_node_calculate_type(
    anna_node_t *this);

/**
   Check the validity of the code. This should only be run after the
   AST has been prepared.
*/
void anna_node_validate(anna_node_t *this, anna_stack_template_t *stack);

/**
   Prints a simple textual representation of the specified AST
   tree. For an unprepared AST tree, the output should be readily
   rereadable by the parser, and can hence be used as a serialization
   format. This is not the case for a prepared AST tree.
*/
void anna_node_print(int level, anna_node_t *this);

/**
   Returns a simple textual representation of the specified AST
   tree. For an unprepared AST tree, the output should be readily
   rereadable by the parser, and can hence be used as a serialization
   format. This is not the case for a prepared AST tree.
*/
wchar_t *anna_node_string(anna_node_t *this);

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

anna_node_t *anna_node_replace(
    anna_node_t *tree, anna_node_identifier_t *from, anna_node_t *to);

typedef void(*anna_node_function_t)(anna_node_t *, void *);
typedef anna_node_t *(*anna_node_replace_function_t)(anna_node_t *, void *);

void anna_node_each(
    anna_node_t *tree, anna_node_function_t fun, void *aux);

anna_node_t *anna_node_each_replace(
    anna_node_t *tree, anna_node_replace_function_t fun, void *aux);

void anna_node_find(
    anna_node_t *tree, int node_type, array_list_t *al);

/**
   Check if the node is a call node with an identifier with the
   specified name as its function.
*/
int anna_node_is_call_to(
    anna_node_t *this, 
    wchar_t *name);

/**
   Check if the node is an identifier with the specified name.
*/
int anna_node_is_named(
    anna_node_t *this, 
    wchar_t *name);

void anna_yacc_init(void);

anna_entry_t *anna_node_static_invoke_try(
    anna_node_t *this, 
    anna_stack_template_t *stack);

anna_entry_t *anna_node_static_invoke(
    anna_node_t *this, 
    anna_stack_template_t *stack);

void anna_node_calculate_type_children(
    anna_node_call_t *node);

int anna_node_validate_call_parameters(
    anna_node_call_t *call, 
    anna_function_type_t *target, 
    int is_method, 
    int print_error);

void anna_node_call_map(
    anna_node_call_t *call, 
    anna_function_type_t *target, 
    int is_method);

anna_type_t *anna_node_resolve_to_type(
    anna_node_t *node, anna_stack_template_t *stack);

anna_node_t *anna_node_type_lookup_get_payload(anna_node_t *node);

#endif

