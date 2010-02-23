#ifndef DUCK_H
#define DUCK_H

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

typedef struct duck_object *(*duck_function_ptr_t)( struct duck_node_call *, 
						    struct duck_stack_frame *);

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
    struct duck_node_call *body;
    duck_function_ptr_t native;
    struct duck_type *return_type;
    struct duck_object *wrapper;
    size_t input_count;
    wchar_t **input_name;
    int is_macro;
    struct duck_type_t *input_type[];
};

typedef struct duck_type duck_type_t;
typedef struct duck_member duck_member_t;
typedef struct duck_object duck_object_t;
typedef struct duck_frame duck_frame_t;
typedef struct duck_function duck_function_t;

#endif
