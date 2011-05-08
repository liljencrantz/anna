
#ifndef ANNA_HASH_H
#define ANNA_HASH_H

#include "anna.h"
#include "anna_node.h"

struct anna_stack_template;

anna_object_t *anna_hash_create(anna_type_t *spec1, anna_type_t *spec2);
anna_object_t *anna_hash_create2(anna_type_t *hash_type);
/*
void anna_hash_set(struct anna_object *this, ssize_t offset, struct anna_object *value);
anna_object_t *anna_hash_get(anna_object_t *this, ssize_t offset);
void anna_hash_add(struct anna_object *this, struct anna_object *value);

size_t anna_hash_get_size(anna_object_t *this);
void anna_hash_set_size(anna_object_t *this, size_t sz);
*/
void anna_hash_type_create(struct anna_stack_template *);
anna_type_t *anna_hash_type_get(anna_type_t *, anna_type_t *);
void anna_hash_mark(anna_object_t *obj);

#endif
