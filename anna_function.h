#ifndef ANNA_FUNCTION_H
#define ANNA_FUNCTION_H

#include "anna.h"
#include "anna_node.h"
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
				    wchar_t **argn);

anna_function_t *anna_function_create(wchar_t *name,
				      int flags,
				      struct anna_node_call *body, 
				      anna_type_t *return_type,
				      size_t argc,
				      anna_type_t **argv,
				      wchar_t **argn,
				      struct anna_stack_frame *parent_stack,
				      int return_pop_count);

anna_function_t *anna_function_create_from_definition(
    struct anna_node_call *definition,
    anna_stack_frame_t *scope);


#endif
