#ifndef ANNA_FUNCTION_H
#define ANNA_FUNCTION_H

#include "anna.h"
//#include "anna_node.h"
#include "anna_stack.h"

extern array_list_t anna_function_list;

static inline anna_function_t *anna_function_unwrap(anna_object_t *obj)
{
#ifdef ANNA_WRAPPER_CHECK_ENABLED
    if(!obj)
    {
	wprintf(
	    L"Critical: Tried to unwrap null pointer as a function\n");
	CRASH;
    }
#endif

    anna_member_t *m = obj->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_PAYLOAD];
    if(!unlikely((long)m))
    {
	return 0;
    }
    
    anna_function_t *fun = (anna_function_t *)obj->member[m->offset];

    if(likely((long)fun)) 
    {
	//wprintf(L"Got object of type %ls with native method payload\n", obj->type->name);
	return fun;
    }
    else 
    {
	anna_object_t **function_wrapper_ptr =
	    anna_static_member_addr_get_mid(
		obj->type, 
		ANNA_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    //wprintf(L"Got object with __call__ member\n");
	    return anna_function_unwrap(
		*function_wrapper_ptr);	    
	}
	return 0;	
    }
}

anna_object_t *anna_function_wrap(anna_function_t *result);

anna_function_type_key_t *anna_function_unwrap_type(anna_type_t *type);

int anna_function_prepared(anna_function_t *t);

anna_function_t *anna_native_create(wchar_t *name,
				    int flags,
				    anna_native_t native, 
				    anna_type_t *return_type,
				    size_t argc,
				    anna_type_t **argv,
				    wchar_t **argn,
				    struct anna_stack_template *parent_stack);

anna_function_t *anna_function_create_from_definition(
    struct anna_node_call *definition);

anna_function_t *anna_macro_create(
    wchar_t *name,
    struct anna_node_call *body,
    wchar_t *arg_name);

anna_function_t *anna_function_create_from_block(
    struct anna_node_call *definition);

void anna_function_print(anna_function_t *function);

void anna_function_setup_interface(anna_function_t *f, anna_stack_template_t *location);
void anna_function_setup_body(anna_function_t *f);
void anna_function_argument_hint(
    anna_function_t *f,
    int argument,
    anna_type_t *type);

int anna_function_has_alias(anna_function_t *fun, wchar_t *name);
int anna_function_has_alias_reverse(anna_function_t *fun, wchar_t *name);

void anna_function_alias_add(anna_function_t *fun, wchar_t *name);
void anna_function_alias_reverse_add(anna_function_t *fun, wchar_t *name);

#endif
