#ifndef ANNA_MODULE_DATA_H
#define ANNA_MODULE_DATA_H

typedef void (*anna_module_function_t)(anna_stack_template_t *mod);

typedef struct
{
    wchar_t *name;
    anna_module_function_t creator;
    anna_module_function_t loader;
} anna_module_data_t;


/**
   Create all the specified modules in the specified stack 
 */
#define anna_module_data_create(module_data, stack)			\
    {									\
	int i;								\
	anna_stack_template_t *substack[sizeof(modules)/sizeof(*modules)]; \
	for(i=0; i<sizeof(modules)/sizeof(*modules); i++)		\
	{								\
	    substack[i] = anna_stack_create(stack);			\
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
		stack,							\
		modules[i].name,					\
		anna_stack_wrap(substack[i])->type,			\
		anna_from_obj(anna_stack_wrap(substack[i])),		\
		ANNA_STACK_READONLY);					\
	}								\
    }

#endif
