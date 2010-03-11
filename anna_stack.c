#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "anna_node.h"
#include "anna_stack.h"

anna_stack_frame_t *anna_stack_create(size_t sz, anna_stack_frame_t *parent)
{
   anna_stack_frame_t *stack = calloc(1,sizeof(anna_stack_frame_t) + sizeof(anna_object_t *)*sz);
   hash_init(&stack->member_string_identifier, &hash_wcs_func, &hash_wcs_cmp);
   stack->member_type = calloc(1, sizeof(anna_type_t *)*sz);
   stack->count = 0;
   stack->capacity = sz;
   stack->parent = parent;
   return stack;
}

void anna_stack_declare(anna_stack_frame_t *stack, 
			wchar_t *name,
			anna_type_t *type, 
			anna_object_t *initial_value)
{
    if(!name)
	CRASH;
    
    if(!initial_value)
      {
	CRASH;
      }

    if(wcscmp(name, L"a") == 0)
      CRASH;
    
    
    assert(name);
    assert(type);
    assert(stack);
    
    //wprintf(L"Declare %ls to %d\n", name, initial_value);
    
    size_t *old_offset = hash_get(&stack->member_string_identifier, name);
    if(old_offset)
    {
	if(stack->member_type[*old_offset] != type)
	{
	    wprintf(L"Critical: Tried to redeclare variable %ls, was of type %ls, now of type %ls\n", name, stack->member_type[*old_offset]->name, type->name);
	    exit(1);
	    
	}
	stack->member[*old_offset] = initial_value;
	return;
    }
    assert(stack->count < stack->capacity);
    
    size_t *offset = calloc(1,sizeof(size_t));
    *offset = stack->count++;
    hash_put(&stack->member_string_identifier, name, offset);
    stack->member_type[*offset] = type;
    stack->member[*offset] = initial_value;
}

anna_object_t **anna_stack_addr_get_str(anna_stack_frame_t *stack, wchar_t *name)
{
    //anna_stack_frame_t *orig = stack;
  
    assert(name);
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
	if(offset) 
	{
	    return &stack->member[*offset];
	}
	stack = stack->parent;
    }
    wprintf(L"Critical: Tried to access unknown variable: %ls\nStack content:\n", name);
    //anna_stack_print(orig);
    CRASH;
}

void anna_stack_set_str(anna_stack_frame_t *stack, wchar_t *name, anna_object_t *value)
{
    //wprintf(L"Set %ls to %d\n", name, value);
    (*anna_stack_addr_get_str(stack, name)) = value;
}

anna_object_t *anna_stack_get_str(anna_stack_frame_t *stack, wchar_t *name)
{
  //wprintf(L"Get %ls: %d\n", name, *anna_stack_addr_get_str(stack, name));    
  return *anna_stack_addr_get_str(stack, name);
}

anna_type_t *anna_stack_get_type(anna_stack_frame_t *stack_orig, wchar_t *name)
{
    assert(stack_orig);
    assert(name);
    anna_stack_frame_t *stack = stack_orig;
    
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
	if(offset) 
	{
	  return stack->member_type[*offset];
	}
	stack = stack->parent;	
    }
    return 0;
    
}

anna_sid_t anna_stack_sid_create(anna_stack_frame_t *stack, wchar_t *name)
{
    anna_stack_frame_t *top = stack;
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
    wprintf(L"Critical: Tried to create sid for unknown variable: %ls\n", name);
    CRASH;
}

anna_object_t *anna_stack_get_sid(anna_stack_frame_t *stack, anna_sid_t sid)
{
  int i;
  for(i=0; i<sid.frame; i++) {
    stack=stack->parent;
  }
  return stack->member[sid.offset];
}

void anna_stack_set_sid(anna_stack_frame_t *stack, anna_sid_t sid, anna_object_t *value)
{
  int i;
  for(i=0; i<sid.frame; i++) {
    stack=stack->parent;
  }
  stack->member[sid.offset] = value;
}

anna_stack_frame_t *anna_stack_clone(anna_stack_frame_t *template)
{
  assert(template);
  
    size_t sz = sizeof(anna_stack_frame_t) + sizeof(anna_object_t *)*template->count;
    //wprintf(L"Cloning stack with %d items (sz %d)\n", template->count, sz);
    anna_stack_frame_t *stack = malloc(sz);
    memcpy(stack, template, sz);
    stack->stop=0;
    return stack;  
}

void anna_print_stack_member(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    wchar_t *name = (wchar_t *)key_ptr;
    size_t *offset=(size_t *)val_ptr;
    anna_stack_frame_t *stack = (anna_stack_frame_t *)aux_ptr;
    anna_type_t *type = stack->member_type[*offset];
    anna_object_t *value = stack->member[*offset];
    wprintf(L"%ls %ls = %ls\n", type->name, name, L"...");
}

void anna_stack_print(anna_stack_frame_t *stack)
{
    if(!stack)
	return;
    hash_foreach2(&stack->member_string_identifier, &anna_print_stack_member, stack);
    anna_stack_print(stack->parent);
}
