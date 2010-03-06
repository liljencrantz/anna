#ifndef ANNA_STRING_H
#define ANNA_STRING_H

#include "anna.h"
#include "anna_node.h"


anna_object_t *anna_string_create();
void anna_string_type_create();
wchar_t *anna_string_get_payload(anna_object_t *this);
size_t anna_string_get_payload_size(anna_object_t *this);

#endif
