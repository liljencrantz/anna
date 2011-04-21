#ifndef ANNA_FUNCTION_TYPE_H
#define ANNA_FUNCTION_TYPE_H

#include "anna.h"
#include "anna_node.h"

void anna_function_type_create(anna_function_type_t *key, anna_type_t *res);
anna_type_t *anna_function_type_each_create(
    wchar_t *name, anna_type_t *key_type, anna_type_t *value_type);
__pure anna_function_type_t *anna_function_type_unwrap(anna_type_t *type);

#endif
