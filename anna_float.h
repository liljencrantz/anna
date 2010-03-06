#ifndef DUCK_FLOAT_H
#define DUCK_FLOAT_H

#include "anna.h"
#include "anna_node.h"


anna_object_t *anna_float_create(double value);
void anna_float_set(anna_object_t *this, double value);
double anna_float_get(anna_object_t *this);
void anna_float_type_create();

#endif
