
#ifndef DUCK_LIST_H
#define DUCK_LIST_H

#include "duck.h"
#include "duck_node.h"

struct duck_stack_frame;

duck_object_t *duck_list_create();

void duck_list_set(struct duck_object *this, ssize_t offset, struct duck_object *value);
duck_object_t *duck_list_get(duck_object_t *this, ssize_t offset);
void duck_list_add(struct duck_object *this, struct duck_object *value);

size_t duck_list_get_size(duck_object_t *this);
void duck_list_set_size(duck_object_t *this, size_t sz);

size_t duck_list_get_capacity(duck_object_t *this);
void duck_list_set_capacity(duck_object_t *this, size_t sz);

//duck_object_t **duck_list_get_payload(duck_object_t *this);

void duck_list_type_create(struct duck_stack_frame *);

#endif
