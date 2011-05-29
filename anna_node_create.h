#ifndef ANNA_NODE_CREATE_H
#define ANNA_NODE_CREATE_H

#include <gmp.h>
#include <stdio.h>

#include "anna.h"
#include "anna_node.h"

anna_node_dummy_t *anna_node_create_dummy(
    anna_location_t *loc, 
    struct anna_object *val);

anna_node_closure_t *anna_node_create_closure(
    anna_location_t *loc,
    anna_function_t *val);

anna_node_type_t *anna_node_create_type(
    anna_location_t *loc,
    anna_type_t *val);

anna_node_wrapper_t *anna_node_create_return(
    anna_location_t *loc,
    struct anna_node *val,
    int type);

anna_node_member_access_t *anna_node_create_member_get(
    anna_location_t *loc, 
    struct anna_node *object, 
    mid_t mid);

anna_node_member_access_t *anna_node_create_member_set(
    anna_location_t *loc, 
    struct anna_node *object, 
    mid_t mid, 
    struct anna_node *value);

anna_node_int_literal_t *anna_node_create_int_literal(
    anna_location_t *loc, 
    mpz_t val);

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

#define anna_node_create_call2( ... ) anna_node_create_call_internal( 0, __VA_ARGS__, (void *)0 )

__sentinel anna_node_call_t *anna_node_create_call_internal(
    int is_block,
    anna_location_t *loc,
    ...);

anna_node_call_t *anna_node_create_specialize(
    anna_location_t *loc, 
    anna_node_t *function, 
    size_t argc, 
    anna_node_t **argv);

anna_node_call_t *anna_node_create_member_call(
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
    struct anna_node *value,
    struct anna_node_call *attr,
    int is_const);

anna_node_wrapper_t *anna_node_create_type_of(
    anna_location_t *loc,
    anna_node_t *payload);

anna_node_wrapper_t *anna_node_create_return_type_of(
    anna_location_t *loc,
    anna_node_t *payload);

anna_node_wrapper_t *anna_node_create_input_type_of(
    anna_location_t *loc,
    anna_node_t *payload,
    int idx);

anna_node_cond_t *anna_node_create_mapping(
    anna_location_t *loc,
    anna_node_t *from,
    anna_node_t *to);

anna_node_call_t *anna_node_create_block(
    anna_location_t *loc,
    size_t argc, 
    anna_node_t **argv);

#define anna_node_create_block2( ... ) anna_node_create_call_internal( 1, __VA_ARGS__, (void *)0 )

#endif

