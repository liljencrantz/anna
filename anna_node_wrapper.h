
#ifndef ANNA_NODE_WRAPPER_H
#define ANNA_NODE_WRAPPER_H

struct anna_stack_frame;

anna_object_t *anna_node_wrap(anna_node_t *node);
void anna_node_wrapper_types_create(struct anna_stack_frame *stack);

#endif
