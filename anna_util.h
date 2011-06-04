#ifndef ANNA_UTIL_H
#define ANNA_UTIL_H

#include "anna_node.h"

wchar_t *anna_util_identifier_generate(wchar_t *prefix, anna_location_t *definition);

/*
  Calculate a suitable hash value for the data livng at the specified
  pointer. The count parameter is the number of elements (size of int)
  of the data. The output data will be truncated to fit into a fast
  int.
 */
int anna_hash(int *data, size_t count);

#endif
