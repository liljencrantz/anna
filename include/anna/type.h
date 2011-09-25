#ifndef ANNA_TYPE_H
#define ANNA_TYPE_H

#include "anna.h"
#include "anna/node.h"
#include "anna/stack.h"

#define ANNA_TYPE_REGISTERED 512
#define ANNA_TYPE_PREPARED_INTERFACE 1024
#define ANNA_TYPE_PREPARED_IMPLEMENTATION 2048
#define ANNA_TYPE_COMPILED 4096
#define ANNA_TYPE_SPECIALIZED 8192
#define ANNA_TYPE_MEMBER_DECLARATION_IN_PROGRESS (8192*2)

/**
  Return the anna_type_t contained in the specified anna_type_t.wrapper
 */
anna_type_t *anna_type_unwrap(
    anna_object_t *wrapper);

anna_type_t *anna_type_create(wchar_t *name, anna_node_call_t *definition);

anna_node_call_t *anna_type_attribute_list_get(anna_type_t *type);

anna_node_call_t *anna_type_definition_get(anna_type_t *type);

anna_type_t *anna_type_native_create(wchar_t *name, anna_stack_template_t *stack);
anna_type_t *anna_type_stack_create(wchar_t *name, anna_stack_template_t *stack);

void anna_type_copy(anna_type_t *dst, anna_type_t *src);

void anna_type_get_member_names(anna_type_t *type, wchar_t **dest);

void anna_type_print(anna_type_t *type);

anna_member_t *anna_type_member_info_get(anna_type_t *type, wchar_t *name);

size_t anna_type_member_count(anna_type_t *type);

anna_type_t *anna_type_unwrap(anna_object_t *wrapper);

anna_object_t *anna_type_wrap(anna_type_t *result);

int anna_type_prepared(anna_type_t *result);

void anna_type_definition_make(anna_type_t *type);

size_t anna_type_static_member_allocate(anna_type_t *type);

/**
  Returns the type of the specified member in the specified type
 */
anna_type_t *anna_type_member_type_get(anna_type_t *type, wchar_t *name);
/**
   Returns true if the member with the specified name is a method
 */
int anna_type_member_is_method(anna_type_t *type, wchar_t *name);
/**
   Reallocate the mid table to the specified new size
 */
void anna_type_reallocade_mid_lookup(size_t old_sz, size_t sz);

/**
   
 */
void anna_type_set_stack(anna_type_t *type, anna_stack_template_t *parent);
void anna_type_setup_interface(anna_type_t *type);

void anna_type_prepare_member(anna_type_t *type, mid_t mid, anna_stack_template_t *stack);

/**
   Specialize the specified type with the specified temaplte specialization arguments
 */
anna_type_t *anna_type_specialize(anna_type_t *type, anna_node_call_t *spec);

/**
   Check if the specified type is an unspecialized template, and if
   so, use the specified constructor arguments to locate the most
   suitable template specialization
 */
anna_type_t *anna_type_implicit_specialize(anna_type_t *type, anna_node_call_t *call);

void anna_type_object_is_created(void);

void anna_type_copy_object(anna_type_t *type);

void anna_type_macro_expand(anna_type_t *f, anna_stack_template_t *stack);

void anna_type_calculate_size(anna_type_t *this);

void anna_type_init(void);

/**
   Returns a type representing a function with the specified signature.
*/
anna_type_t *anna_type_for_function(
    anna_type_t *result, size_t argc,
    anna_type_t **argv, wchar_t **argn, anna_node_t **argd,
    int is_variadic);

void anna_type_mark_static(void);

void anna_type_document(anna_type_t *type, wchar_t *documentation);

int anna_type_mid_internal(mid_t mid);

void anna_type_finalizer_add(anna_type_t *type, anna_finalizer_t finalizer);


#endif
