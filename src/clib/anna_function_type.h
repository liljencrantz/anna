#ifndef ANNA_FUNCTION_TYPE_H
#define ANNA_FUNCTION_TYPE_H

void anna_function_type_create(anna_function_type_t *key, anna_type_t *res);
anna_type_t *anna_function_type_each_create(
    wchar_t *name, anna_type_t *key_type, anna_type_t *value_type);
__pure anna_function_type_t *anna_function_type_unwrap(anna_type_t *type);
void anna_function_type_load(anna_stack_template_t *stack);
void anna_function_type_create_types(anna_stack_template_t *stack);

#endif
