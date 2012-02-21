#ifndef ANNA_FUNCTION_TYPE_H
#define ANNA_FUNCTION_TYPE_H

__pure anna_function_type_t *anna_function_type_unwrap(anna_type_t *type);

__pure static inline int anna_function_type_is_variadic(anna_function_type_t *f)
{
    return !!(f->flags & ANNA_FUNCTION_VARIADIC);
}

__pure static inline int anna_function_type_is_variadic_named(anna_function_type_t *f)
{
    return !!(f->flags & ANNA_FUNCTION_VARIADIC_NAMED);
}


#endif
