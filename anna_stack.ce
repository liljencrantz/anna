#define _GNU_SOURCE
#include <stdlib.h>     
#include <stdio.h>     
#include <GL/glew.h>	// Header File For The OpenGL32 Library
#include <string.h>
#include <assert.h>
#include <wchar.h>

#include "util.h"
#include "common.h"
#include "anna_node.h"
#include "anna_stack.h"
#include "anna_node_create.h"
#include "anna_type.h"
#include "anna_function.h"
#include "anna_util.h"
#include "anna_member.h"
#include "anna_alloc.h"
#include "anna_intern.h"
#include "anna_vm.h"

typedef struct
{
    anna_stack_template_t *stack;
    anna_type_t *type;
}
    anna_stack_prepare_data;

static void anna_stack_ensure_capacity(anna_stack_template_t *stack, size_t new_sz)
{
    if(stack->capacity < new_sz){
	size_t sz = maxi(8, maxi(new_sz, stack->capacity*2));
	stack->member_declare_node = realloc(stack->member_declare_node, sizeof(anna_node_t *)*sz);
	stack->member_flags = realloc(stack->member_flags, sizeof(int)*sz);
	stack->capacity = sz;
    }
}

anna_stack_template_t *anna_stack_create(anna_stack_template_t *parent)
{
    anna_stack_template_t *stack = anna_alloc_stack_template();
    hash_init(&stack->member_string_identifier, &hash_wcs_func, &hash_wcs_cmp);
    stack->count = 0;
    stack->capacity = 0;
    stack->parent = parent;
    al_init(&stack->import);
    al_init(&stack->expand);
    return stack;
}

void anna_stack_declare(anna_stack_template_t *stack, 
			wchar_t *name,
			anna_type_t *type, 
			anna_object_t *initial_value,
			int flags)
{
    if(!name)
	CRASH;

    if(stack->flags & ANNA_STACK_DECLARE)
    {
	return;
    }

    if(name[0]==L'!' && (wcscmp(name, L"!unused")!=0))
	return;
    
    
    if(!initial_value)
    {
	wprintf(
	    L"Critical: No initial value provided in declaration of %ls\n",
	    name);
	CRASH;
    }

//    assert(name);
    assert(type);
    assert(stack);
    //wprintf(L"Declare %ls to be of type %ls\n", name, type->name);
    
    size_t *old_offset = hash_get(&stack->member_string_identifier, name);
    if(old_offset)
    {
	wprintf(
	    L"Critical: Tried to redeclare variable %ls\n",
	    name);
	CRASH;
    }
    anna_stack_ensure_capacity(stack, stack->count+1);
    
    size_t *offset = calloc(1,sizeof(size_t));
    *offset = stack->count++;
    
    hash_put(&stack->member_string_identifier, anna_intern(name), offset);
    stack->member_flags[*offset] = flags;
    stack->member_declare_node[*offset] = 0;
    
    anna_type_t *res = anna_stack_wrap(stack)->type;    
    stack->flags |= ANNA_STACK_DECLARE;
    mid_t mid = anna_member_create(
	res,
	-1,
	name,
	1,
	type);    
    stack->flags = stack->flags & ~ANNA_STACK_DECLARE;
    *anna_entry_get_addr_static(
	res,
	mid) = (anna_entry_t *)initial_value;

}

void anna_stack_declare2(anna_stack_template_t *stack, 
			 anna_node_declare_t *declare_node)
{
    if(!declare_node->name)
	CRASH;

    assert(stack);
    //wprintf(L"Declare %ls to be of type %ls\n", name, type->name);
    
    size_t *old_offset = hash_get(&stack->member_string_identifier, declare_node->name);
    if(old_offset)
    {
	wprintf(
	    L"Critical: Tried to redeclare variable %ls\n",
	    declare_node->name);
	
	CRASH;
    }
    anna_stack_ensure_capacity(stack, stack->count+1);
    
    size_t *offset = calloc(1,sizeof(size_t));
    *offset = stack->count++;
    hash_put(&stack->member_string_identifier, wcsdup(declare_node->name), offset);
    stack->member_flags[*offset] = 0;
    stack->member_declare_node[*offset] = declare_node;

    anna_type_t *res = anna_stack_wrap(stack)->type;
    stack->flags |= ANNA_STACK_DECLARE;
    anna_member_create(
	res,
	-1,
	declare_node->name,
	1,
	0);
    stack->flags = stack->flags & ~ANNA_STACK_DECLARE;
}

anna_stack_template_t *anna_stack_template_search(
    anna_stack_template_t *stack,
    wchar_t *name)
{
    if(!stack)
    {
	wprintf(L"Critical: Null stack!\n");
	CRASH;	
    }    
  
    assert(name);
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
	if(offset) 
	{
	    return stack;
	}

	int i;
	for(i=0; i<al_get_count(&stack->import); i++)
	{
	    anna_stack_template_t *import = al_get(&stack->import, i);
	    size_t *offset = (size_t *)hash_get(&import->member_string_identifier, name);
	    if(offset) 
	    {
		return import;
	    }
	}
	stack = stack->parent;
    }
    return 0;
}

anna_object_t *anna_stack_macro_get(
    anna_stack_template_t *stack,
    wchar_t *name)
{
    if(!stack)
    {
	wprintf(L"Critical: Null stack!\n");
	CRASH;	
    }  
    assert(name);

    while(stack)
    {
	int i;
	for(i=0; i<al_get_count(&stack->expand); i++)
	{
	    anna_stack_template_t *exp = al_get(&stack->expand, i);
	    size_t *offset = (size_t *)hash_get(&exp->member_string_identifier, name);
	    
	    if(offset) 
	    {
		return anna_as_obj(
		    anna_entry_get(
			exp->wrapper, anna_mid_get(name)));
	    }
	}
	stack = stack->parent;
    }
    return 0;
}

anna_object_t *anna_stack_template_get(anna_stack_template_t *stack, wchar_t *name)
{
    size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
    if(offset) 
    {
	return anna_as_obj(
	    anna_entry_get(
		stack->wrapper, anna_mid_get(name)));
    }
    return 0;
}

void anna_stack_set(anna_stack_template_t *stack, wchar_t *name, anna_object_t *value)
{
//    wprintf(L"Set %ls to %ls\n", name, value->type->name);
    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    if(!f)
	return;

    anna_type_t *res = anna_stack_wrap(f)->type;
    *anna_entry_get_addr_static(
	res,
	anna_mid_get(name)) = (anna_entry_t *)value;
}

anna_object_t *anna_stack_get(anna_stack_template_t *stack, wchar_t *name)
{
    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    if(!f)
	return 0;
    anna_type_t *res = anna_stack_wrap(f)->type;
    return anna_as_obj(anna_entry_get_static(
	res,
	anna_mid_get(name)));
}

anna_type_t *anna_stack_get_type(anna_stack_template_t *stack, wchar_t *name)
{
    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    if(!f)
	return 0;
    anna_type_t *res = anna_stack_wrap(f)->type;
    return anna_member_get(res, anna_mid_get(name))->type;
}

int anna_stack_get_flag(anna_stack_template_t *stack, wchar_t *name)
{
    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    return f->member_flags[*(size_t *)hash_get(&f->member_string_identifier, name)];
}

void anna_stack_set_flag(anna_stack_template_t *stack, wchar_t *name, int value)
{
    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    if(!f)
	return;
    f->member_flags[*(size_t *)hash_get(&f->member_string_identifier, name)] = value;
}

void anna_stack_set_type(anna_stack_template_t *stack, wchar_t *name, anna_type_t *type){
    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    if(!f){
	return;
	debug(
	    D_CRITICAL, 
	    L"Tried to set unknown variable %ls to type %ls\n", name, type->name);
	anna_stack_print(stack);
	CRASH;
    }
    anna_type_t *res = anna_stack_wrap(f)->type;    
    anna_member_type_set(res, anna_mid_get(name), type);
}

anna_node_declare_t *anna_stack_get_declaration(
    anna_stack_template_t *stack, wchar_t *name)
{
    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    if(!f)
	return 0;
    return f->member_declare_node[*(size_t *)hash_get(&f->member_string_identifier, name)];
}

anna_sid_t anna_stack_sid_create(anna_stack_template_t *stack, wchar_t *name)
{
//    anna_stack_template_t *top = stack;
    assert(stack);
    assert(name);
    anna_sid_t sid = {0,0};
    
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
	if(offset) 
	{
	  sid.offset = *offset;
	  return sid;
	}
	sid.frame++;
	stack = stack->parent;	
    }
    sid.frame=-1;
    sid.offset=-1;
    return sid;
    /*
      wprintf(L"Critical: Tried to create sid for unknown variable: %ls\n", name);
      CRASH;
    */
}

static void anna_print_stack_member(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    wchar_t *name = (wchar_t *)key_ptr;
    anna_stack_template_t *stack = (anna_stack_template_t *)aux_ptr;
    anna_type_t *res = anna_stack_wrap(stack)->type;
    anna_type_t *type = anna_member_get(res, anna_mid_get(name))->type;
    wprintf(L"%ls %ls = %ls\n", type?type->name:L"<UNKNOWN>", name, L"...");
}

void anna_stack_print(anna_stack_template_t *stack)
{
    //CRASH;
    
    if(!stack)
	return;

    wprintf(
	L"Stack frame with %d members belonging to function %ls:\n",
	stack->count,
	stack->function?stack->function->name:L"<null>");
    hash_foreach2(&stack->member_string_identifier, &anna_print_stack_member, stack);
    
    anna_stack_print(stack->parent);
}

int anna_stack_depth(anna_stack_template_t *stack)
{
    return stack?anna_stack_depth(stack->parent)+1:0;
}

void anna_stack_print_trace(anna_stack_template_t *stack)
{
    wprintf(L"Stack trace:\n");
    while(stack)
    {
	wprintf(
	    L"Stack frame belonging to function %ls\n",
	    stack->function?stack->function->name:L"<null>");
	stack = stack->parent;
    }
}

static void anna_stack_save_name(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    array_list_t *al = (array_list_t *)aux_ptr;
    wchar_t *name = (wchar_t *)key_ptr;
    al_push(al, name);
}

static anna_type_t *anna_stack_type_create(anna_stack_template_t *stack)
{
    anna_type_t *res = anna_type_stack_create(
	anna_util_identifier_generate(
	    L"StackType",
	    stack->function?&(stack->function->definition->location):0),
	stack);
    anna_member_create(
	res, ANNA_MID_STACK_PAYLOAD,  L"!stackPayload", 
	0, null_type);
    anna_member_create(
	res,
	ANNA_MID_STACK_TYPE_PAYLOAD,
	L"!stackTypePayload",
	1,
	null_type);
    *(anna_entry_get_addr_static(res, ANNA_MID_STACK_TYPE_PAYLOAD)) = (anna_entry_t *)stack;
    return res;
}

anna_object_t *anna_stack_wrap(anna_stack_template_t *stack)
{
    if(!stack->wrapper)
    {
	anna_type_t *t = anna_stack_type_create(stack);
	stack->wrapper = anna_object_create(t);
	stack->wrapper->member[0] = (anna_entry_t *)stack;
    }
    return stack->wrapper;
}

anna_stack_template_t *anna_stack_unwrap(anna_object_t *wrapper)
{
    return *(anna_stack_template_t **)anna_entry_get_addr(wrapper, ANNA_MID_STACK_PAYLOAD);
}

