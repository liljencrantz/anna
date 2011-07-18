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
#include "anna_function.h"
#include "anna_member.h"
#include "anna.h"
#include "anna_alloc.h"
#include "anna_node_check.h"
#include "anna_intern.h"
#include "anna_attribute.h"
#include "anna_vm.h"
#include "anna_node_hash.h"
#include "anna_node_check.h"
#include "clib/anna_pair.h"
#include "clib/anna_list.h"
#include "clib/anna_hash.h"
#include "clib/anna_function_type.h"
#include "anna_mid.h"

static array_list_t  anna_type_list = AL_STATIC;
static int anna_type_object_created = 0;
static array_list_t anna_type_uninherited = AL_STATIC;
static hash_table_t anna_type_for_function_identifier;

static void anna_type_mark_static_iter(void *key_ptr,void *val_ptr)
{
    anna_alloc_mark_type(val_ptr);
}

void anna_type_mark_static()
{
    hash_foreach(
	&anna_type_for_function_identifier,
	anna_type_mark_static_iter);
}

void anna_type_copy_object(anna_type_t *type)
{
    if(anna_type_object_created)
    {
	anna_type_copy(type, object_type);
    }
    else 
    {
	al_push(&anna_type_uninherited, type);
    }
}

void anna_type_object_is_created()
{
    anna_type_object_created = 1;
    int i;
    for(i=0; i<al_get_count(&anna_type_uninherited); i++)
    {
	anna_type_t *t = al_get(&anna_type_uninherited, i);
	anna_type_copy(t, object_type);
    }
    al_destroy(&anna_type_uninherited);
}

void anna_type_reallocade_mid_lookup(size_t old_sz, size_t sz)
{
    int i;
    
    for(i=0;i<al_get_count(&anna_type_list); i++)
    {
	anna_type_t *type = (anna_type_t *)al_get(&anna_type_list, i);
	type->mid_identifier = realloc(type->mid_identifier, sz*sizeof(anna_member_t *));
	if(type == null_type)
	{
	    int j;
	    for(j=old_sz; j<sz; j++)
	    {
		type->mid_identifier[j] = type->mid_identifier[0];
//		wprintf(L"Setting mid %d of null type to point to null member %d\n", j, type->mid_identifier[0]);
	    }
	}
	else
	{
	    memset(&type->mid_identifier[old_sz], 0, (sz-old_sz)*sizeof(anna_member_t *));
	}
    }
}

/**
   FIXME: Very ugly method. It mangles functions into methods and does
   a bunch of related stuff. But it's a bit of an unreadable mess...
 */
static void anna_type_mangle_methods(
    anna_type_t *type)
{
    size_t i;
    anna_node_call_t *body= type->body;
//    wprintf(L"Mangle methods in %ls\n", type->name);
    
    for(i=0; i<body->child_count; i++)
    {
	if(anna_node_is_call_to(body->child[i], L"__const__"))
	{
	    anna_node_call_t *decl =(anna_node_call_t *)body->child[i];
	    if(decl->child_count >= 3)
	    {
		if(anna_node_is_call_to(decl->child[2], L"__def__"))
		{
		    anna_node_call_t *def =(anna_node_call_t *)decl->child[2];
		    if(def->child_count >= 5)
		    {
			//anna_node_identifier_t *name = (anna_node_identifier_t *)def->child[0];
			if(anna_node_is_named(def->child[0], L"__init__"))
			{
			    if(anna_node_is_call_to(def->child[4], L"__block__"))
			    {
				//wprintf(L"Found init of type %ls\n", type->name);
				anna_node_call_t *body = (anna_node_call_t *)def->child[4];
				anna_node_call_add_child(
				    body, (anna_node_t *)anna_node_create_identifier(0, L"this"));				
			    }		    
			}
			
			if(anna_node_is_call_to(def->child[2], L"__block__") && 
			   anna_node_is_call_to(def->child[3], L"__block__"))
			{

			    //wprintf(L"Add this-argument to method %ls in %ls\n", name->name, type->name);
			    anna_node_call_t *def_decl =(anna_node_call_t *)def->child[2];
			    anna_node_call_t *attr =(anna_node_call_t *)def->child[3];
			    if(!anna_attribute_flag(attr, L"static"))
			    {
				anna_node_call_t *this_decl = anna_node_create_call2(
				    0,
				    anna_node_create_identifier(0,L"__var__"),
				    anna_node_create_identifier(0, L"this"), 
				    anna_node_create_dummy(0, anna_type_wrap(type)), 
				    anna_node_create_null(0),
				    anna_node_create_block2(0)
				    );
				
				anna_node_call_prepend_child(
				    def_decl,
				    (anna_node_t *)this_decl);
				anna_node_call_prepend_child(
				    (anna_node_call_t *)def->child[4],
				    (anna_node_t *)anna_node_create_call2(
					0,
					anna_node_create_identifier(0,L"use"),
					anna_node_create_identifier(0, L"this")));
				
			    }
			}
		    }	    
		}
	    }   
	}
    }
}

static anna_node_t *anna_node_specialize(anna_node_t *code, array_list_t *spec)
{
    int i;
    for(i=0; i<al_get_count(spec); i++)
    {
	anna_node_t *node = (anna_node_t *)al_get(spec, i);
	CHECK_NODE_TYPE(node, ANNA_NODE_CALL);
	anna_node_call_t *call = (anna_node_call_t *)node;
	CHECK_CHILD_COUNT(call, L"Template specialization", 2);
	CHECK_NODE_TYPE(call->child[0], ANNA_NODE_INTERNAL_IDENTIFIER);
	code = anna_node_replace(code, (anna_node_identifier_t *)call->child[0], call->child[1]);
    }
    
    return code;    
}


static anna_type_t *anna_type_create_internal(wchar_t *name, anna_node_call_t *definition)
{
    anna_type_t *result = anna_alloc_type();
    hash_init(&result->name_identifier, &hash_wcs_func, &hash_wcs_cmp);
    result->mid_identifier = anna_mid_identifier_create();
    result->name = anna_intern(name);
    result->definition = definition;
    
    if(definition)
    {
	//anna_node_print(D_CRITICAL, definition->child[2]);
	array_list_t al = AL_STATIC;
	
	result->attribute = node_cast_call(definition->child[1]);
	anna_attribute_call_all(result->attribute, L"template", &al);
	result->body = node_cast_call(
	    anna_node_specialize(
		anna_node_clone_deep(definition->child[2]),
		&al));
	
	anna_type_mangle_methods(result);
    }
    al_push(&anna_type_list, result);
    hash_init(&result->specializations, anna_node_hash_func, anna_node_hash_cmp);

    anna_type_calculate_size(result);
    return result;
}

anna_type_t *anna_type_create(wchar_t *name, anna_node_call_t *definition)
{
    anna_type_t *result = anna_type_create_internal(name, definition);
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

anna_type_t *anna_type_native_create(wchar_t *name, anna_stack_template_t *stack)
{
    anna_type_t *result = anna_type_create_internal(name, 0);    
    return result;
}

anna_type_t *anna_type_stack_create(wchar_t *name, anna_stack_template_t *stack)
{    
    anna_type_t *result = anna_type_create_internal(name, 0);
    result->stack = stack;
    return result;
}

static void add_member(void *key, void *value, void *aux)
{
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
    wchar_t **members = calloc(sizeof(wchar_t *), hash_get_count(&type->name_identifier));
    anna_type_get_member_names(type, members);    
    for(i=0; i< hash_get_count(&type->name_identifier); i++)
    {
	assert(members[i]);
	anna_member_t *member = anna_type_member_info_get(type, members[i]);
	assert(member);
	if(member->is_property)
	{
	    wprintf(
		L"\tproperty %ls %ls setter: %d, getter: %d\n",
		member->type->name, members[i], 
		member->setter_offset,
		member->getter_offset);
	}
	else if(member->is_bound_method)
	{
	    wprintf(
		L"\tmethod %ls: type: %ls, static: %ls, property: %ls, offset: %d\n",
		members[i], member->type->name, 
		member->is_static?L"true":L"false",
		member->is_property?L"true":L"false",
		member->offset);
	}
	else
	{
	    wprintf(
		L"\tvar %ls %ls, static: %ls, offset: %d\n",
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

anna_object_t *anna_type_wrap(anna_type_t *result)
{
    if(likely((long)result->wrapper))
	return result->wrapper;
    
    result->wrapper = anna_object_create(type_type);
    memcpy(anna_entry_get_addr(result->wrapper, ANNA_MID_TYPE_WRAPPER_PAYLOAD), &result, sizeof(anna_type_t *));  
    return result->wrapper;
}

anna_type_t *anna_type_unwrap(anna_object_t *wrapper)
{
    anna_type_t **tmp = (anna_type_t **)anna_entry_get_addr(wrapper, ANNA_MID_TYPE_WRAPPER_PAYLOAD);
    return tmp?*tmp:0;
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
	type->static_member_blob = realloc(
	    type->static_member_blob, 
	    new_sz * sizeof(int));
	int i;
	for(i=type->static_member_count; i < new_sz; i++)
	{
	    type->static_member_blob[i] = 69;
	}

	if(!type->static_member)
	{
	    wprintf(L"Out of memory");
	    CRASH;
	}
	type->static_member_capacity = new_sz;
    }
    return type->static_member_count++;    
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
    return memb->is_bound_method;
    
/*anna_type_t *member_type = anna_type_member_type_get(type, name);
  return !!anna_entry_get_addr_static(member_type, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);   */
}

static mid_t anna_type_mid_at_static_offset(anna_type_t *orig, size_t off)
{
    int i;
    for(i=0; i<anna_mid_max_get(); i++)
    {
	anna_member_t *memb = orig->mid_identifier[i];
	if(!memb || !memb->is_static)
	    continue;
	
	if(memb->offset == off)
	    return i;
       
    }
    CRASH;
}

void anna_type_copy(anna_type_t *res, anna_type_t *orig)
{
    int i;

    if(orig == object_type && !anna_type_object_created)
    {
	anna_type_copy_object(res);
    }
    //wprintf(L"Copy type %ls into type %ls\n", orig->name, res->name);
    //anna_type_print(res);
    
    /*
      First copy all members that have a previously unused mid, making
      note of which members already existed
     */
    int steps = anna_mid_max_get();
    int *copied = calloc(sizeof(int), steps);
    
    for(i=0; i<steps; i++)
    {
       anna_member_t *memb = orig->mid_identifier[i];
       if(!memb)
           continue;
       
       if(res->mid_identifier[i])
	  continue;
       
       copied[i] = 1;
       
       int storage = (memb->is_static?ANNA_MEMBER_STATIC:0)|((memb->offset==-1)?ANNA_MEMBER_VIRTUAL:0);
       
       anna_member_t *copy = anna_member_get(
           res,
           anna_member_create(
               res,
               anna_mid_get(memb->name), 
	       storage, memb->type));
       copy->is_bound_method = memb->is_bound_method;
       copy->is_property = memb->is_property;
       copy->getter_offset = -1;
       copy->setter_offset = -1;
       if(memb->is_static)
       {
	   assert(res->static_member);
	   assert(orig->static_member);       
	   res->static_member[copy->offset] = orig->static_member[memb->offset];
       }
    }
    
    /*
      Then, for every copied static member with storage, copy over the initial value
    */
    for(i=0; i<steps; i++)
    {
	if(!copied[i])
	    continue;
	
       anna_member_t *memb = orig->mid_identifier[i];
       anna_member_t *copy = anna_member_get(
           res,
	   anna_mid_get(memb->name));
       if(memb->is_static && memb->offset != -1)
	   res->static_member[copy->offset]=orig->static_member[memb->offset];
    }
    
    /*
      Then, for every copied property, find the offset of the getter and setter
    */

    for(i=0; i<steps; i++)
    {
	if(!copied[i])
	    continue;

       anna_member_t *memb = orig->mid_identifier[i];

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

    free(copied);
}

static void anna_type_prepare_member_internal(
    anna_type_t *type,
    anna_node_declare_t *decl,
    anna_stack_template_t *stack)
{
    if(hash_contains(
	   &type->name_identifier,
	   decl->name))
    {
//	wprintf(L"Skip %ls\n", decl->name);
	return;
    }
    
    int is_static = 0;
    int is_method = 0;
    int is_bound = 0;
//    wprintf(L"Register %ls\n", decl->name);
    
    if(decl->value->node_type == ANNA_NODE_CLOSURE)
    {
	is_static = 1;
	is_method = 1;
	is_bound = !anna_attribute_flag(decl->attribute, L"static");
    }
    
    anna_node_calculate_type(
	(anna_node_t *)decl);
    
    if(!is_method)
    {
	is_static = anna_attribute_flag(decl->attribute, L"static");
	
	array_list_t etter = AL_STATIC;
	anna_attribute_call_all(decl->attribute, L"property", &etter);
	
	if(al_get_count(&etter))
	{
	    al_destroy(&etter);
	    return;
	}
    }
        
    mid_t mid = anna_member_create(
	type,
	anna_mid_get(decl->name),
	is_static,
	decl->return_type
	);
    
    anna_member_t *member = anna_member_get(
	type, mid);
    member->attribute = (anna_node_call_t *)anna_node_clone_deep((anna_node_t *)decl->attribute);
    
    if(is_method)
    {
	anna_node_closure_t  *clo = (anna_node_closure_t *)decl->value;
	member->is_bound_method = is_bound;
	*anna_entry_get_addr_static(type, mid) = anna_from_obj(anna_function_wrap(clo->payload));
	anna_function_set_stack(clo->payload, stack);
	anna_function_setup_interface(clo->payload);
	//anna_function_setup_body(clo->payload);
    }
    if(is_static)
    {
	anna_entry_t *value = anna_node_static_invoke(
	    decl->value, decl->stack);
	type->static_member[member->offset] = value;
    }
    
}


static void anna_type_prepare_property(
    anna_type_t *type,
    anna_node_declare_t *decl,
    anna_stack_template_t *stack)
{
    if(hash_contains(
	   &type->name_identifier,
	   decl->name))
    {
	return;
    }
    
    if(decl->value->node_type == ANNA_NODE_CLOSURE)
    {
	return;
    }
    
    array_list_t etter = AL_STATIC;
    anna_attribute_call_all(decl->attribute, L"property", &etter);
    if(al_get_count(&etter)>2)
    {
	anna_error((anna_node_t *)decl, L"Invalid property");
	goto END;
    }
	
    if(al_get_count(&etter))
    {
	int has_setter = al_get_count(&etter) == 2;
	anna_node_t *g_node = (anna_node_t *)al_get(&etter, 0);
	wchar_t *getter=0, *setter=0;
	ssize_t getter_offset=-1, setter_offset=-1;

	if(g_node->node_type != ANNA_NODE_NULL)
	{
	    if(g_node->node_type != ANNA_NODE_IDENTIFIER)
	    {
		anna_error(g_node, L"Invalid getter");
		goto END;
	    }
	    getter = ((anna_node_identifier_t *)g_node)->name;
	    
	    anna_member_t *g_memb = anna_member_get(type, anna_mid_get(getter));
	    if(!g_memb)
	    {
		anna_error(g_node, L"Unknown method");
		goto END;
	    }
	    getter_offset = g_memb->offset;
	}
	
	if(has_setter)
	{
	    anna_node_t *s_node = al_get(&etter, 1);
	    if(s_node->node_type != ANNA_NODE_IDENTIFIER)
	    {
		anna_error(s_node, L"Invalid setter");
		goto END;
	    }
	    setter = ((anna_node_identifier_t *)s_node)->name;

	    anna_member_t *s_memb = anna_member_get(type, anna_mid_get(setter));
	    if(!s_memb)
	    {
		anna_error(s_node, L"Unknown method");
		goto END;
	    }
	    setter_offset = s_memb->offset;
	}

	anna_member_create_property(
	    type, anna_mid_get(decl->name),
	    decl->return_type, getter_offset, setter_offset);
    }

  END:
    al_destroy(&etter);

}

ANNA_VM_NATIVE(anna_type_noop, 1)
{
    return param[0];
}

static void anna_type_extend(
    anna_type_t *type)
{
    array_list_t parents=AL_STATIC;
    anna_attribute_call_all(type->attribute, L"extends", &parents);
    
    int i;
    for(i=al_get_count(&parents)-1; i>=0; i--)
    {
	anna_node_t *c = (anna_node_t *)al_get(&parents, i);
	anna_node_calculate_type(c);
	if(c->return_type != type_type)
	{
	    anna_error(c, L"Invalid parent type");
	    continue;
	}	
	anna_type_t *par = anna_node_resolve_to_type(c, type->stack);
	if(!par)
	{
	    anna_error(c, L"Could not find specified type");
	    continue;
	}
	anna_type_setup_interface(par);
	anna_type_copy(type, par);
		
    }
    anna_type_copy(type, object_type);
}

void anna_type_set_stack(
    anna_type_t *t,
    anna_stack_template_t *parent_stack)
{
    if(t->body && t->body->stack)
	return;
    
    t->stack = parent_stack;
    
    if(t->body)
    {
	anna_node_set_stack(
	    (anna_node_t *)t->body,
	    t->stack);
	anna_node_resolve_identifiers((anna_node_t *)t->body);
	anna_node_set_stack(
	    (anna_node_t *)t->attribute,
	    t->stack);
    }
}

static anna_node_t *anna_type_setup_interface_internal(
    anna_type_t *type)
{

    if( type->flags & ANNA_TYPE_PREPARED_INTERFACE)
	return 0;
    
    type->flags |= ANNA_TYPE_PREPARED_INTERFACE;

    //wprintf(L"Set up interface for type %ls\n", type->name);
    //anna_node_print(4, type->definition);    

    if(type->definition)
    {
	
	if(type->definition->child[0]->node_type != ANNA_NODE_IDENTIFIER)
	{
	    CRASH;
	}	

	CHECK_NODE_TYPE(type->definition->child[0], ANNA_NODE_IDENTIFIER);
	CHECK_NODE_BLOCK(type->definition->child[1]);
	CHECK_NODE_BLOCK(type->definition->child[2]);
	
//	anna_node_call_t *attribute_list = 
//	    (anna_node_call_t *)type->definition->child[2];
	
	anna_node_call_t *node = type->body;
	size_t i;

	for(i=0; i<node->child_count; i++)
	{
	    anna_node_t *decl = node->child[i];
	    if((decl->node_type != ANNA_NODE_DECLARE) && 
	       (decl->node_type != ANNA_NODE_CONST))
	    {
		anna_error(
		    decl,
		    L"Only declarations are allowed directly inside class definitions\n");
		continue;
	    }
	    anna_type_prepare_member_internal(
		type,
		(anna_node_declare_t *)node->child[i],
		type->stack);
	}
	for(i=0; i<node->child_count; i++)
	{
	    //anna_node_t *decl = node->child[i];
	    anna_type_prepare_property(
		type,
		(anna_node_declare_t *)node->child[i],
		type->stack);
	}
    }
    anna_type_extend(type);    
    int i;
    for(i=0; i<anna_mid_max_get(); i++)
    {
	anna_member_t *memb = type->mid_identifier[i];
	if(memb && memb->is_static && memb->type != null_type)
	{
	    anna_entry_t *val = *anna_entry_get_addr_static(type, i);
	    if(!anna_entry_null(val))
	    {
		anna_function_t *fun = anna_function_unwrap(anna_as_obj(val));
		if(fun)
		{
		    anna_function_setup_body(fun);
		}
	    }	    
	}
    }
    

    if(!anna_member_get(type, anna_mid_get(L"__init__"))){

//	    wprintf(L"Internal noop init in %ls\n", type->name);
	wchar_t *argn[] = 
	    {
		L"this"
	    }
	;
	anna_type_t *argv[] = 
	    {
		type,
	    }
	;
	
	anna_member_create_native_method(
	    type, anna_mid_get(L"__init__"), 0,
	    &anna_type_noop, type, 1, argv, argn);
/*
  anna_object_t **cp = anna_entry_get_addr_static(
  type,
  ANNA_MID_INIT_PAYLOAD);
*/
    }

    return 0;
}

void anna_type_prepare_member(anna_type_t *type, mid_t mid, anna_stack_template_t *stack) 
{
    if(!type->definition)
    {
	return;
    }
    
    anna_node_call_t *node = type->body;
    size_t i;
    wchar_t *name = anna_mid_get_reverse(mid);

    type->stack = stack;
    
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
		type->stack);
	}
    }    
}

void anna_type_setup_interface(anna_type_t *type)
{
    anna_type_setup_interface_internal(type);
}

anna_type_t *anna_type_specialize(anna_type_t *type, anna_node_call_t *spec)
{
    anna_node_call_t *def = (anna_node_call_t *)anna_node_clone_deep((anna_node_t *)type->definition);
    anna_node_call_t *attr = node_cast_call(def->child[1]);
    int i;

    array_list_t al = AL_STATIC;
    anna_attribute_call_all(attr, L"template", &al);
    
    for(i=0; i<attr->child_count;i++)
    {
	anna_node_call_t *tm = node_cast_call((anna_node_t *)al_get(&al, i));
	assert(tm->child_count == 2);
	tm->child[1] = spec->child[i];
    }
//    anna_node_print(4, def);
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%ls«...»", type->name);
    anna_type_t *res = anna_type_create(sb_content(&sb), def);
    sb_destroy(&sb);
    
    anna_type_macro_expand(res, type->stack_macro);
    res->flags |= ANNA_TYPE_SPECIALIZED;
    return res;
}

static int attr_idx(anna_node_call_t *attr, wchar_t *name)
{
    int i;
//    anna_node_print(5, attr);
    int idx=0;
    for(i=0; i<attr->child_count; i++)
    {
	if (anna_node_is_call_to(attr->child[i], L"template"))
	{
	    anna_node_call_t *tmpl = (anna_node_call_t *)attr->child[i];
	    if(tmpl->child_count == 1)
	    {
		if (anna_node_is_call_to(tmpl->child[i], L"__mapping__"))
		{
		    anna_node_call_t *pair = (anna_node_call_t *)tmpl->child[0];
		    if(pair->child_count == 2)
		    {
			if( anna_node_is_named(pair->child[0], name))
			{
			    return idx;
			}
		    }
		}
	    }
	    idx++;
	}
    }
    return -1;
}

static anna_node_call_t *get_constructor_input_list(anna_node_call_t *def)
{
    int i;
    if(def->child[2]->node_type == ANNA_NODE_CALL)
    {
	anna_node_call_t *body = (anna_node_call_t *)def->child[2];
	
	for(i=0; i<body->child_count; i++)
	{
	    if(anna_node_is_call_to(body->child[i], L"__const__"))
	    {
		anna_node_call_t *decl = (anna_node_call_t *)body->child[i];
		if(decl->child_count == 4 && anna_node_is_named(decl->child[0], L"__init__"))
		{
		    if(anna_node_is_call_to(decl->child[2], L"__def__"))
		    {
			anna_node_call_t *meth = (anna_node_call_t *)decl->child[2];
			if(meth->child_count == 5)
			    return (anna_node_call_t *)meth->child[2];
		    }
		    
		}
		
	    }
	    
	}
    }
    
    return 0;
    
}


anna_type_t *anna_type_implicit_specialize(anna_type_t *type, anna_node_call_t *call)
{
    if((call->child_count < 1) || (type->flags & ANNA_TYPE_SPECIALIZED))
    {
	return type;
    }

    if(type == pair_type)
    {
	if(call->child_count != 2)
	{
	    return type;
	}
	
	return anna_pair_type_get(
	    call->child[0]->return_type, 
	    call->child[1]->return_type);	
    }
    else if(type == any_list_type)
    {
	anna_type_t *tt = call->child[0]->return_type;
	int i;
	for(i=1; i<call->child_count; i++)
	{
	    tt = anna_type_intersect(tt, call->child[i]->return_type);
	}
	
	return anna_list_type_get_any(
	    tt);
    }
    else if(type == mutable_list_type)
    {
	anna_type_t *tt = call->child[0]->return_type;
	int i;
	for(i=1; i<call->child_count; i++)
	{
	    tt = anna_type_intersect(tt, call->child[i]->return_type);
	}
	
	return anna_list_type_get_mutable(
	    tt);
    }
    else if(type == imutable_list_type)
    {
	anna_type_t *tt = call->child[0]->return_type;
	int i;
	for(i=1; i<call->child_count; i++)
	{
	    tt = anna_type_intersect(tt, call->child[i]->return_type);
	}
	
	return anna_list_type_get_imutable(
	    tt);
    }
    else if(type == hash_type)
    {
	anna_type_t *arg_type = call->child[0]->return_type;
	anna_type_t *spec1 = 
	    (anna_type_t *)*anna_entry_get_addr_static(
		arg_type, ANNA_MID_PAIR_SPECIALIZATION1);
	anna_type_t *spec2 =
	    (anna_type_t *)*anna_entry_get_addr_static(
		arg_type, ANNA_MID_PAIR_SPECIALIZATION2);
	int i;
	for(i=1; i<call->child_count; i++)
	{
	    arg_type = call->child[i]->return_type;
	    spec1 = anna_type_intersect(
		spec1, 
		(anna_type_t *)*anna_entry_get_addr_static(
		    arg_type, ANNA_MID_PAIR_SPECIALIZATION1));
	    spec2 = anna_type_intersect(
		spec2, 
		(anna_type_t *)*anna_entry_get_addr_static(
		    arg_type, ANNA_MID_PAIR_SPECIALIZATION2));
	}
	
	if(spec1 && spec2)
	{	    
	    return anna_hash_type_get(
		spec1, 
		spec2);
	}
	return type;
    }
    else if(!type->definition)
    {
	return type;
    }
    
    anna_node_call_t *def = (anna_node_call_t *)
	anna_node_clone_deep((anna_node_t *)type->definition);
    anna_node_call_t *attr = node_cast_call(def->child[1]);
    
    array_list_t al = AL_STATIC;
    anna_attribute_call_all(attr, L"template", &al);
    
    if(al_get_count(&al) == 0)    
    {
	return type;
    }    
    
    int i;
    
    anna_object_t *constructor_obj = anna_as_obj_fast(
	anna_entry_get_static(
	    type,
	    ANNA_MID_INIT_PAYLOAD));
    anna_function_t *constr = anna_function_unwrap(constructor_obj);

    if(call->child_count > constr->input_count)
    {
	return type;
    }
    
//    wprintf(L"Looking ok for implicit spec\n");
    
    anna_node_call_t *input_node = get_constructor_input_list(type->definition);
    anna_type_t **type_spec = calloc(sizeof(anna_type_t *), attr->child_count);
    int spec_count=0;
    if(input_node)
    {
	for(i=0; i<input_node->child_count; i++)
	{	
	anna_node_call_t *decl = node_cast_call(input_node->child[i]);
//	anna_node_print(4, decl);
	if(decl->child[1]->node_type == ANNA_NODE_INTERNAL_IDENTIFIER)
	{
	    anna_node_identifier_t *id =(anna_node_identifier_t *)decl->child[1];

//	    wprintf(L"Check if %ls is a template param\n", id->name);
	    int templ_idx = attr_idx(attr, id->name);
	    if(templ_idx >= 0)
	    {
		if(!type_spec[templ_idx])
		{
		    type_spec[templ_idx] = call->child[i]->return_type;
		    spec_count++;
		}
		else
		{
		    type_spec[templ_idx] = anna_type_intersect(type_spec[templ_idx], call->child[i]->return_type);
		}
	    }
	}
    }
    }
    

    if(spec_count ==attr->child_count)
    {
	anna_node_call_t *spec_call = anna_node_create_block2(0);
	for(i=0; i<attr->child_count; i++)
	{
	    anna_node_call_add_child(
		spec_call, 
		(anna_node_t *)anna_node_create_dummy(
		    0,
		    anna_type_wrap(type_spec[i])));
	}
	type = anna_type_specialize(type, spec_call);
    }
    
    free(type_spec);
    
    return type;
}

void anna_type_macro_expand(anna_type_t *f, anna_stack_template_t *stack)
{
    f->stack_macro = stack;
    
    if(f->definition)
    {
	anna_node_call_t *body = f->body;
	
	int i;
	for(i=0;i<body->child_count; i++)
	    body->child[i] = anna_node_macro_expand(body->child[i], stack);
    }
}

void anna_type_calculate_size(anna_type_t *this)
{
    this->object_size = 
	anna_align(
	    sizeof(anna_object_t)+sizeof(anna_object_t *)*this->member_count);
}

static int hash_function_type_func(void *a)
{
    anna_function_type_t *key = (anna_function_type_t *)a;
    int res = (int)(long)key->return_type ^ key->flags;
    size_t i;
    
    for(i=0;i<key->input_count; i++)
    {
	res = (res<<19) ^ (int)(long)key->input_type[i] ^ (res>>13);
	res ^= wcslen(key->input_name[i]);
    }
    
    return res;
}

static int hash_function_type_comp(void *a, void *b)
{
    size_t i;
    
    anna_function_type_t *key1 = (anna_function_type_t *)a;
    anna_function_type_t *key2 = (anna_function_type_t *)b;

    //debug(D_SPAM,L"Compare type for function %ls %d and %ls %d\n", key1->result->name, hash_function_type_func(key1), key2->result->name, hash_function_type_func(key2));

    if(key1->return_type != key2->return_type)
	return 0;
    if(key1->input_count != key2->input_count)
	return 0;
    if(key1->flags != key2->flags)
	return 0;

    for(i=0;i<key1->input_count; i++)
    {
	if(key1->input_type[i] != key2->input_type[i])
	    return 0;
	if(wcscmp(key1->input_name[i], key2->input_name[i]) != 0)
	    return 0;
	if(key1->input_default[i])
	{
	    if(key2->input_default[i])
	    {
		if(anna_node_compare(key1->input_default[i], key2->input_default[i]))
		{
		    return 0;
		}
	    }
	    else
	    {
		return 0;
	    }
	}
	else if(key2->input_default[i])
	{
	    return 0;
	}
    }
    //debug(D_SPAM,L"Same!\n");
    
    return 1;
}

void anna_type_init()
{
    hash_init(
	&anna_type_for_function_identifier,
	&hash_function_type_func,
	&hash_function_type_comp);
}

anna_type_t *anna_type_for_function(
    anna_type_t *result, 
    size_t argc, 
    anna_type_t **argv, 
    wchar_t **argn, 
    anna_node_t **argd, 
    int flags)
{

    //static int count=0;
    //if((count++)==10) {CRASH};
    flags = flags & (ANNA_FUNCTION_VARIADIC | ANNA_FUNCTION_MACRO | ANNA_FUNCTION_CONTINUATION | ANNA_FUNCTION_BOUND_METHOD);
    size_t i;
    static anna_function_type_t *key = 0;
    static size_t key_sz = 0;
    size_t new_key_sz = sizeof(anna_function_type_t) + sizeof(anna_type_t *)*argc;
    
    if(!result)
    {
	debug(D_CRITICAL,
	    L"Critical: Function lacks return type!\n");
	CRASH;
    }
    
    if(argc)
	assert(argv);
    
    if(new_key_sz>key_sz)
    {
	int was_null = (key==0);
	key = realloc(key, new_key_sz);
	key_sz = new_key_sz;
	key->input_name = was_null?malloc(sizeof(wchar_t *)*argc):realloc(key->input_name, sizeof(wchar_t *)*argc);
	key->input_default = was_null?malloc(sizeof(anna_node_t *)*argc):realloc(key->input_default, sizeof(anna_node_t *)*argc);
    }
    
    key->flags = flags;
    key->return_type=result;
    key->input_count = argc;
    
    for(i=0; i<argc;i++)
    {
//	wprintf(L"%d %d %d\n", argv, argv[i], argv[i]);
	
	if(argv[i] && wcscmp(argv[i]->name, L"!FakeFunctionType")==0)
	{
	    debug(D_CRITICAL,
		L"Critical: Tried to get a function key for function with uninitialized argument types\n");
	    CRASH;
	}
	
	key->input_type[i]=argv[i];
	key->input_name[i]=argn[i];
	key->input_default[i]=argd ? argd[i] : 0;
    }

    anna_type_t *res = hash_get(&anna_type_for_function_identifier, key);
    if(!res)
    {
	anna_function_type_t *new_key = malloc(new_key_sz);
	memcpy(new_key, key, new_key_sz);	
	new_key->input_name = malloc(sizeof(wchar_t *)*argc);
	new_key->input_default = malloc(sizeof(anna_node_t *)*argc);
	for(i=0; i<argc;i++)
	{
	    new_key->input_name[i]=wcsdup(argn[i]);
	    new_key->input_default[i]= (argd && argd[i]) ? anna_node_clone_deep(argd[i]) : 0;
	}
	static int num=0;
	
	string_buffer_t sb;
	sb_init(&sb);
	wchar_t *fn = L"def";
	
	if(flags & ANNA_FUNCTION_MACRO)
	{ 
	    fn = L"macro";
	}
	else if(flags&ANNA_FUNCTION_CONTINUATION)
	{
	    fn = L"continuation";
	}
	
	sb_printf(&sb, L"!%ls %ls (", fn, result->name);
	for(i=0; i<argc;i++)
	{
	    wchar_t *dots = (i==argc-1) && (flags & ANNA_FUNCTION_VARIADIC)?L"...":L"";
	    sb_printf(&sb, L"%ls%ls %ls%ls", i==0?L"":L", ", argv[i]->name, dots, argn[i]);
	}
	sb_printf(&sb, L")%d", num++);
	
	res = anna_type_native_create(sb_content(&sb), stack_global);
	sb_destroy(&sb);
	hash_put(&anna_type_for_function_identifier, new_key, res);
	anna_function_type_create(new_key, res);
    }
    
    anna_function_type_t *ggg = anna_function_type_unwrap(res);
    assert(ggg->input_count == argc);
    
    return res;
}

void anna_type_document(anna_type_t *type, wchar_t *doc)
{
    anna_node_call_t *attr = anna_node_create_call2(
	0,
	anna_node_create_identifier(0, L"documentation"),
	anna_node_create_string_literal(0, wcslen(doc), doc));
    
    if(!type->attribute)
    {
	type->attribute = anna_node_create_call2(
	    0,
	    anna_node_create_identifier(0, L"__call__"),
	    attr);
    }
    else
    {
	anna_node_call_add_child(type->attribute, (anna_node_t *)attr);    
    }
}

int anna_type_mid_internal(mid_t mid)
{
    return 
	(mid == anna_mid_get(L"__name__"))||
	(mid == anna_mid_get(L"__member__"))||
	(mid == anna_mid_get(L"__abides__"))||
	(mid == anna_mid_get(L"__attribute__"));
}