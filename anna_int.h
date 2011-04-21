#ifndef ANNA_INT_H
#define ANNA_INT_H

#include "anna.h"
#include "anna_node.h"

struct anna_stack_template;
extern anna_object_t *anna_int_one;
extern anna_object_t *anna_int_minus_one;
extern anna_object_t *anna_int_zero;

anna_object_t *anna_int_create(int);
int anna_int_get(anna_object_t *this);
void anna_int_type_create(struct anna_stack_template *);

#endif
