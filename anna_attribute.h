#ifndef ANNA_ATTRIBUTEQ_H
#define ANNA_ATTRIBUTE_H

#include "anna.h"
#include "anna_node.h"


int anna_attribute_has_alias(anna_node_call_t *attribute, wchar_t *name);
wchar_t *anna_attribute_identifier(anna_node_call_t *attribute, wchar_t *name);
anna_node_t *anna_attribute_node(anna_node_call_t *attribute, wchar_t *name);

#endif