#ifndef DUCK_NODE_H
#define DUCK_NODE_H

#include <stdio.h>

#include "duck.h"
#include "duck_stack.h"

#define DUCK_NODE_CALL 0
#define DUCK_NODE_LOOKUP 1
#define DUCK_NODE_INT_LITERAL 2
#define DUCK_NODE_STRING_LITERAL 3
#define DUCK_NODE_CHAR_LITERAL 4
#define DUCK_NODE_FLOAT_LITERAL 5
#define DUCK_NODE_NULL 6
#define DUCK_NODE_DUMMY 7
#define DUCK_NODE_TRAMPOLINE 8
#define DUCK_NODE_ASSIGN 9
#define DUCK_NODE_MEMBER_GET 10
#define DUCK_NODE_MEMBER_GET_WRAP 11
#define DUCK_NODE_MEMBER_SET 12
#define DUCK_NODE_CONSTRUCT 13

struct YYLTYPE
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
    wchar_t *filename;
};

typedef struct YYLTYPE duck_location_t;
typedef struct YYLTYPE YYLTYPE;
#define yyltype YYLTYPE
#define YYLTYPE YYLTYPE

struct duck_node
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
};

struct duck_node_lookup
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
    wchar_t *name;
    duck_sid_t sid;
};

struct duck_node_assign
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
    duck_sid_t sid;
    struct duck_node *value;
};

struct duck_node_member_get
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
    struct duck_node *object;
    size_t mid;
    struct duck_type *type;
};

struct duck_node_member_set
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
    struct duck_node *object;
    struct duck_node *value;
    size_t mid;
    struct duck_type *type;
};


struct duck_node_call
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
    struct duck_node *function;
    size_t child_count;
    size_t child_capacity;
    struct duck_node **child;
};

struct duck_node_string_literal
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
    size_t payload_size;
    wchar_t *payload;
};

struct duck_node_char_literal
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
    wchar_t payload;
};

struct duck_node_int_literal
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
    int payload;
};

struct duck_node_dummy
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
    struct duck_object *payload;
};

struct duck_node_float_literal
{
    int node_type;
    struct duck_object *wrapper;
    duck_location_t location;
    double payload;
};

typedef struct duck_node duck_node_t;
typedef struct duck_node_call duck_node_call_t;
typedef struct duck_node_dummy duck_node_dummy_t;
typedef struct duck_node_member_get duck_node_member_get_t;
typedef struct duck_node_member_set duck_node_member_set_t;
typedef struct duck_node_assign duck_node_assign_t;
typedef struct duck_node_lookup duck_node_lookup_t;
typedef struct duck_node_int_literal duck_node_int_literal_t;
typedef struct duck_node_float_literal duck_node_float_literal_t;
typedef struct duck_node_string_literal duck_node_string_literal_t;
typedef struct duck_node_char_literal duck_node_char_literal_t;

extern int duck_yacc_error_count;

void duck_node_set_location(duck_node_t *node, duck_location_t *l);
duck_node_dummy_t *duck_node_dummy_create(duck_location_t *loc, struct duck_object *val, int is_trampoline);
duck_node_member_get_t *duck_node_member_get_create(duck_location_t *loc, struct duck_node *object, size_t mid, struct duck_type *type, int wrap);
duck_node_member_set_t *duck_node_member_set_create(duck_location_t *loc, struct duck_node *object, size_t mid, struct duck_node *value, struct duck_type *type);
duck_node_int_literal_t *duck_node_int_literal_create(duck_location_t *loc, int val);
duck_node_float_literal_t *duck_node_float_literal_create(duck_location_t *loc, double val);
duck_node_char_literal_t *duck_node_char_literal_create(duck_location_t *loc, wchar_t val);
duck_node_string_literal_t *duck_node_string_literal_create(duck_location_t *loc, size_t sz, wchar_t *str);
duck_node_call_t *duck_node_call_create(duck_location_t *loc, duck_node_t *function, size_t argc, duck_node_t **argv);
duck_node_lookup_t *duck_node_lookup_create(duck_location_t *loc, wchar_t *name);
duck_node_t *duck_node_null_create(duck_location_t *loc);
duck_node_assign_t *duck_node_assign_create(duck_location_t *loc, duck_sid_t sid, struct duck_node *value);
void duck_node_call_add_child(duck_node_call_t *call, duck_node_t *child);
void duck_node_call_prepend_child(duck_node_call_t *call, duck_node_t *child);
void duck_node_call_set_function(duck_node_call_t *call, duck_node_t *function);

duck_node_call_t *node_cast_call(duck_node_t *node);
duck_node_lookup_t *node_cast_lookup(duck_node_t *node);
duck_node_int_literal_t *node_cast_int_literal(duck_node_t *node);
duck_node_string_literal_t *node_cast_string_literal(duck_node_t *node);

void duck_node_print(duck_node_t *this);

/*
  This functions all treverse the AST, and taking clever actions on each node
 */

/**
   Prepare the specified code for execution. This includes running macros, declaring variables, changing name based lookups into offset lookups, etc.
 */
duck_node_t *duck_node_prepare(duck_node_t *this, duck_function_t *function, duck_node_list_t *parent);
duck_object_t *duck_node_invoke(duck_node_t *this, duck_stack_frame_t *stack);

/**
   Check the validity of the code. This should only be run after the
   AST has been prepared, or any macros will make it cry.
 */
void duck_node_validate(duck_node_t *this, duck_stack_frame_t *stack);
/**
   Returns the return type of the specified AST node
 */
duck_type_t *duck_node_get_return_type(duck_node_t *this, duck_stack_frame_t *stack);

void duck_node_print(duck_node_t *this);

duck_node_t *duck_parse(wchar_t *name);
void duck_node_print_code(duck_node_t *node);

#endif

