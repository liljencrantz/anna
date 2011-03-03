#ifndef ANNA_CHAR_H
#define ANNA_CHAR_H

#include "anna.h"
#include "anna_node.h"


anna_object_t *anna_char_create(wchar_t val);
void anna_char_type_create(anna_stack_frame_t *);
wchar_t anna_char_get(anna_object_t *this);
void anna_char_set(anna_object_t *this, wchar_t value);


#endif
