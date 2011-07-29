/** 
    This file contains two convenience macros, useful for creating
    interdependent native types inside of multiple modules. 
*/

#ifndef ANNA_TYPE_DATA_H
#define ANNA_TYPE_DATA_H

typedef struct 
{
    anna_type_t **addr;
    wchar_t *name;
}
anna_type_data_t;

/**
   Create all the specified types in the specified module 
 */
#define anna_type_data_create(type_data, module)				\
    {									\
	int i;								\
	for(i=0; i<(sizeof(type_data)/sizeof(*type_data)); i++)		\
	{								\
	    *(type_data[i].addr) = anna_type_native_create(type_data[i].name, module); \
	}								\
    }

/**
   Declare the specified types in the specified module and copy the
   members from the object type into each of the types.

   Note: If the type is either the object type or the null type, the
   copying of members from the object type will be skipped.
 */
#define anna_type_data_register(type_data, module)			\
    {									\
	int i;								\
	for(i=0; i<(sizeof(type_data)/sizeof(*type_data)); i++)		\
	{								\
	    if(((*type_data[i].addr) != object_type) && ((*type_data[i].addr) != null_type)) \
	    {								\
		anna_type_copy_object(*type_data[i].addr);		\
	    }								\
	    anna_stack_declare(						\
		module, (*type_data[i].addr)->name,			\
		type_type, anna_from_obj(anna_type_wrap(*type_data[i].addr)), ANNA_STACK_READONLY); \
	}								\
    }
    
#endif
