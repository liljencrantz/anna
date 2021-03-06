#ifndef ANNA_CLIB_H
#define ANNA_CLIB_H

extern anna_type_t *channel_type, *task_type;
extern anna_function_t *anna_lang_nothing;

void anna_ctime_load(anna_stack_template_t *stack);

void anna_lang_create_types(anna_stack_template_t *stack);
void anna_lang_load(anna_stack_template_t *stack);

void anna_parser_create_types(anna_stack_template_t *stack);
void anna_parser_load(anna_stack_template_t *stack);

void anna_reflection_create_types(anna_stack_template_t *stack);
void anna_reflection_load(anna_stack_template_t *stack);

void anna_debug_create_types(anna_stack_template_t *stack);
void anna_debug_load(anna_stack_template_t *stack);

void anna_mp_create_types(anna_stack_template_t *stack);
void anna_mp_load(anna_stack_template_t *stack);
void anna_mp_join(void);

#endif
