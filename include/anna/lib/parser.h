
#ifndef ANNA_NODE_WRAPPER_H
#define ANNA_NODE_WRAPPER_H

struct anna_stack_template;

extern anna_type_t *node_call_type, *node_type,
    *node_identifier_type, *node_imutable_call_type,
    *node_int_literal_type, *node_string_literal_type,
    *node_char_literal_type, *node_float_literal_type,
    *node_null_literal_type, *node_dummy_type;

anna_object_t *anna_node_wrap(anna_node_t *node);
anna_node_t *anna_node_unwrap(anna_object_t *this);

void anna_node_wrapper_add_method(anna_function_t *fun);

#endif
