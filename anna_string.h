#ifndef ANNA_STRING_H
#define ANNA_STRING_H

#include "anna.h"
#include "anna_node.h"


anna_object_t *anna_string_create(size_t sz, wchar_t *data);
anna_object_t *anna_string_copy(anna_object_t *obj);
void anna_string_type_create(anna_stack_template_t *stack);
void anna_string_print(anna_object_t *obj);
wchar_t *anna_string_payload(anna_object_t *obj);
size_t anna_string_get_count(anna_object_t *obj);

#endif
