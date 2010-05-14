#ifndef ANNA_NODE_CREATE_H
#define ANNA_NODE_CREATE_H

#include <stdio.h>

#include "anna.h"
#include "anna_node.h"

anna_node_dummy_t *anna_node_dummy_create(
    anna_location_t *loc, 
    struct anna_object *val, 
    int is_trampoline);

anna_node_dummy_t *anna_node_blob_create(
    anna_location_t *loc, 
    void *val);

anna_node_return_t *anna_node_return_create(
    anna_location_t *loc,
    struct anna_node *val, 
    int steps);

anna_node_import_t *anna_node_import_create(
    anna_location_t *loc,
    struct anna_node *val);

anna_node_member_get_t *anna_node_member_get_create(
    anna_location_t *loc, 
    struct anna_node *object, 
    size_t mid, 
    struct anna_type *type, 
    int wrap);

anna_node_member_set_t *anna_node_member_set_create(
    anna_location_t *loc, 
    struct anna_node *object, 
    size_t mid, 
    struct anna_node *value, 
    struct anna_type *type);

anna_node_int_literal_t *anna_node_int_literal_create(
    anna_location_t *loc, 
    int val);

anna_node_float_literal_t *anna_node_float_literal_create(
    anna_location_t *loc,
    double val);

anna_node_char_literal_t *anna_node_char_literal_create(
    anna_location_t *loc, 
    wchar_t val);

anna_node_string_literal_t *anna_node_string_literal_create(
    anna_location_t *loc, 
    size_t sz, 
    wchar_t *str);

anna_node_call_t *anna_node_call_create(
    anna_location_t *loc, 
    anna_node_t *function, 
    size_t argc, 
    anna_node_t **argv);
anna_node_identifier_t *anna_node_identifier_create(
    anna_location_t *loc, 
    wchar_t *name);
anna_node_t *anna_node_null_create(anna_location_t *loc);
anna_node_assign_t *anna_node_assign_create(
    anna_location_t *loc, 
    anna_sid_t sid, 
    struct anna_node *value);

anna_node_call_t *anna_node_native_method_declare_create(
    anna_location_t *loc,
    ssize_t mid,
    wchar_t *name,
    int flags,
    anna_native_t func,
    anna_node_t *result,
    size_t argc,
    anna_node_t **argv,
    wchar_t **argn);

anna_node_call_t *anna_node_member_declare_create(
    anna_location_t *loc,
    ssize_t mid,
    wchar_t *name,
    int is_static,
    anna_node_t *member_type);


anna_node_call_t *anna_node_property_create(
    anna_location_t *loc,
    wchar_t *name,
    anna_node_t *member_type,
    wchar_t *getter,
    wchar_t *setter);

anna_node_t *anna_node_function_declaration_create(
    anna_location_t *loc,
    anna_node_t *result,
    size_t argc,
    anna_node_t **argv,
    wchar_t **argn);

anna_node_t *anna_node_templated_type_create(
    anna_location_t *loc,
    anna_node_t *type,
    size_t argc,
    anna_node_t **argv);

anna_node_t *anna_node_simple_templated_type_create(
    anna_location_t *loc,
    wchar_t *type_name,
    wchar_t *param_name);

anna_node_call_t *anna_node_block_create(
    anna_location_t *loc);


#endif

