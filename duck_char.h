#ifndef DUCK_CHAR_H
#define DUCK_CHAR_H

#include "duck.h"
#include "duck_node.h"


duck_object_t *duck_char_create();
void duck_char_type_create();
wchar_t duck_char_get(duck_object_t *this);
void duck_char_set(duck_object_t *this, wchar_t value);


#endif
