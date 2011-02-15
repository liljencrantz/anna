#ifndef ANNA_NODE_CREATE_H
#define ANNA_NODE_CREATE_H

#include <stdio.h>

#include "anna.h"
#include "anna_node.h"

anna_node_dummy_t *anna_node_create_dummy(
    anna_location_t *loc, 
    struct anna_object *val, 
    int is_trampoline);

anna_node_dummy_t *anna_node_create_closure(
    anna_location_t *loc,
    anna_function_t *val);

anna_node_dummy_t *anna_node_create_blob(
    anna_location_t *loc, 
    void *val);

anna_node_return_t *anna_node_create_return(
    anna_location_t *loc,
    struct anna_node *val, 
    int steps);

anna_node_import_t *anna_node_create_import(
    anna_location_t *loc,
    struct anna_node *val);

anna_node_member_get_t *anna_node_create_member_get(
    anna_location_t *loc, 
    struct anna_node *object, 
    mid_t mid);

anna_node_member_set_t *anna_node_create_member_set(
    anna_location_t *loc, 
    struct anna_node *object, 
    mid_t mid, 
    struct anna_node *value);

anna_node_int_literal_t *anna_node_create_int_literal(
    anna_location_t *loc, 
    int val);

anna_node_float_literal_t *anna_node_create_float_literal(
    anna_location_t *loc,
    double val);

anna_node_char_literal_t *anna_node_create_char_literal(
    anna_location_t *loc, 
    wchar_t val);

anna_node_string_literal_t *anna_node_create_string_literal(
    anna_location_t *loc, 
    size_t sz, 
    wchar_t *str);

anna_node_call_t *anna_node_create_call(
    anna_location_t *loc, 
    anna_node_t *function, 
    size_t argc, 
    anna_node_t **argv);

anna_node_member_call_t *anna_node_create_member_call(
    anna_location_t *loc, 
    anna_node_t *object,
    mid_t mid,
    size_t argc, 
    anna_node_t **argv);

anna_node_identifier_t *anna_node_create_identifier(
    anna_location_t *loc, 
    wchar_t *name);

anna_node_cond_t *anna_node_create_cond(
    anna_location_t *loc, 
    int type,
    anna_node_t *arg1,
    anna_node_t *arg2);

anna_node_if_t *anna_node_create_if(
    anna_location_t *loc, 
    anna_node_t *cond,
    anna_node_call_t *block1,
    anna_node_call_t *block2);

anna_node_t *anna_node_create_null(anna_location_t *loc);

anna_node_assign_t *anna_node_create_assign(
    anna_location_t *loc, 
    wchar_t *name,
    struct anna_node *value);

anna_node_declare_t *anna_node_create_declare(
    anna_location_t *loc, 
    wchar_t *name,
    struct anna_node *type,
    struct anna_node *value);

anna_node_call_t *anna_node_create_native_method_declare(
    anna_location_t *loc,
    mid_t mid,
    wchar_t *name,
    int flags,
    anna_native_t func,
    anna_node_t *result,
    size_t argc,
    anna_node_t **argv,
    wchar_t **argn);

anna_node_call_t *anna_node_create_member_declare(
    anna_location_t *loc,
    mid_t mid,
    wchar_t *name,
    int is_static,
    anna_node_t *member_type);


anna_node_call_t *anna_node_create_property(
    anna_location_t *loc,
    wchar_t *name,
    anna_node_t *member_type,
    wchar_t *getter,
    wchar_t *setter);

anna_node_t *anna_node_create_function_declaration(
    anna_location_t *loc,
    anna_node_t *result,
    size_t argc,
    anna_node_t **argv,
    wchar_t **argn);

anna_node_t *anna_node_create_templated_type(
    anna_location_t *loc,
    anna_node_t *type,
    size_t argc,
    anna_node_t **argv);

anna_node_t *anna_node_create_simple_templated_type(
    anna_location_t *loc,
    wchar_t *type_name,
    wchar_t *param_name);

anna_node_call_t *anna_node_create_block(
    anna_location_t *loc,
    size_t argc, 
    anna_node_t **argv);



#endif

