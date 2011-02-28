#ifndef ANNA_TYPE_H
#define ANNA_TYPE_H

#include "anna.h"
#include "anna_node.h"
#include "anna_stack.h"

extern array_list_t  anna_type_list;

anna_type_t *anna_type_create(wchar_t *name, anna_node_call_t *definition);

anna_node_call_t *anna_type_attribute_list_get(anna_type_t *type);

anna_node_call_t *anna_type_definition_get(anna_type_t *type);

anna_type_t *anna_type_native_create(wchar_t *name, anna_stack_frame_t *stack);

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

int anna_type_is_fake(anna_type_t *t);

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
void anna_type_setup_interface(anna_type_t *type, anna_stack_frame_t *parent);

void anna_type_prepare_member(anna_type_t *type, mid_t mid, anna_stack_frame_t *stack);

#endif
