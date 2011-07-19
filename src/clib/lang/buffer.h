
#ifndef ANNA_BUFFER_H
#define ANNA_BUFFER_H

struct anna_stack_template;

anna_object_t *anna_buffer_create(void);

void anna_buffer_set(struct anna_object *this, ssize_t offset, unsigned char value);
unsigned char anna_buffer_get(anna_object_t *this, ssize_t offset);

size_t anna_buffer_get_count(anna_object_t *this);
void anna_buffer_set_count(anna_object_t *this, size_t sz);

size_t anna_buffer_get_capacity(anna_object_t *this);
void anna_buffer_set_capacity(anna_object_t *this, size_t sz);

unsigned char *anna_buffer_get_payload(anna_object_t *this);

void anna_buffer_type_create(void);

#endif
