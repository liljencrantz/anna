
#ifndef ANNA_RANGE_H
#define ANNA_RANGE_H

struct anna_stack_template;

ssize_t anna_range_get_from(anna_object_t *this);
ssize_t anna_range_get_step(anna_object_t *this);
ssize_t anna_range_get_to(anna_object_t *this);

void anna_range_set_from(anna_object_t *obj, ssize_t v);
void anna_range_set_to(anna_object_t *obj, ssize_t v);
void anna_range_set_step(anna_object_t *obj, ssize_t v);

ssize_t anna_range_get_count(anna_object_t *obj);

void anna_range_type_create(void);
int anna_range_get_open(anna_object_t *obj);
void anna_range_set_open(anna_object_t *obj, int v);

#endif
