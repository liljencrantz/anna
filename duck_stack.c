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
    wprintf(L"Critical: Tried to access unknown variable: %ls", name);
    exit(1);
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

duck_type_t *duck_stack_get_type(duck_stack_frame_t *stack, wchar_t *name)
{
    assert(name);
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_lookup, name);
	if(offset) 
	{
	    return &stack->member_type[*offset];
	}
	stack = stack->parent;
	
    }
    wprintf(L"Critical: Tried to access type of unknown variable: %ls", name);
    exit(1);
}

