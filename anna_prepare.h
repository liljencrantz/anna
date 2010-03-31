#ifndef ANNA_PREPARE_H
#define ANNA_PREPARE_H

#include "anna.h"

/**
  Run macros, optimize and validate AST of specified function
 */
void anna_prepare_function(anna_function_t *function);
/**
   Fill out type interface, i.e. make sure names and types of all type
   members are known.
 */
anna_node_t *anna_prepare_type_interface(anna_type_t *type);

/**
   Prepare the actual bodies of the type, i.e. run
   anna_prepare_function on all methods, etc.
 */
anna_node_t *anna_prepare_type_implementation(anna_type_t *type);

#endif
