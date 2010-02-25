#ifndef DUCK_NODE_H
#define DUCK_NODE_H

#include <stdio.h>

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

struct duck_node
{
    int node_type;
    struct duck_object *wrapper;
    wchar_t *source_filename;
    size_t source_position;
};

struct duck_node_lookup
{
    int node_type;
    struct duck_object *wrapper;
    wchar_t *source_filename;
    size_t source_position;
    wchar_t *name;
};

struct duck_node_assign
{
  int node_type;
  struct duck_object *wrapper;
  wchar_t *source_filename;
  size_t source_position;
  duck_sid_t sid;
  struct duck_node *value;
};

struct duck_node_member_get
{
  int node_type;
  struct duck_object *wrapper;
  wchar_t *source_filename;
  size_t source_position;
  struct duck_node *object;
  size_t mid;
  struct duck_type *type;
};

struct duck_node_call
{
    int node_type;
    struct duck_object *wrapper;
    wchar_t *source_filename;
    size_t source_position;
    struct duck_node *function;
    size_t child_count;
    size_t child_capacity;
    struct duck_node **child;
};

struct duck_node_string_literal
{
    int node_type;
    struct duck_object *wrapper;
    wchar_t *source_filename;
    size_t source_position;
    size_t payload_size;
    wchar_t *payload;
};

struct duck_node_char_literal
{
    int node_type;
    struct duck_object *wrapper;
    wchar_t *source_filename;
    size_t source_position;
    wchar_t payload;
};

struct duck_node_int_literal
{
    int node_type;
    struct duck_object *wrapper;
    wchar_t *source_filename;
    size_t source_position;
    int payload;
};

struct duck_node_dummy
{
    int node_type;
    struct duck_object *wrapper;
    wchar_t *source_filename;
    size_t source_position;
    struct duck_object *payload;
};

struct duck_node_float_literal
{
    int node_type;
    struct duck_object *wrapper;
    wchar_t *source_filename;
    size_t source_position;
    double payload;
};

typedef struct duck_node duck_node_t;
typedef struct duck_node_call duck_node_call_t;
typedef struct duck_node_dummy duck_node_dummy_t;
typedef struct duck_node_member_get duck_node_member_get_t;
typedef struct duck_node_assign duck_node_assign_t;
typedef struct duck_node_lookup duck_node_lookup_t;
typedef struct duck_node_int_literal duck_node_int_literal_t;
typedef struct duck_node_float_literal duck_node_float_literal_t;
typedef struct duck_node_string_literal duck_node_string_literal_t;
typedef struct duck_node_char_literal duck_node_char_literal_t;

extern duck_node_t *duck_parse_tree;

duck_node_dummy_t *duck_node_dummy_create(wchar_t *src, size_t src_pos, struct duck_object *val, int is_trampoline);
duck_node_member_get_t *duck_node_member_get_create(wchar_t *src, size_t src_pos, struct duck_node *object, size_t mid, struct duck_type *type, int wrap);
duck_node_int_literal_t *duck_node_int_literal_create(wchar_t *src, size_t src_pos, int val);
duck_node_float_literal_t *duck_node_float_literal_create(wchar_t *src, size_t src_pos, double val);
duck_node_char_literal_t *duck_node_char_literal_create(wchar_t *src, size_t src_pos, wchar_t val);
duck_node_string_literal_t *duck_node_string_literal_create(wchar_t *src, size_t src_pos, size_t sz, wchar_t *str);
duck_node_call_t *duck_node_call_create(wchar_t *src, size_t src_pos, duck_node_t *function, size_t argc, duck_node_t **argv);
duck_node_lookup_t *duck_node_lookup_create(wchar_t *src, size_t src_pos, wchar_t *name);
duck_node_t *duck_node_null_create(wchar_t *src, size_t src_pos);
duck_node_assign_t *duck_node_assign_create(wchar_t *src, size_t src_pos, duck_sid_t sid, struct duck_node *value);
void duck_node_call_add_child(duck_node_call_t *call, duck_node_t *child);
void duck_node_call_set_function(duck_node_call_t *call, duck_node_t *function);

extern wchar_t *duck_current_filename;
extern size_t duck_current_pos;

duck_node_t *duck_parse(FILE *, wchar_t *);

extern int duck_yacc_error;

#endif
