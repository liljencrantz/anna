#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "anna_type.h"
#include "anna_macro.h"
#include "anna_prepare.h"

array_list_t  anna_type_list = 
{
    0, 0, 0
};

static anna_member_t **anna_mid_identifier_create()
{
    /*
      FIXME: Track, reallocate when we run out of space, etc.
    */
    return calloc(1,4096);
}

anna_type_t *anna_type_create(wchar_t *name, anna_stack_frame_t *stack)
{
    anna_type_t *result = calloc(1,sizeof(anna_type_t));
    result->static_member_count = 0;
    result->member_count = 0;
    hash_init(&result->name_identifier, &hash_wcs_func, &hash_wcs_cmp);
    result->mid_identifier = anna_mid_identifier_create();
    result->name = name;
    result->stack = stack;
    al_push(&anna_type_list, result);
    return result;  
}
			  
anna_node_call_t *anna_type_attribute_list_get(anna_type_t *type)
{
    return (anna_node_call_t *)type->definition->child[2];
}

anna_node_call_t *anna_type_definition_get(anna_type_t *type)
{
    return (anna_node_call_t *)type->definition->child[3];
}

void anna_type_definition_make(anna_type_t *type)
{
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

    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)anna_node_identifier_create(
	    0,
	    type->name));
    
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

    type->definition = full_definition;
    
}


anna_type_t *anna_type_native_create(wchar_t *name, anna_stack_frame_t *stack)
{    
    anna_type_t *type = anna_type_create(name, stack);
/*
    if(type_type == 0)
    {
	if(wcscmp(name, L"Type") == 0)
	{
	    type_type=type;
	}
	else
	{
	    wprintf(L"Tried to declare a type before the type type\n");
	    CRASH;
	}
    }
*/  
    anna_type_definition_make(type);
    return type;
}

static void add_member(void *key, void *value, void *aux)
{
    //wprintf(L"Got member %ls\n", key);
    wchar_t ***dest = (wchar_t ***)aux;
    **dest = key;
    (*dest)++;
}

void anna_type_get_member_names(anna_type_t *type, wchar_t **dest)
{
    hash_foreach2(&type->name_identifier, &add_member, &dest);
}

void anna_type_print(anna_type_t *type)
{
    int i;
    wprintf(L"Type %ls:\n", type->name);
    wchar_t **members = calloc(sizeof(wchar_t *), anna_type_member_count(type));
    anna_type_get_member_names(type, members);    
    for(i=0; i< anna_type_member_count(type); i++)
    {
	assert(members[i]);
	anna_member_t *member = anna_type_member_info_get(type, members[i]);
	assert(member);
	if(member->is_property)
	{
	    wprintf(L"\tproperty %ls %ls setter: %d, getter: %d\n",
		    member->type->name, members[i], 
		    member->setter_offset,
		    member->getter_offset);
	}
	else if(member->is_method)
	{
	    wprintf(L"\tfunction %ls: type: %ls, static: %ls, property: %ls, offset: %d\n",
		    members[i], member->type->name, 
		    member->is_static?L"true":L"false",
		    member->is_property?L"true":L"false",
		    member->offset);
	}
	else
	{
	    wprintf(L"\tvar %ls %ls, static: %ls, offset: %d\n",
		    member->type->name, members[i], 
		    member->is_static?L"true":L"false",
		    member->offset);
	}
    }

    free(members);
}

anna_member_t *anna_type_member_info_get(anna_type_t *type, wchar_t *name)
{
    return (anna_member_t *)hash_get(&(type->name_identifier), name);
}

size_t anna_type_member_count(anna_type_t *type)
{
    return type->member_count + type->static_member_count+type->property_count;
}

void anna_type_native_parent(anna_type_t *type, wchar_t *name)
{
    anna_node_t *param[]=
	{
	    anna_node_identifier_create(
		0,
		name)
	}
    ;
		       

    anna_node_call_t *attribute_list = 
        (anna_node_call_t *)type->definition->child[2];
    anna_node_call_add_child(
	attribute_list, 
	anna_node_call_create(
	    0,
	    anna_node_identifier_create(
		0,
		L"extends"),
	    1,
	    param));
}

anna_object_t *anna_type_wrap(anna_type_t *result)
{
    if(likely(result->wrapper))
	return result->wrapper;

    result->wrapper = anna_object_create(type_type);
    memcpy(anna_member_addr_get_mid(result->wrapper, ANNA_MID_TYPE_WRAPPER_PAYLOAD), &result, sizeof(anna_type_t *));  
    return result->wrapper;
}

anna_type_t *anna_type_unwrap(anna_object_t *wrapper)
{
    return *(anna_type_t **)anna_member_addr_get_mid(wrapper, ANNA_MID_TYPE_WRAPPER_PAYLOAD);
}

int anna_type_prepared(anna_type_t *t)
{
    return !!(t->flags & ANNA_TYPE_PREPARED_INTERFACE);
}

size_t anna_type_static_member_allocate(anna_type_t *type)
{
    if(type->static_member_count >= type->static_member_capacity)
    {
	size_t new_sz = maxi(8, 2*type->static_member_capacity);
	type->static_member = realloc(type->static_member, new_sz*sizeof(anna_object_t *));
	if(!type->static_member)
	{
	    wprintf(L"Out of memory");
	    CRASH;
	}
	type->static_member_capacity = new_sz;
    }
    return type->static_member_count++;    
}
