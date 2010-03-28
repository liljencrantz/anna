#ifndef ANNA_FUNCTION_H
#define ANNA_FUNCTION_H

#include "anna.h"
#include "anna_node.h"
#include "anna_stack.h"

anna_function_t *anna_function_unwrap(anna_object_t *wrapper);

anna_object_t *anna_function_wrap(anna_function_t *result);

anna_function_type_key_t *anna_function_unwrap_type(anna_type_t *type);

#endif
