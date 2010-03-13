#ifndef ANNA_CHAR_H
#define ANNA_CHAR_H

#include "anna.h"
#include "anna_node.h"


anna_object_t *anna_char_create();
void anna_char_type_create();
wchar_t anna_char_get(anna_object_t *this);
void anna_char_set(anna_object_t *this, wchar_t value);


#endif