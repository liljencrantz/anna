#ifndef ANNA_FUNCTION_H
#define ANNA_FUNCTION_H

#include "anna.h"
//#include "anna_node.h"
#include "anna_stack.h"

extern array_list_t anna_function_list;

anna_function_t *anna_function_unwrap(anna_object_t *wrapper);

anna_object_t *anna_function_wrap(anna_function_t *result);

anna_function_type_key_t *anna_function_unwrap_type(anna_type_t *type);

int anna_function_prepared(anna_function_t *t);

anna_function_t *anna_native_create(wchar_t *name,
				    int flags,
				    anna_native_t native, 
				    anna_type_t *return_type,
				    size_t argc,
				    anna_type_t **argv,
				    wchar_t **argn,
				    struct anna_stack_template *parent_stack);

anna_function_t *anna_function_create_from_definition(
    struct anna_node_call *definition);

anna_function_t *anna_macro_create(
    wchar_t *name,
    struct anna_node_call *body,
    wchar_t *arg_name);

anna_function_t *anna_function_create_from_block(
    struct anna_node_call *definition);

void anna_function_print(anna_function_t *function);

void anna_function_setup_interface(anna_function_t *f, anna_stack_template_t *location);
void anna_function_setup_body(anna_function_t *f);
void anna_function_argument_hint(
    anna_function_t *f,
    int argument,
    anna_type_t *type);


#endif
