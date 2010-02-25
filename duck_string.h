#ifndef DUCK_STRING_H
#define DUCK_STRING_H

#include "duck.h"
#include "duck_node.h"


duck_object_t *duck_string_create();
void duck_string_type_create();
wchar_t *duck_string_get_payload(duck_object_t *this);
size_t duck_string_get_payload_size(duck_object_t *this);

#endif
