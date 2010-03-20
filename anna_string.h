#ifndef ANNA_STRING_H
#define ANNA_STRING_H

#include "anna.h"
#include "anna_node.h"


anna_object_t *anna_string_create();
void anna_string_type_create(anna_stack_frame_t *stack);
void anna_string_print(anna_object_t *obj);


#endif
