
#ifndef ANNA_NODE_WRAPPER_H
#define ANNA_NODE_WRAPPER_H

struct anna_stack_template;

extern anna_type_t *node_call_wrapper_type;
extern anna_type_t *node_wrapper_type;

anna_object_t *anna_node_wrap(anna_node_t *node);

anna_stack_template_t *anna_node_create_wrapper_types(void);

anna_node_t *anna_node_unwrap(anna_object_t *this);


#endif
