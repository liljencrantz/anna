#ifndef ANNA_STACK_H
#define ANNA_STACK_H

#include "anna/util.h"
#include "anna/base.h"

struct anna_stack_template;
struct anna_node_declare;
struct anna_node;
struct anna_node_call;
struct anna_type;
struct anna_use;

struct anna_sid
{
  ssize_t frame;
  ssize_t offset;
};

typedef struct anna_sid anna_sid_t;

#define ANNA_STACK_NAMESPACE 1024
#define ANNA_STACK_LOADED 2048
#define ANNA_STACK_DECLARE 4096

/* Stack member flags */
#define ANNA_STACK_READONLY 1

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
       Store flags for this stack. 
    */
    int flags;
    struct anna_stack_template *parent;
    size_t count;
    size_t capacity;
    hash_table_t member_string_identifier;
    int stop;
    struct anna_node_call *definition;
    
    anna_function_t *function;
    /**
       An Anna object representing this entire stack frame
    */
    struct anna_object *wrapper;
    /**
       The AST node (if any) used to declare each declaration in this
       stack frame.
     */
    struct anna_node_declare **member_declare_node;  
    /**
       One integer representing the flags for each declaration in this
       stack frame . Currently, the only available flag is
       ANNA_STACK_READONLY, which is used for constants.
     */
    int *member_flags;
    /**
       List of all modules to use for implicit namespace lookups
     */
    array_list_t import;
    /**
       List of all modules to use for macro expansion
     */
    array_list_t expand;
    /**
       Only used by modules. The name of the module.
     */
    wchar_t *name;
    /**
       Only used by modules. Full name of module location.
    */
    wchar_t *filename;
};

typedef struct anna_stack_template anna_stack_template_t;

anna_object_t *anna_stack_wrap(anna_stack_template_t *stack);
anna_stack_template_t *anna_stack_unwrap(anna_object_t *stack);

anna_stack_template_t *anna_stack_create(anna_stack_template_t *parent);
void anna_stack_name(anna_stack_template_t *stack, wchar_t *name);

void anna_stack_declare(
    anna_stack_template_t *stack,
    wchar_t *name,
    anna_type_t *type,
    anna_entry_t *initial_value,
    int flags);

void anna_stack_declare2(
    anna_stack_template_t *stack,
    struct anna_node_declare *declare_node);

anna_entry_t **anna_stack_addr_get(anna_stack_template_t *stack, wchar_t *name);

void anna_stack_set(anna_stack_template_t *stack, wchar_t *name, anna_entry_t *value);

anna_entry_t *anna_stack_get(anna_stack_template_t *stack, wchar_t *name);

anna_entry_t *anna_stack_macro_get(anna_stack_template_t *stack, wchar_t *name);

anna_entry_t *anna_stack_template_get(anna_stack_template_t *stack, wchar_t *name);

anna_entry_t *anna_stack_get_const(anna_stack_template_t *stack, wchar_t *name);

anna_type_t *anna_stack_get_type(anna_stack_template_t *stack, wchar_t *name);

void anna_stack_set_type(anna_stack_template_t *stack, wchar_t *name, anna_type_t *type);
struct anna_node_declare *anna_stack_get_declaration(
    anna_stack_template_t *stack, wchar_t *name);

anna_sid_t anna_stack_sid_create(anna_stack_template_t *stack, wchar_t *name);

void anna_stack_print(anna_stack_template_t *stack);

int anna_stack_depth(anna_stack_template_t *stack);

void anna_stack_print_trace(anna_stack_template_t *stack);

void anna_stack_prepare(anna_type_t *type);

int anna_stack_get_flag(anna_stack_template_t *stack, wchar_t *name);
void anna_stack_set_flag(anna_stack_template_t *stack, wchar_t *name, int value);

anna_stack_template_t *anna_stack_template_search(
    anna_stack_template_t *stack,
    wchar_t *name);

struct anna_use *anna_stack_search_use(
    anna_stack_template_t *stack,
    wchar_t *name);

int anna_stack_check(anna_stack_template_t *stack, int i);
anna_entry_t *anna_stack_get_try(anna_stack_template_t *stack, wchar_t *name);

void anna_stack_document(anna_stack_template_t *stack, wchar_t *documentation);

#endif
