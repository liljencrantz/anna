#ifndef ANNA_UTIL_H
#define ANNA_UTIL_H

#include "anna_node.h"

/**
   Generate a unique variable name. The name will start with a '@'
   symbol, which ordinary variable names can not contain. If prefix is
   not null, it will be included at the beginning of the generated
   name. If definition is not null, the specified location will be
   encoded into the name.
 */
wchar_t *anna_util_identifier_generate(wchar_t *prefix, anna_location_t *definition);

/**
  Calculate a suitable hash value for the data livng at the specified
  pointer. The count parameter is the number of elements (size of int)
  of the data. The output data will be truncated to fit into a fast
  int.
 */
int anna_hash(int *data, size_t count);

anna_vmstack_t *anna_util_noop(anna_vmstack_t *stack, anna_object_t *me);

#endif
