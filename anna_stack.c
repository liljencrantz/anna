#include <stdlib.h>     
#include <stdio.h>     
#include <GL/glew.h>	// Header File For The OpenGL32 Library
#include <string.h>
#include <assert.h>

#include "util.h"
#include "common.h"
#include "anna_node.h"
#include "anna_stack.h"
#include "anna_node_create.h"
#include "anna_type.h"
#include "anna_function.h"
#include "anna_prepare.h"
#include "anna_util.h"
#include "anna_member.h"

#ifndef offsetof
#define offsetof(T,F) ((unsigned int)((char *)&((T *)0L)->F - (char *)0L))
#endif

typedef struct
{
    anna_stack_template_t *stack;
    anna_type_t *type;
}
    anna_stack_prepare_data;

anna_stack_template_t *anna_stack_create(size_t sz, anna_stack_template_t *parent)
{
    anna_stack_template_t *stack = calloc(1,sizeof(anna_stack_template_t) + sizeof(anna_object_t *)*sz);
    hash_init(&stack->member_string_identifier, &hash_wcs_func, &hash_wcs_cmp);
    stack->member_type = calloc(1, sizeof(anna_type_t *)*sz);
    stack->member_declare_node = calloc(1, sizeof(anna_node_t *)*sz);
    stack->member_flags = calloc(1, sizeof(int)*sz);
    stack->count = 0;
    stack->capacity = sz;
    stack->parent = parent;
    al_init(&stack->import);
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

    if(!initial_value)
    {
	wprintf(L"Critical: No initial value provided in declaration of %ls\n",
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
//	if(stack->member_type[*old_offset] != type)
//	{
	    wprintf(
		L"Critical: Tried to redeclare variable %ls\n",
		name);
	    CRASH;
	    
//	}
//	stack->member[*old_offset] = initial_value;
//	return;
    }
#ifdef ANNA_CHECK_STACK_ENABLED
    if(stack->flags & ANNA_STACK_FROZEN)
    {
	wprintf(L"Critical: Tried to declare value %ls in stack after it was frozen\n",
		name);
	CRASH;	
    }
#endif
    assert(stack->count < stack->capacity);
    
    size_t *offset = calloc(1,sizeof(size_t));
    *offset = stack->count++;
    hash_put(&stack->member_string_identifier, name, offset);
    stack->member_flags[*offset] = flags;
    stack->member_type[*offset] = type;
    stack->member_declare_node[*offset] = 0;
    stack->member[*offset] = initial_value;
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
//	if(stack->member_type[*old_offset] != type)
//	{
	    wprintf(
		L"Critical: Tried to redeclare variable %ls\n",
		declare_node->name);
	    
	    CRASH;
	    
//	}
//	stack->member[*old_offset] = initial_value;
//	return;
    }
#ifdef ANNA_CHECK_STACK_ENABLED
    if(stack->flags & ANNA_STACK_FROZEN)
    {
	wprintf(L"Critical: Tried to declare value %ls in stack after it was frozen\n",
		declare_node->name);
	CRASH;	
    }
#endif
    assert(stack->count < stack->capacity);
    
    size_t *offset = calloc(1,sizeof(size_t));
    *offset = stack->count++;
    hash_put(&stack->member_string_identifier, declare_node->name, offset);
    stack->member_flags[*offset] = 0;
    stack->member_type[*offset] = 0;
    stack->member_declare_node[*offset] = declare_node;
    stack->member[*offset] = null_object;
}

static inline anna_stack_template_t *anna_stack_template_search(
    anna_stack_template_t *stack,
    wchar_t *name,
    int import_only)
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
	    return import_only?0:stack;
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

anna_object_t **anna_stack_addr_get_str(anna_stack_template_t *stack, wchar_t *name)
{
    anna_stack_template_t *f = anna_stack_template_search(stack, name, 0);
    if(!f)
	return 0;
    return &f->member[*(size_t *)hash_get(&f->member_string_identifier, name)];
}

anna_object_t *anna_stack_template_get_str(anna_stack_template_t *stack, wchar_t *name)
{
    size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
    if(offset) 
    {
	return stack->member[*offset];
    }
    return 0;
}

void anna_stack_set_str(anna_stack_template_t *stack, wchar_t *name, anna_object_t *value)
{
//    wprintf(L"Set %ls to %ls\n", name, value->type->name);
    anna_stack_template_t *f = anna_stack_template_search(stack, name, 0);
    f->member[*(size_t *)hash_get(&f->member_string_identifier, name)] = value;
}

anna_object_t *anna_stack_get_str(anna_stack_template_t *stack, wchar_t *name)
{
#ifdef ANNA_CHECK_STACK_ACCESS
    anna_object_t **res =anna_stack_addr_get_str(stack, name);
    if(unlikely(!res))
    {
	wprintf(
	    L"Critical: Tried to access non-existing variable %ls\n",
	    name);
	anna_stack_print(stack);
	CRASH;
    }
    return *res;
#else
    return *anna_stack_addr_get_str(stack, name);
#endif
}

anna_type_t *anna_stack_get_type(anna_stack_template_t *stack, wchar_t *name)
{
    anna_stack_template_t *f = anna_stack_template_search(stack, name, 0);
    if(!f)
	return 0;
    return f->member_type[*(size_t *)hash_get(&f->member_string_identifier, name)];
}

int anna_stack_get_flag(anna_stack_template_t *stack, wchar_t *name)
{
    anna_stack_template_t *f = anna_stack_template_search(stack, name, 0);
    return &f->member_flags[*(size_t *)hash_get(&f->member_string_identifier, name)];
}

void anna_stack_set_type(anna_stack_template_t *stack, wchar_t *name, anna_type_t *type){
    anna_stack_template_t *f = anna_stack_template_search(stack, name, 0);
    f->member_type[*(size_t *)hash_get(&f->member_string_identifier, name)] = type;
}

anna_node_declare_t *anna_stack_get_declaration(
    anna_stack_template_t *stack, wchar_t *name)
{
    anna_stack_template_t *f = anna_stack_template_search(stack, name, 0);
    if(!f)
	return 0;
    return &f->member_declare_node[*(size_t *)hash_get(&f->member_string_identifier, name)];
}

anna_stack_template_t *anna_stack_get_import(anna_stack_template_t *stack, wchar_t *name)
{
    return anna_stack_template_search(stack, name, 1);
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

anna_object_t *anna_stack_get_sid(anna_stack_template_t *stack, anna_sid_t sid)
{
  int i;
  for(i=0; i<sid.frame; i++) {
    stack=stack->parent;
  }
  return stack->member[sid.offset];
}

void anna_stack_set_sid(anna_stack_template_t *stack, anna_sid_t sid, anna_object_t *value)
{
  int i;
  for(i=0; i<sid.frame; i++) {
    stack=stack->parent;
  }
  stack->member[sid.offset] = value;
}

anna_stack_template_t *anna_stack_clone(anna_stack_template_t *template)
{
    assert(template);
    size_t sz = sizeof(anna_stack_template_t) + sizeof(anna_object_t *)*template->count;
    //wprintf(L"Cloning stack with %d items (sz %d)\n", template->count, sz);
    anna_stack_template_t *stack = malloc(sz);
    memcpy(stack, template, sz);
    stack->stop=0;
    return stack;  
}

static void anna_print_stack_member(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    wchar_t *name = (wchar_t *)key_ptr;
    size_t *offset=(size_t *)val_ptr;
    anna_stack_template_t *stack = (anna_stack_template_t *)aux_ptr;
    anna_type_t *type = stack->member_type[*offset];
    //anna_object_t *value = stack->member[*offset];
    wprintf(L"%ls %ls = %ls\n", type?type->name:L"<UNKNOWN>", name, L"...");
}

void anna_stack_print(anna_stack_template_t *stack)
{
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

/*
static void anna_stack_create_property(anna_type_t *res, anna_stack_template_t *stack, wchar_t *name)
{
    size_t *offset = hash_get(&stack->member_string_identifier, name);
    anna_type_t *type = stack->member_type[*offset];
    anna_object_t *value = stack->member[*offset];

    if(!type)
    {
	debug(D_CRITICAL, L"Dang it. Stack variable %ls totally doesn't have a type!\n", name);
	CRASH;
    }
    
    anna_const_property_create(res, -1, name, value);
}
*/
static anna_type_t *anna_stack_type_create(anna_stack_template_t *stack)
{
    anna_type_t *res = anna_type_native_create(
	anna_util_identifier_generate(
	    L"StackType",
	    stack->function?&(stack->function->definition->location):0),
	stack_global);
    anna_member_create(
	res, ANNA_MID_STACK_PAYLOAD,  L"!stackPayload", 
	0, null_type);
    anna_member_create(
	res,
	ANNA_MID_STACK_TYPE_PAYLOAD,
	L"!stackTypePayload",
	1,
	null_type);
    (*anna_static_member_addr_get_mid(res, ANNA_MID_STACK_TYPE_PAYLOAD)) = (anna_object_t *)stack;
    return res;
}

void anna_stack_populate_wrapper(anna_stack_template_t *stack)
{
//    anna_stack_print(stack);
    anna_type_t *res = anna_stack_wrap(stack)->type;
    int i;
    array_list_t names = AL_STATIC;
    hash_foreach2(&stack->member_string_identifier, &anna_stack_save_name, &names);
    assert(al_get_count(&names) == stack->count);
    for(i=0; i<al_get_count(&names); i++)
    {
	wchar_t *name = (wchar_t *)al_get(&names, i);
	size_t *offset = hash_get(&stack->member_string_identifier, name);
	anna_type_t *mem_type = stack->member_type[*offset];
	anna_object_t *initial_value = stack->member[*offset];
	
	mid_t mid = anna_member_create(
	    res,
	    -1,
	    name,
	    1,
	    mem_type);
	*anna_static_member_addr_get_mid(
	    res,
	    mid) = initial_value;	
    }
    al_destroy(&names);
    
//    anna_type_print(res);
}

anna_object_t *anna_stack_wrap(anna_stack_template_t *stack)
{
    if(!stack->wrapper)
    {
/*
#ifdef ANNA_CHECK_STACK_ENABLED
stack->flags |= ANNA_STACK_FROZEN;
#endif
*/
	anna_type_t *t = anna_stack_type_create(stack);
	stack->wrapper = anna_object_create_raw(1);
	stack->wrapper->type=t;
	stack->wrapper->member[0] = (anna_object_t *)stack;
    }
    return stack->wrapper;
}

anna_stack_template_t *anna_stack_unwrap(anna_object_t *wrapper)
{
    return *(anna_stack_template_t **)anna_member_addr_get_mid(wrapper, ANNA_MID_STACK_PAYLOAD);
}

