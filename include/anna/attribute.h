#ifndef ANNA_ATTRIBUTEQ_H
#define ANNA_ATTRIBUTE_H

#include "anna/base.h"
#include "anna/node.h"


__cold int anna_attribute_has_alias(anna_node_call_t *attribute, wchar_t *name);
__cold int anna_attribute_has_alias_reverse(anna_node_call_t *attribute, wchar_t *name);
__cold int anna_attribute_flag(anna_node_call_t *attribute, wchar_t *name);
__cold anna_node_t *anna_attribute_call(anna_node_call_t *attribute, wchar_t *name);
__cold void anna_attribute_call_all(anna_node_call_t *attribute, wchar_t *name, array_list_t *res);
/**
   Search the specified attribute list for an attribute with the specified
 */
__cold int anna_attribute_template_idx(anna_node_call_t *attr, wchar_t *name);

#endif
