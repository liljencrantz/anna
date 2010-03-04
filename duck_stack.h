#ifndef DUCK_STACK_H
#define DUCK_STACK_H

#include "util.h"
#include "duck.h"

struct duck_stack_frame;

struct duck_sid
{
  size_t frame;
  size_t offset;
};

typedef struct duck_sid duck_sid_t;

struct duck_stack_frame
{
    struct duck_stack_frame *parent;
    size_t count;
    size_t capacity;
    hash_table_t member_string_lookup;
    int stop;
    struct duck_type **member_type;  
    struct duck_object *member[];
};

typedef struct duck_stack_frame duck_stack_frame_t;

duck_stack_frame_t *duck_stack_create(size_t sz, duck_stack_frame_t *parent);
void duck_stack_declare(duck_stack_frame_t *stack,
			wchar_t *name,
			duck_type_t *type,
			duck_object_t *initial_value);
duck_object_t **duck_stack_member_addr_get_str(duck_stack_frame_t *stack, wchar_t *name);
void duck_stack_set_str(duck_stack_frame_t *stack, wchar_t *name, struct duck_object *value);
duck_object_t *duck_stack_get_str(duck_stack_frame_t *stack, wchar_t *name);
duck_object_t *duck_stack_get_sid(duck_stack_frame_t *stack, duck_sid_t sid);
void duck_stack_set_sid(duck_stack_frame_t *stack, duck_sid_t sid, duck_object_t *value);
duck_type_t *duck_stack_get_type(duck_stack_frame_t *stack, wchar_t *name);
duck_sid_t duck_stack_sid_create(duck_stack_frame_t *stack, wchar_t *name);
duck_stack_frame_t *duck_stack_clone(duck_stack_frame_t *template);
void duck_stack_print(duck_stack_frame_t *stack);

#endif
