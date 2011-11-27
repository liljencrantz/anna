#ifndef ANNA_FUNCTION_TYPE_H
#define ANNA_FUNCTION_TYPE_H

anna_type_t *anna_function_type_each_create(
    wchar_t *name, anna_type_t *key_type, anna_type_t *value_type);
__pure anna_function_type_t *anna_function_type_unwrap(anna_type_t *type);
__pure static inline int anna_function_type_is_variadic(anna_function_type_t *f)
{
    return !!(f->flags & ANNA_FUNCTION_VARIADIC);
}


#endif
