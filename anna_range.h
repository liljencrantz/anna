
#ifndef ANNA_RANGE_H
#define ANNA_RANGE_H

#include "anna.h"
#include "anna_node.h"

struct anna_stack_frame;

anna_object_t *anna_range_create(int from, int step, int to);

int anna_range_get_from(anna_object_t *this);
int anna_range_get_step(anna_object_t *this);
int anna_range_get_to(anna_object_t *this);

void anna_range_set_from(anna_object_t *obj, int v);
void anna_range_set_to(anna_object_t *obj, int v);
void anna_range_set_step(anna_object_t *obj, int v);

size_t anna_range_get_count(anna_object_t *obj);

void anna_range_type_create(struct anna_stack_frame *);

#endif
