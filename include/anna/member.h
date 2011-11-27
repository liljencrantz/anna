#ifndef ANNA_MEMBER_H
#define ANNA_MEMBER_H

#include "anna/base.h"
#include "anna/node.h"

extern anna_type_t *member_method_type, *member_property_type, *member_variable_type;

anna_member_t *anna_member_unwrap(anna_object_t *obj);

anna_object_t *anna_member_wrap(anna_type_t *type, anna_member_t *member);

void anna_member_create_types(anna_stack_template_t *stack);
void anna_member_load(anna_stack_template_t *stack);


anna_member_t *anna_member_get(anna_type_t *type, mid_t mid);
anna_member_t *anna_member_method_search(
    anna_type_t *type, mid_t mid, 
    anna_node_call_t *call, 
    int is_reverse);

mid_t anna_member_create(
    anna_type_t *type,
    mid_t mid,
    int storage,
    anna_type_t *member_type);

mid_t anna_member_create_method(
    anna_type_t *type,
    mid_t mid,
    anna_function_t *method);

mid_t anna_member_create_blob(
    anna_type_t *type,
    mid_t mid,
    int storage,
    size_t sz);

size_t anna_member_create_native_property(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *property_type,
    anna_native_t getter,
    anna_native_t setter,
    wchar_t *doc);

size_t anna_member_create_property(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *property_type,
    ssize_t getter_offset,
    ssize_t setter_offset);

void anna_member_type_set(
    anna_type_t *type,
    mid_t mid,
    anna_type_t *member_type);

/**
   Convenience method for creating a new method in the specified type.
*/
size_t anna_member_create_native_method(
    anna_type_t *type,
    mid_t mid,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn,
    anna_node_t **argd,
    wchar_t *doc);

size_t anna_member_create_native_type_method(
    anna_type_t *type,
    mid_t mid,
    int flags,
    anna_native_t func,
    anna_type_t *result,
    size_t argc,
    anna_type_t **argv,
    wchar_t **argn,
    anna_node_t **argd,
    wchar_t *doc);

void anna_member_document(
    anna_type_t *type,
    mid_t mid,
    wchar_t *doc);

static inline int anna_member_is_bound(anna_member_t *member)
{
    return !!(member->storage & ANNA_MEMBER_BOUND);
}

static inline void anna_member_set_bound(anna_member_t *member, int value)
{
    if(value)
    {
	member->storage |= ANNA_MEMBER_BOUND;
    }
    else
    {
	member->storage = (member->storage & ~ANNA_MEMBER_BOUND);
    }
}

static inline void anna_member_set_static(anna_member_t *member, int value)
{
    if(value)
    {
	member->storage |= ANNA_MEMBER_STATIC;
    }
    else
    {
	member->storage = (member->storage & ~ANNA_MEMBER_STATIC);
    }
}

static inline int anna_member_is_property(anna_member_t *member)
{
    return !!(member->storage & ANNA_MEMBER_PROPERTY);
}

static inline void anna_member_set_property(anna_member_t *member, int value)
{
    if(value)
    {
	member->storage |= ANNA_MEMBER_PROPERTY;
    }
    else
    {
	member->storage = (member->storage & ~ANNA_MEMBER_PROPERTY);
    }
}

/**
   Returns the user visible function type of this member, that is, the
   function type without the implicit this parameter.

   If the member is not of a function type, null is returned.
 */
anna_function_type_t *anna_member_bound_function_type(anna_member_t *member);


#endif
