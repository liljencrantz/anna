#ifndef ANNA_TYPE_H
#define ANNA_TYPE_H

#include "anna.h"
#include "anna_node.h"
#include "anna_stack.h"

anna_node_call_t *anna_type_attribute_list_get(anna_type_t *type);

anna_node_call_t *anna_type_definition_get(anna_type_t *type);

anna_type_t *anna_type_native_create(wchar_t *name, anna_stack_frame_t *stack);

void anna_type_native_parent(anna_type_t *type, wchar_t *name);

void anna_type_get_member_names(anna_type_t *type, wchar_t **dest);

void anna_type_native_setup(anna_type_t *, anna_stack_frame_t *);

void anna_type_print(anna_type_t *type);

anna_member_t *anna_type_member_info_get(anna_type_t *type, wchar_t *name);

size_t anna_type_member_count(anna_type_t *type);

#endif
