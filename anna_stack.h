#ifndef ANNA_STACK_H
#define ANNA_STACK_H

#include "util.h"
#include "anna.h"

struct anna_stack_template;
struct anna_node_declare;

struct anna_sid
{
  ssize_t frame;
  ssize_t offset;
};

typedef struct anna_sid anna_sid_t;



#define ANNA_STACK_NAMESPACE 1024
#define ANNA_STACK_UNLOADED 2048

/* Stack member flags */
#define ANNA_STACK_READONLY 1

#define anna_stack_get_ro(stack, name) !!(anna_stack_get_flag(stack, name) & ANNA_STACK_READONLY)

/**
   A stack frame template. Used during compilation to represent the
   stack frame for a function definition or the members of a
   namespace. The VM has a packed structure for function invocations,
   and the stack templates used to represent namespaces are converted
   into objects, so these objects become useless after the compilation
   phase.
*/
struct anna_stack_template
{
    /**
       Store flags for this stack. Currently only used by the GC to
       mark this allocation as a stack.
     */
    int flags;
    struct anna_stack_template *parent;
    size_t count;
    size_t capacity;
    hash_table_t member_string_identifier;
    int stop;
    anna_function_t *function;
    /**
       An object representing this stack frame
    */
    struct anna_object *wrapper;
    struct anna_type **member_type;  
    struct anna_node_declare **member_declare_node;  
    int *member_flags;
    array_list_t import;
    array_list_t expand;
    struct anna_object **member;
    /**
       Only used by modules. Full name of module location.
     */
    wchar_t *filename;
};

typedef struct anna_stack_template anna_stack_template_t;

anna_object_t *anna_stack_wrap(anna_stack_template_t *stack);
anna_stack_template_t *anna_stack_unwrap(anna_object_t *stack);

anna_stack_template_t *anna_stack_create(anna_stack_template_t *parent);

void anna_stack_declare(
    anna_stack_template_t *stack,
    wchar_t *name,
    anna_type_t *type,
    anna_object_t *initial_value,
    int flags);

void anna_stack_declare2(
    anna_stack_template_t *stack,
    struct anna_node_declare *declare_node);

anna_object_t **anna_stack_addr_get(anna_stack_template_t *stack, wchar_t *name);

void anna_stack_set(anna_stack_template_t *stack, wchar_t *name, struct anna_object *value);

anna_object_t *anna_stack_get(anna_stack_template_t *stack, wchar_t *name);

anna_object_t *anna_stack_macro_get(anna_stack_template_t *stack, wchar_t *name);

anna_object_t *anna_stack_template_get(anna_stack_template_t *stack, wchar_t *name);

anna_object_t *anna_stack_get_const(anna_stack_template_t *stack, wchar_t *name);

anna_type_t *anna_stack_get_type(anna_stack_template_t *stack, wchar_t *name);

void anna_stack_set_type(anna_stack_template_t *stack, wchar_t *name, anna_type_t *type);
struct anna_node_declare *anna_stack_get_declaration(
    anna_stack_template_t *stack, wchar_t *name);

anna_sid_t anna_stack_sid_create(anna_stack_template_t *stack, wchar_t *name);

void anna_stack_print(anna_stack_template_t *stack);

int anna_stack_depth(anna_stack_template_t *stack);

void anna_stack_print_trace(anna_stack_template_t *stack);

void anna_stack_populate_wrapper(anna_stack_template_t *stack);

void anna_stack_prepare(anna_type_t *type);

int anna_stack_get_flag(anna_stack_template_t *stack, wchar_t *name);
void anna_stack_set_flag(anna_stack_template_t *stack, wchar_t *name, int value);

anna_stack_template_t *anna_stack_template_search(
    anna_stack_template_t *stack,
    wchar_t *name);

#endif
