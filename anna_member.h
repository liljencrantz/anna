#ifndef ANNA_MEMBER_H
#define ANNA_MEMBER_H

#include "anna.h"
#include "anna_node.h"

#define ANNA_MEMBER_STATIC 1
#define ANNA_MEMBER_VIRTUAL 2

anna_member_t *anna_member_unwrap(anna_object_t *obj);

anna_object_t *anna_member_wrap(anna_type_t *type, anna_member_t *member);

void anna_member_types_create(anna_stack_template_t *stack);
anna_member_t *anna_member_get(anna_type_t *type, mid_t mid);
anna_member_t *anna_member_method_search(anna_type_t *type, mid_t mid, size_t argc, anna_type_t **argv, int is_reverse);

mid_t anna_member_create(
    anna_type_t *type,
    mid_t mid,
    wchar_t *name,
    int storage,
    anna_type_t *member_type);

size_t anna_native_property_create(
    anna_type_t *type,
    mid_t mid,
    wchar_t *name,
    anna_type_t *property_type,
    anna_native_function_t getter,
    anna_native_function_t setter);

mid_t anna_const_property_create(
    anna_type_t *type, mid_t mid, wchar_t *name, anna_object_t *value);

#endif
