#ifndef ANNA_ATTRIBUTEQ_H
#define ANNA_ATTRIBUTE_H

#include "anna.h"
#include "anna_node.h"


int anna_attribute_has_alias(anna_node_call_t *attribute, wchar_t *name);
int anna_attribute_has_alias_reverse(anna_node_call_t *attribute, wchar_t *name);
int anna_attribute_flag(anna_node_call_t *attribute, wchar_t *name);
anna_node_t *anna_attribute_call(anna_node_call_t *attribute, wchar_t *name);
void anna_attribute_call_all(anna_node_call_t *attribute, wchar_t *name, array_list_t *res);


#endif