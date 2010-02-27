#ifndef DUCK_INT_H
#define DUCK_INT_H

#include "duck.h"
#include "duck_node.h"

struct duck_stack_frame;
duck_object_t *duck_int_one;

duck_object_t *duck_int_create(int);
void duck_int_set(duck_object_t *this, int value);
int duck_int_get(duck_object_t *this);
void duck_int_type_create(struct duck_stack_frame *);

#endif
