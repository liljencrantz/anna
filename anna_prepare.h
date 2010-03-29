#ifndef ANNA_PREPARE_H
#define ANNA_PREPARE_H

#include "anna.h"

void anna_prepare_function(anna_function_t *function);
anna_node_t *anna_prepare_type(
    anna_type_t *type, 
    anna_function_t *function, 
    anna_node_list_t *parent);


#endif
