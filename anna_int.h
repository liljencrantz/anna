#ifndef ANNA_INT_H
#define ANNA_INT_H

#include "anna.h"
#include "anna_node.h"

struct anna_stack_template;
anna_object_t *anna_int_one;

anna_object_t *anna_int_create(int);
void anna_int_set(anna_object_t *this, int value);
int anna_int_get(anna_object_t *this);
void anna_int_type_create(struct anna_stack_template *);

#endif
