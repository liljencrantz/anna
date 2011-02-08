#ifndef ANNA_MEMBER_H
#define ANNA_MEMBER_H

#include "anna.h"
#include "anna_node.h"

anna_member_t *anna_member_unwrap(anna_object_t *obj);

anna_object_t *anna_member_wrap(anna_type_t *type, anna_member_t *member);

void anna_member_types_create(anna_stack_frame_t *stack);
anna_member_t *anna_member_get(anna_type_t *type, size_t mid);
anna_member_t *anna_member_method_search(anna_type_t *type, size_t mid, size_t argc, anna_type_t **argv);

size_t anna_member_create(
    anna_type_t *type,
    size_t mid,
    wchar_t *name,
    int is_static,
    anna_type_t *member_type);

#endif
