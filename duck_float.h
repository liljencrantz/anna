#ifndef DUCK_FLOAT_H
#define DUCK_FLOAT_H

#include "duck.h"
#include "duck_node.h"


duck_object_t *duck_float_create();
void duck_float_set(duck_object_t *this, double value);
double duck_float_get(duck_object_t *this);
void duck_float_type_create();

#endif
