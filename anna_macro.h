#ifndef ANNA_MACRO_H
#define ANNA_MACRO_H

anna_node_t *anna_macro_iter(anna_node_call_t *node,
			     anna_function_t *func, 
			     anna_node_list_t *parent);

anna_node_t *anna_macro_type_setup(anna_type_t *type, 
				   anna_function_t *function, 
				   anna_node_list_t *parent);

#endif