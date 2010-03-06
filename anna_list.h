
#ifndef ANNA_LIST_H
#define ANNA_LIST_H

#include "anna.h"
#include "anna_node.h"

struct anna_stack_frame;

anna_object_t *anna_list_create();

void anna_list_set(struct anna_object *this, ssize_t offset, struct anna_object *value);
anna_object_t *anna_list_get(anna_object_t *this, ssize_t offset);
void anna_list_add(struct anna_object *this, struct anna_object *value);

size_t anna_list_get_size(anna_object_t *this);
void anna_list_set_size(anna_object_t *this, size_t sz);

size_t anna_list_get_capacity(anna_object_t *this);
void anna_list_set_capacity(anna_object_t *this, size_t sz);

//anna_object_t **anna_list_get_payload(anna_object_t *this);

void anna_list_type_create(struct anna_stack_frame *);

#endif
