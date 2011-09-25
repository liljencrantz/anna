#ifndef ANNA_USE_H
#define ANNA_USE_H

struct anna_stack_template;
struct anna_type;

struct anna_use
{
    struct anna_node *node;
    struct anna_type *type;
};

typedef struct anna_use anna_use_t;

anna_use_t *anna_use_create_stack(struct anna_stack_template *stack);
anna_use_t *anna_use_create_node(anna_node_t *node, struct anna_type *type);

#endif
