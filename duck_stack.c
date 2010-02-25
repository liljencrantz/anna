#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "duck_node.h"
#include "duck_stack.h"

duck_stack_frame_t *duck_stack_create(size_t sz, duck_stack_frame_t *parent)
{
   duck_stack_frame_t *stack = calloc(1,sizeof(duck_stack_frame_t) + sizeof(duck_object_t *)*sz);
   hash_init(&stack->member_string_lookup, &hash_wcs_func, &hash_wcs_cmp);
   stack->member_type = calloc(1, sizeof(duck_type_t *)*sz);
   stack->count = 0;
   stack->capacity = sz;
   stack->parent = parent;
   return stack;
}

void duck_stack_declare(duck_stack_frame_t *stack, 
			wchar_t *name,
			duck_type_t *type, 
			duck_object_t *initial_value)
{
    assert(initial_value);
    assert(name);
    assert(type);
    assert(stack);
    
    //wprintf(L"Declare %ls to %d\n", name, initial_value);
    
    size_t *old_offset = hash_get(&stack->member_string_lookup, name);
    if(old_offset)
    {
	assert(stack->member_type[*old_offset] == type);
	stack->member[*old_offset] = initial_value;
	return;
    }
    assert(stack->count < stack->capacity);
    
    size_t *offset = calloc(1,sizeof(size_t));
    *offset = stack->count++;
    hash_put(&stack->member_string_lookup, name, offset);
    stack->member_type[*offset] = type;
    stack->member[*offset] = initial_value;
}

duck_object_t **duck_stack_addr_get_str(duck_stack_frame_t *stack, wchar_t *name)
{
    assert(name);
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_lookup, name);
	if(offset) 
	{
	    return &stack->member[*offset];
	}
	stack = stack->parent;
    }
    wprintf(L"Critical: Tried to access unknown variable: %ls\n", name);
    CRASH;
}

void duck_stack_set_str(duck_stack_frame_t *stack, wchar_t *name, duck_object_t *value)
{
    //wprintf(L"Set %ls to %d\n", name, value);
    (*duck_stack_addr_get_str(stack, name)) = value;
}

duck_object_t *duck_stack_get_str(duck_stack_frame_t *stack, wchar_t *name)
{
  //wprintf(L"Get %ls: %d\n", name, *duck_stack_addr_get_str(stack, name));    
  return *duck_stack_addr_get_str(stack, name);
}

duck_type_t *duck_stack_get_type(duck_stack_frame_t *stack_orig, wchar_t *name)
{
    assert(stack_orig);
    assert(name);
    duck_stack_frame_t *stack = stack_orig;
    
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_lookup, name);
	if(offset) 
	{
	  return stack->member_type[*offset];
	}
	stack = stack->parent;	
    }
    wprintf(L"Critical: Tried to access type of unknown variable: %ls in stack %d\n", name, stack_orig);
    CRASH;
    
}

duck_sid_t duck_stack_sid_create(duck_stack_frame_t *stack, wchar_t *name)
{
    duck_stack_frame_t *top = stack;
    assert(stack);
    assert(name);
    duck_sid_t sid = {0,0};
    
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_lookup, name);
	if(offset) 
	{
	  sid.offset = *offset;
	  return sid;
	}
	sid.frame++;
	stack = stack->parent;	
    }
    wprintf(L"Critical: Tried to create sid for unknown variable: %ls\n", name);
    CRASH;
}

duck_object_t *duck_stack_get_sid(duck_stack_frame_t *stack, duck_sid_t sid)
{
  int i;
  for(i=0; i<sid.frame; i++) {
    stack=stack->parent;
  }
  return stack->member[sid.offset];
}

void duck_stack_set_sid(duck_stack_frame_t *stack, duck_sid_t sid, duck_object_t *value)
{
  int i;
  for(i=0; i<sid.frame; i++) {
    stack=stack->parent;
  }
  stack->member[sid.offset] = value;
}

duck_stack_frame_t *duck_stack_clone(duck_stack_frame_t *template)
{
  assert(template);
  
    size_t sz = sizeof(duck_stack_frame_t) + sizeof(duck_object_t *)*template->count;
    //wprintf(L"Cloning stack with %d items (sz %d)\n", template->count, sz);
    duck_stack_frame_t *stack = malloc(sz);
    memcpy(stack, template, sz);
    return stack;  
}

void duck_print_stack_member(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    wchar_t *name = (wchar_t *)key_ptr;
    size_t *offset=(size_t *)val_ptr;
    duck_stack_frame_t *stack = (duck_stack_frame_t *)aux_ptr;
    duck_type_t *type = stack->member_type[*offset];
    duck_object_t *value = stack->member[*offset];
    wprintf(L"%ls %ls = %ls\n", type->name, name, L"...");
}

void duck_stack_print(duck_stack_frame_t *stack)
{
    if(!stack)
	return;
    hash_foreach2(&stack->member_string_lookup, &duck_print_stack_member, stack);
    duck_stack_print(stack->parent);
}
