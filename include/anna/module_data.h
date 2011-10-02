/** 
    This file contains a convenience macro, useful for creating
    interdependent multiple interdependent native modules. 
*/

#ifndef ANNA_MODULE_DATA_H
#define ANNA_MODULE_DATA_H

typedef void (*anna_module_function_t)(anna_stack_template_t *mod);


typedef struct
{
    /**
       The name of the module.
     */
    wchar_t *name;
    /**
       A reference to a function that creates empty type objects for
       all types in the module.
     */
    anna_module_function_t creator;
    /**
       A reference to a function that adds all functions and members
       to the module and it's types.
     */
    anna_module_function_t loader;
    anna_stack_template_t *module;
} anna_module_data_t;

/**
   Create all the specified submodules in the specified parent module
 */
#define anna_module_data_create(module_data, parent)			\
    {									\
	int i;								\
	for(i=0; i<sizeof(module_data)/sizeof(*module_data); i++)	\
	{								\
	    module_data[i].module = anna_stack_create(parent);		\
	    anna_stack_name(module_data[i].module, module_data[i].name); \
	    module_data[i].module->flags |= ANNA_STACK_NAMESPACE;	\
	}								\
	for(i=0; i<sizeof(module_data)/sizeof(*module_data); i++)	\
	{								\
	    if(module_data[i].creator)					\
		module_data[i].creator(module_data[i].module);		\
	}								\
	for(i=0; i<sizeof(module_data)/sizeof(*module_data); i++)	\
	{								\
	    module_data[i].loader(module_data[i].module);		\
	    anna_stack_declare(						\
		parent,							\
		module_data[i].name,					\
		anna_stack_wrap(module_data[i].module)->type,		\
		anna_from_obj(anna_stack_wrap(module_data[i].module)),	\
		ANNA_STACK_READONLY);					\
	    anna_type_setup_interface(anna_stack_wrap(module_data[i].module)->type); \
	}								\
    }

#endif
