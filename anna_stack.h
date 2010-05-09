#ifndef ANNA_STACK_H
#define ANNA_STACK_H

#include "util.h"
#include "anna.h"

struct anna_stack_frame;

struct anna_sid
{
  ssize_t frame;
  ssize_t offset;
};

typedef struct anna_sid anna_sid_t;

#define ANNA_STACK_FROZEN 1

struct anna_stack_frame
{
    struct anna_stack_frame *parent;
    size_t count;
    size_t capacity;
    hash_table_t member_string_identifier;
    int stop;
    anna_function_t *function;
#ifdef ANNA_CHECK_STACK_ENABLED
    int flags;
#endif
    struct anna_object *wrapper;
    struct anna_type **member_type;  
    struct anna_object *member[];
};

typedef struct anna_stack_frame anna_stack_frame_t;

anna_object_t *anna_stack_wrap(anna_stack_frame_t *stack);
anna_stack_frame_t *anna_stack_unwrap(anna_object_t *stack);

anna_stack_frame_t *anna_stack_create(size_t sz, anna_stack_frame_t *parent);

void anna_stack_declare(anna_stack_frame_t *stack,
			wchar_t *name,
			anna_type_t *type,
			anna_object_t *initial_value);

anna_object_t **anna_stack_addr_get_str(anna_stack_frame_t *stack, wchar_t *name);

void anna_stack_set_str(anna_stack_frame_t *stack, wchar_t *name, struct anna_object *value);

anna_object_t *anna_stack_get_str(anna_stack_frame_t *stack, wchar_t *name);

anna_object_t *anna_stack_get_sid(anna_stack_frame_t *stack, anna_sid_t sid);

void anna_stack_set_sid(anna_stack_frame_t *stack, anna_sid_t sid, anna_object_t *value);

anna_type_t *anna_stack_get_type(anna_stack_frame_t *stack, wchar_t *name);

anna_sid_t anna_stack_sid_create(anna_stack_frame_t *stack, wchar_t *name);

anna_stack_frame_t *anna_stack_clone(anna_stack_frame_t *template);

void anna_stack_print(anna_stack_frame_t *stack);

int anna_stack_depth(anna_stack_frame_t *stack);

void anna_stack_print_trace(anna_stack_frame_t *stack);

void anna_stack_prepare(anna_type_t *type);

#endif
