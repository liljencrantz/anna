
#ifndef ANNA_LIST_H
#define ANNA_LIST_H

#include "anna.h"
#include "anna_node.h"

extern anna_type_t *imutable_list_type;
extern anna_type_t *mutable_list_type;
extern anna_type_t *any_list_type;

struct anna_stack_template;

anna_object_t *anna_list_create_imutable(anna_type_t *spec);
anna_object_t *anna_list_create_mutable(anna_type_t *spec);
anna_object_t *anna_list_create2(anna_type_t *list_type);

void anna_list_set(struct anna_object *this, ssize_t offset, anna_entry_t *value);
anna_entry_t *anna_list_get(anna_object_t *this, ssize_t offset);
void anna_list_add(struct anna_object *this, anna_entry_t *value);

size_t anna_list_get_count(anna_object_t *this);
void anna_list_set_count(anna_object_t *this, size_t sz);

size_t anna_list_get_capacity(anna_object_t *this);
void anna_list_set_capacity(anna_object_t *this, size_t sz);

anna_entry_t **anna_list_get_payload(anna_object_t *this);

void anna_list_type_create(void);
anna_type_t *anna_list_type_get_mutable(anna_type_t *subtype);
anna_type_t *anna_list_type_get_imutable(anna_type_t *subtype);
anna_type_t *anna_list_type_get_any(anna_type_t *subtype);

static inline ssize_t anna_list_calc_offset(ssize_t offset, size_t size)
{
    if(unlikely(offset < 0)) {
	if((-offset) > size)
	    return -1;
	
	return size+offset;
    }
    return offset;
}
void anna_list_add_method(anna_function_t *fun);

#endif
