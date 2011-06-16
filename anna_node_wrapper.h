
#ifndef ANNA_NODE_WRAPPER_H
#define ANNA_NODE_WRAPPER_H

struct anna_stack_template;

extern anna_type_t *node_call_wrapper_type;
extern anna_type_t *node_wrapper_type;
extern anna_type_t *node_identifier_wrapper_type;
extern anna_type_t *node_imutable_call_wrapper_type;

anna_object_t *anna_node_wrap(anna_node_t *node);
anna_object_t *anna_node_wrap_imutable(anna_node_t *node);

void anna_node_create_wrapper_types(anna_stack_template_t *stack);

anna_node_t *anna_node_unwrap(anna_object_t *this);

void anna_node_wrapper_add_method(anna_function_t *fun);


#endif
