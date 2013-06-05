#ifndef ANNA_OBJECT_H
#define ANNA_OBJECT_H

#include "anna/base.h"
#include "anna/alloc.h"

/**
   Create and return a new object of the specified type.
*/
__malloc __hot static inline anna_object_t *anna_object_create(anna_type_t *type)
{
    anna_object_t *result = 
	anna_alloc_object(type->object_size);
    result->type = type;
    type->internal_init(result);
    return result;
}

/**
   Create and return a new object of the specified type without
   calling it's initializer code. WARNING: Don't use unless you know what
   you're doing.  */
__malloc __hot static inline anna_object_t *anna_object_create_raw(anna_type_t *type)
{
    anna_object_t *result = 
	anna_alloc_object(type->object_size);
    result->type = type;
    return result;
}

/**
   Print a description of the specified object
 */
__cold void anna_object_print(
    anna_object_t *obj);

#endif
