#ifndef ANNA_MODULE_H
#define ANNA_MODULE_H

#include "anna_stack.h"

void anna_module_init(void);
anna_object_t *anna_module_load(wchar_t *module_name);
void anna_module_const_int(
    anna_stack_template_t *stack,
    wchar_t *name,
    int value);


#endif
