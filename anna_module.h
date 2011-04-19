#ifndef ANNA_MODULE_H
#define ANNA_MODULE_H

#include "anna_stack.h"

void anna_module_init(void);
anna_object_t *anna_module_load(wchar_t *module_name);

#endif
