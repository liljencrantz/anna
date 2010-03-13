
#include "anna_type.h"

anna_node_call_t *anna_type_attribute_list_get(anna_type_t *type)
{
    return (anna_node_call_t *)type->definition->child[2];
}

anna_node_call_t *anna_type_definition_get(anna_type_t *type)
{
    return (anna_node_call_t *)type->definition->child[3];
}

anna_type_t *anna_type_native_create(wchar_t *name, anna_stack_frame_t *stack)
{    
    anna_type_t *type = anna_type_create(name, 64, 0);
    anna_stack_declare(stack, name, type_type, type->wrapper);
    
    anna_node_call_t *definition = 
	anna_node_call_create(
	    0,
	    (anna_node_t *)anna_node_identifier_create(0, L"__block__"),
	    0,
	    0);
    
    anna_node_call_t *full_definition = 
	anna_node_call_create(
	    0,
	    (anna_node_t *)anna_node_identifier_create(0, L"__type__"),
	    0,
	    0);
    type->definition = full_definition;

    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    name));
    
    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    L"class"));
    
    anna_node_call_t *attribute_list = 
	anna_node_call_create(
	    0,
	    (anna_node_t *)anna_node_identifier_create(0, L"__block__"),
	    0,
	    0);	

    anna_node_call_add_child(
	full_definition,	
	(anna_node_t *)attribute_list);
    
    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)definition);
    
    return type;
    
}

void anna_type_native_setup(anna_type_t *type, anna_stack_frame_t *stack)
{
    anna_function_t *func;

    func = anna_native_create(L"!anonymous",
			      ANNA_FUNCTION_MACRO,
			      (anna_native_t)(anna_native_function_t)0,
			      0,
			      0,
			      0, 
			      0);
    func->stack_template=stack;
    anna_macro_type_setup(type, func, 0);
}

