#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna_type.h"
#include "anna_node_create.h"
#include "anna_macro.h"
#include "anna_prepare.h"
#include "anna_function.h"
#include "anna_member.h"
#include "anna.h"
#include "anna_node_check.h"

array_list_t  anna_type_list = 
{
    0, 0, 0
};

void anna_type_reallocade_mid_lookup(size_t old_sz, size_t sz)
{
    int i;
    for(i=0;i<al_get_count(&anna_type_list); i++)
    {
	anna_type_t *type = (anna_type_t *)al_get(&anna_type_list, i);
	type->mid_identifier = realloc(type->mid_identifier, sz*sizeof(anna_member_t *));
	memset(&type->mid_identifier[old_sz], 0, (sz-old_sz)*sizeof(anna_member_t *));
    }
}

static void anna_type_add_implicit_this(
    anna_type_t *type)
{
    size_t i;
    anna_node_call_t *body= type->body;
    
    for(i=0; i<body->child_count; i++)
    {
	if(anna_node_is_call_to(body->child[i], L"__var__"))
	{
	    anna_node_call_t *decl =(anna_node_call_t *)body->child[i];
	    if(decl->child_count >= 3)
	    {
		if(anna_node_is_call_to(decl->child[2], L"__def__"))
		{
		    anna_node_call_t *def =(anna_node_call_t *)decl->child[2];
		    if(def->child_count >= 5)
		    {
			if(anna_node_is_call_to(def->child[2], L"__block__"))
			{
			    anna_node_call_t *def_decl =(anna_node_call_t *)def->child[2];
			    anna_node_t *param[] ={
				anna_node_create_identifier(0, L"this"), 
				anna_node_create_dummy(0, anna_type_wrap(type), 0), 
				anna_node_create_null(0)
			    };	    
			    anna_node_call_t *this_decl = anna_node_create_call(
				0,
				(anna_node_t *)anna_node_create_identifier(
				    0,
				    L"__var__"),
				3, param);
			    
			    anna_node_call_prepend_child(
				def_decl,
				this_decl);
			    
			    anna_node_print(decl);
			    
			}
		    }	    
		}
	    }   
	}
    }
    
}



anna_type_t *anna_type_create(wchar_t *name, anna_node_call_t *definition)
{
    anna_type_t *result = calloc(1,sizeof(anna_type_t));
    hash_init(&result->name_identifier, &hash_wcs_func, &hash_wcs_cmp);
    result->mid_identifier = anna_mid_identifier_create();
    result->name = wcsdup(name);
    result->definition = definition;
    
    if(definition)
    {
	result->body = node_cast_call(anna_node_clone_deep(definition->child[2]));
	anna_type_add_implicit_this(result);
    }
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
	anna_node_create_call(
	    0,
	    (anna_node_t *)anna_node_create_identifier(0, L"type"),
	    0,
	    0);
    
    anna_node_call_t *full_definition = 
	anna_node_create_call(
	    0,
	    (anna_node_t *)anna_node_create_identifier(0, L"__type"),
	    0,
	    0);

    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)anna_node_create_identifier(
	    0,
	    type->name));
    
    anna_node_call_add_child(
	full_definition,
	(anna_node_t *)anna_node_create_identifier(
	    0,
	    L"class"));
    
    anna_node_call_t *attribute_list = 
	anna_node_create_call(
	    0,
	    (anna_node_t *)anna_node_create_identifier(0, L"__block__"),
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
    anna_type_t *type = anna_type_create(name, 0);
    type->stack = stack;
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
//    anna_type_definition_make(type);
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
	    (anna_node_t *)anna_node_create_identifier(
		0,
		name)
	}
    ;
		       

    anna_node_call_t *attribute_list = 
        (anna_node_call_t *)type->definition->child[2];
    anna_node_call_add_child(
	attribute_list, 
	(anna_node_t *)anna_node_create_call(
	    0,
	    (anna_node_t *)anna_node_create_identifier(
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

int anna_type_is_fake(anna_type_t *t)
{
    return !wcscmp(t->name, L"!FakeFunctionType");
    

}

anna_type_t *anna_type_member_type_get(anna_type_t *type, wchar_t *name)
{

    assert(type);
    assert(name);
    
    anna_member_t *m = (anna_member_t *)hash_get(&type->name_identifier, name);
    if(!m)
    {
	return 0;
    }
    
    assert(m->type);
    
    return m->type;
}

int anna_type_member_is_method(anna_type_t *type, wchar_t *name)
{
    anna_member_t *memb = anna_type_member_info_get(type, name);
    return memb->is_method;
    
/*anna_type_t *member_type = anna_type_member_type_get(type, name);
  return !!anna_static_member_addr_get_mid(member_type, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);   */
}

mid_t anna_type_mid_at_static_offset(anna_type_t *orig, size_t off)
{
    int i;
    for(i=0; i<anna_mid_max_get(); i++)
    {
	anna_member_t *memb = orig->mid_identifier[i];
	if(!memb)
	    continue;
	
	if(memb->offset == off)
	    return i;
       
    }
    CRASH;
}

void anna_type_copy(anna_type_t *res, anna_type_t *orig)
{
    int i;
    for(i=0; i<anna_mid_max_get(); i++)
    {
       anna_member_t *memb = orig->mid_identifier[i];
       if(!memb)
           continue;
       
       anna_member_t *copy = anna_member_get(
           res,
           anna_member_create(
               res,
               anna_mid_get(memb->name), memb->name, memb->is_static, memb->type));
       copy->is_method = memb->is_method;
       copy->is_property = memb->is_property;
       copy->getter_offset = -1;
       copy->setter_offset = -1;       
    }

    for(i=0; i<anna_mid_max_get(); i++)
    {
       anna_member_t *memb = orig->mid_identifier[i];
       if(!memb)
           continue;

       anna_member_t *copy = anna_member_get(
           res,
	   anna_mid_get(memb->name));
       
       res->static_member[copy->offset]=orig->static_member[memb->offset];
    }
    
    for(i=0; i<anna_mid_max_get(); i++)
    {
       anna_member_t *memb = orig->mid_identifier[i];
       if(!memb)
           continue;
       if(!memb->is_property)
	   continue;

       anna_member_t *copy = anna_member_get(
           res,
	   anna_mid_get(memb->name));
       
       if(memb->getter_offset != -1)
       {
	   mid_t getter = anna_type_mid_at_static_offset(orig, memb->getter_offset);
	   copy->getter_offset = anna_member_get(res, getter)->offset;
       }
       if(memb->setter_offset != -1)
       {
	   mid_t setter = anna_type_mid_at_static_offset(orig, memb->setter_offset);
	   copy->setter_offset = anna_member_get(res, setter)->offset;	   
       }       
    }

}

static void anna_type_prepare_member_internal(
    anna_type_t *type,
    anna_node_declare_t *decl,
    anna_stack_frame_t *stack)
{
    if(hash_contains(
	   &type->name_identifier,
	   decl->name))
    {
	return;
    }
    
    int is_static = 0;
    int is_method = 0;
//    wprintf(L"Register %ls\n", decl->name);
    
    if(decl->value->node_type == ANNA_NODE_CLOSURE)
    {
	is_static = 1;
	is_method = 1;
    }
    
    anna_node_calculate_type(
	decl,
	stack);
    
    mid_t mid = anna_member_create(
	type,
	-1,
	decl->name,
	is_static,
	decl->return_type
	);
    
    anna_member_t *member = anna_member_get(
	type, mid);
    
    if(is_method)
    {
	anna_node_closure_t  *clo = (anna_node_closure_t *)decl->value;
	member->is_method = 1;	
	*anna_static_member_addr_get_mid(type, mid) = anna_function_wrap(clo->payload);
	anna_function_setup_interface(clo->payload, stack);
    }
}

anna_node_t *anna_type_setup_interface_internal(
    anna_type_t *type, 
    anna_stack_frame_t *parent)
{

    if( type->flags & ANNA_TYPE_PREPARED_INTERFACE)
	return 0;
    
    type->flags |= ANNA_TYPE_PREPARED_INTERFACE;

    if(type->definition)
    {
    
	CHECK_NODE_TYPE(type->definition->child[0], ANNA_NODE_IDENTIFIER);
	CHECK_NODE_BLOCK(type->definition->child[1]);
	CHECK_NODE_BLOCK(type->definition->child[2]);
	
	anna_node_call_t *attribute_list = 
	    (anna_node_call_t *)type->definition->child[2];
	
	anna_node_call_t *node = type->body;
	size_t i;

	for(i=0; i<node->child_count; i++)
	{
	    anna_node_t *decl = node->child[i];
	    if((decl->node_type != ANNA_NODE_DECLARE) && 
	       (decl->node_type != ANNA_NODE_CONST))
	    {
		anna_error(decl, L"Only declarations are allowed directly inside class definitions\n");
		continue;
	    }
	    anna_type_prepare_member_internal(
		type,
		(anna_node_declare_t *)node->child[i],
		parent);
	}
    }
    return 0;
}

void anna_type_prepare_member(anna_type_t *type, mid_t mid, anna_stack_frame_t *stack) 
{
    if(!type->definition)
    {
	return;
    }
    
    anna_node_call_t *node = type->body;
    size_t i;
    wchar_t *name = anna_mid_get_reverse(mid);
    
    for(i=0; i<node->child_count; i++)
    {
	anna_node_t *decl = node->child[i];
	if((decl->node_type != ANNA_NODE_DECLARE) && 
	   (decl->node_type != ANNA_NODE_CONST))
	{
	    continue;
	}
	anna_node_declare_t *decl2 = (anna_node_declare_t *)node->child[i];
	if( wcscmp(decl2->name, name) == 0)
	{
	    anna_type_prepare_member_internal(
		type,
		decl2,
		stack);
	}
	
    }
    
}


void anna_type_setup_interface(anna_type_t *type, anna_stack_frame_t *parent)
{
    anna_type_setup_interface_internal(type, parent);
}
