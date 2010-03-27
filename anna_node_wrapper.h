
#ifndef ANNA_NODE_WRAPPER_H
#define ANNA_NODE_WRAPPER_H

struct anna_stack_frame;

extern anna_type_t *node_call_wrapper_type;

anna_object_t *anna_node_wrap(anna_node_t *node);
void anna_node_wrapper_types_create(struct anna_stack_frame *stack);
anna_node_t *anna_node_unwrap(anna_object_t *this);


#endif
