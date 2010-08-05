#ifndef ANNA_MACRO_H
#define ANNA_MACRO_H

/**
   Declare all native macros
 */
void anna_macro_init(struct anna_stack_frame *stack);

anna_node_t *anna_macro_iter(anna_node_call_t *node,
			     anna_function_t *func, 
			     anna_node_list_t *parent);

anna_node_t *anna_macro_type_setup(anna_type_t *type, 
				   anna_function_t *function,
				   anna_node_list_t *parent);

anna_node_t *anna_macro_function_internal(anna_type_t *type, 
					  anna_node_call_t *node, 
					  anna_function_t *function, 
					  anna_node_list_t *parent,
					  int declare);




#endif
