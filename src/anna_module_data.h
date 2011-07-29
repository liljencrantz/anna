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
} anna_module_data_t;


/**
   Create all the specified submodules in the specified parent module
 */
#define anna_module_data_create(module_data, parent)			\
    {									\
	int i;								\
	anna_stack_template_t *substack[sizeof(modules)/sizeof(*modules)]; \
	for(i=0; i<sizeof(modules)/sizeof(*modules); i++)		\
	{								\
	    substack[i] = anna_stack_create(parent);			\
	    anna_stack_name(substack[i], module_data[i].name);		\
	    substack[i]->flags |= ANNA_STACK_NAMESPACE;			\
	}								\
	for(i=0; i<sizeof(modules)/sizeof(*modules); i++)		\
	{								\
	    if(modules[i].creator)					\
		modules[i].creator(substack[i]);			\
	}								\
	for(i=0; i<sizeof(modules)/sizeof(*modules); i++)		\
	{								\
	    modules[i].loader(substack[i]);				\
	    anna_stack_declare(						\
		parent,							\
		modules[i].name,					\
		anna_stack_wrap(substack[i])->type,			\
		anna_from_obj(anna_stack_wrap(substack[i])),		\
		ANNA_STACK_READONLY);					\
	    anna_type_setup_interface(anna_stack_wrap(substack[i])->type); \
	}								\
    }

#endif
