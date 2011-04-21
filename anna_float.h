#ifndef ANNA_FLOAT_H
#define ANNA_FLOAT_H

#include "anna.h"
#include "anna_node.h"


anna_object_t *anna_float_create(double value);
double anna_float_get(anna_object_t *this);
void anna_float_type_create(anna_stack_template_t *);

#endif
