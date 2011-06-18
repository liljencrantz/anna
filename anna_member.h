#ifndef ANNA_MEMBER_H
#define ANNA_MEMBER_H

#include "anna.h"
#include "anna_node.h"

#define ANNA_MEMBER_STATIC 1
#define ANNA_MEMBER_VIRTUAL 2
#define ANNA_MEMBER_ALLOC 4

anna_member_t *anna_member_unwrap(anna_object_t *obj);

anna_object_t *anna_member_wrap(anna_type_t *type, anna_member_t *member);

void anna_member_create_types(anna_stack_template_t *stack);
void anna_member_load(anna_stack_template_t *stack);


anna_member_t *anna_member_get(anna_type_t *type, mid_t mid);
anna_member_t *anna_member_method_search(
    anna_type_t *type, mid_t mid, 
    anna_node_call_t *call, 
    int is_reverse);

mid_t anna_member_create(
    anna_type_t *type,
    mid_t mid,
    int storage,
    anna_type_t *member_type);

mid_t anna_member_create_method(
    anna_type_t *type,
    mid_t mid,
    anna_function_t *method);

mid_t anna_member_create_blob(
    anna_type_t *type,
    mid_t mid,
    int storage,
    size_t sz);

size_t anna_member_create_native_property(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *property_type,
    anna_native_t getter,
    anna_native_t setter);

size_t anna_member_create_property(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *property_type,
    ssize_t getter_offset,
    ssize_t setter_offset);

void anna_member_type_set(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *member_type);

/**
   Convenience method for creating a new method in the specified type.
*/
size_t anna_member_create_native_method(
    anna_type_t *type,
    mid_t mid,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn);

size_t anna_member_create_native_type_method(
    anna_type_t *type,
    mid_t mid,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn);

#endif
