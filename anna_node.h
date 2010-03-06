#ifndef ANNA_NODE_H
#define ANNA_NODE_H

#include <stdio.h>

#include "anna.h"
#include "anna_stack.h"

#define ANNA_NODE_CALL 0
#define ANNA_NODE_LOOKUP 1
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

struct anna_node
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
};

struct anna_node_lookup
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    wchar_t *name;
    anna_sid_t sid;
};

struct anna_node_assign
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    anna_sid_t sid;
    struct anna_node *value;
};

struct anna_node_member_get
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    struct anna_node *object;
    size_t mid;
    struct anna_type *type;
};

struct anna_node_member_set
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
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
    size_t payload_size;
    wchar_t *payload;
};

struct anna_node_char_literal
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    wchar_t payload;
};

struct anna_node_int_literal
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    int payload;
};

struct anna_node_dummy
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    struct anna_object *payload;
};

struct anna_node_return
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    struct anna_node *payload;
    int steps;  
};

struct anna_node_float_literal
{
    int node_type;
    struct anna_object *wrapper;
    anna_location_t location;
    double payload;
};

typedef struct anna_node anna_node_t;
typedef struct anna_node_call anna_node_call_t;
typedef struct anna_node_dummy anna_node_dummy_t;
typedef struct anna_node_return anna_node_return_t;
typedef struct anna_node_member_get anna_node_member_get_t;
typedef struct anna_node_member_set anna_node_member_set_t;
typedef struct anna_node_assign anna_node_assign_t;
typedef struct anna_node_lookup anna_node_lookup_t;
typedef struct anna_node_int_literal anna_node_int_literal_t;
typedef struct anna_node_float_literal anna_node_float_literal_t;
typedef struct anna_node_string_literal anna_node_string_literal_t;
typedef struct anna_node_char_literal anna_node_char_literal_t;

extern int anna_yacc_error_count;

void anna_node_set_location(anna_node_t *node, anna_location_t *l);
anna_node_dummy_t *anna_node_dummy_create(anna_location_t *loc, struct anna_object *val, int is_trampoline);
anna_node_return_t *anna_node_return_create(anna_location_t *loc, struct anna_node *val, int steps);
anna_node_member_get_t *anna_node_member_get_create(anna_location_t *loc, struct anna_node *object, size_t mid, struct anna_type *type, int wrap);
anna_node_member_set_t *anna_node_member_set_create(anna_location_t *loc, struct anna_node *object, size_t mid, struct anna_node *value, struct anna_type *type);
anna_node_int_literal_t *anna_node_int_literal_create(anna_location_t *loc, int val);
anna_node_float_literal_t *anna_node_float_literal_create(anna_location_t *loc, double val);
anna_node_char_literal_t *anna_node_char_literal_create(anna_location_t *loc, wchar_t val);
anna_node_string_literal_t *anna_node_string_literal_create(anna_location_t *loc, size_t sz, wchar_t *str);
anna_node_call_t *anna_node_call_create(anna_location_t *loc, anna_node_t *function, size_t argc, anna_node_t **argv);
anna_node_lookup_t *anna_node_lookup_create(anna_location_t *loc, wchar_t *name);
anna_node_t *anna_node_null_create(anna_location_t *loc);
anna_node_assign_t *anna_node_assign_create(anna_location_t *loc, anna_sid_t sid, struct anna_node *value);
void anna_node_call_add_child(anna_node_call_t *call, anna_node_t *child);
void anna_node_call_prepend_child(anna_node_call_t *call, anna_node_t *child);
void anna_node_call_set_function(anna_node_call_t *call, anna_node_t *function);

anna_node_call_t *node_cast_call(anna_node_t *node);
anna_node_lookup_t *node_cast_lookup(anna_node_t *node);
anna_node_int_literal_t *node_cast_int_literal(anna_node_t *node);
anna_node_string_literal_t *node_cast_string_literal(anna_node_t *node);

void anna_node_print(anna_node_t *this);

/*
  This functions all treverse the AST, and taking clever actions on each node
*/

/**
   Prepare the specified code for execution. This includes running macros, declaring variables, changing name based lookups into offset lookups, etc.
 */
anna_node_t *anna_node_prepare(anna_node_t *this, anna_function_t *function, anna_node_list_t *parent);
anna_object_t *anna_node_invoke(anna_node_t *this, anna_stack_frame_t *stack);

/**
   Check the validity of the code. This should only be run after the
   AST has been prepared, or any macros will make it cry.
 */
void anna_node_validate(anna_node_t *this, anna_stack_frame_t *stack);
/**
   Returns the return type of the specified AST node
 */
anna_type_t *anna_node_get_return_type(anna_node_t *this, anna_stack_frame_t *stack);

void anna_node_print(anna_node_t *this);

anna_node_t *anna_parse(wchar_t *name);
void anna_node_print_code(anna_node_t *node);

#endif

