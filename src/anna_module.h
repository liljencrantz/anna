#ifndef ANNA_MODULE_H
#define ANNA_MODULE_H

#include "anna_stack.h"

void anna_module_init(void);
anna_object_t *anna_module_load(wchar_t *module_name);
void anna_module_const_int(
    anna_stack_template_t *stack,
    wchar_t *name,
    int value,
    wchar_t *documentation
    );
void anna_module_const_char(
    anna_stack_template_t *stack,
    wchar_t *name,
    wchar_t value,
    wchar_t *documentation
    );
void anna_module_const_float(
    anna_stack_template_t *stack,
    wchar_t *name,
    double value,
    wchar_t *documentation
    );

/* 
   Conveniance function. Creates a function object for the specified function, declares it in the specified module and adds the specified documentation. The documentation must be a string literal.
 */
anna_function_t *anna_module_function(
    anna_stack_template_t *stack,
    wchar_t *name,
    int flags,
    anna_native_t native, 
    anna_type_t *return_type,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn,
    wchar_t *documentation
    );

void anna_module_check(
    anna_stack_template_t *parent, wchar_t *name);

#endif
