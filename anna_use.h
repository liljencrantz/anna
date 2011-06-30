#ifndef ANNA_USE_H
#define ANNA_USE_H

struct anna_stack_template;
struct anna_type;

typedef struct
{
    struct anna_node *node;
    struct anna_type *type;
}
    anna_use_t;

anna_use_t *anna_use_create_stack(struct anna_stack_template *stack);
anna_use_t *anna_use_create_identifier(wchar_t *name, struct anna_type *type);

#endif
