#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "anna_node.h"
#include "anna_stack.h"
#include "anna_node_create.h"
#include "anna_type.h"
#include "anna_function.h"
#include "anna_prepare.h"
#include "anna_util.h"

typedef struct
{
    anna_stack_frame_t *stack;
    anna_type_t *type;
}
    anna_stack_prepare_data;


anna_stack_frame_t *anna_stack_create(size_t sz, anna_stack_frame_t *parent)
{
    anna_stack_frame_t *stack = calloc(1,sizeof(anna_stack_frame_t) + sizeof(anna_object_t *)*sz);
    hash_init(&stack->member_string_identifier, &hash_wcs_func, &hash_wcs_cmp);
    stack->member_type = calloc(1, sizeof(anna_type_t *)*sz);
    stack->member_declare_node = calloc(1, sizeof(anna_node_t *)*sz);
    stack->member_flags = calloc(1, sizeof(int)*sz);
    stack->count = 0;
    stack->capacity = sz;
    stack->parent = parent;
    return stack;
}

void anna_stack_declare(anna_stack_frame_t *stack, 
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

void anna_stack_declare2(anna_stack_frame_t *stack, 
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
    return 0;
    
    wprintf(L"Critical: Tried to access unknown variable: %ls\nStack content:\n", name);
    //anna_stack_print(orig);
    CRASH;
}

anna_object_t *anna_stack_frame_get_str(anna_stack_frame_t *stack, wchar_t *name)
{
    size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
    if(offset) 
    {
	return stack->member[*offset];
    }
    return 0;
}

void anna_stack_set_str(anna_stack_frame_t *stack, wchar_t *name, anna_object_t *value)
{
    //wprintf(L"Set %ls to %d\n", name, value);
    (*anna_stack_addr_get_str(stack, name)) = value;
}

anna_object_t *anna_stack_get_str(anna_stack_frame_t *stack, wchar_t *name)
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

anna_type_t *anna_stack_get_type(anna_stack_frame_t *stack_orig, wchar_t *name)
{
    if(!stack_orig)
    {
	wprintf(L"Critical: Null stack!\n");
	CRASH;	
    }
    
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

void anna_stack_set_type(anna_stack_frame_t *stack_orig, wchar_t *name, anna_type_t *type){
    if(!stack_orig)
    {
	wprintf(L"Critical: Null stack!\n");
	CRASH;	
    }
    
    assert(name);
    anna_stack_frame_t *stack = stack_orig;
    
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
	if(offset) 
	{
	    stack->member_type[*offset] = type;
	    return;
	}
	stack = stack->parent;	
    }
    return;
}

anna_node_declare_t *anna_stack_get_declaration(
    anna_stack_frame_t *stack_orig, wchar_t *name)
{
    if(!stack_orig)
    {
	wprintf(L"Critical: Null stack!\n");
	CRASH;	
    }
    
    assert(name);
    anna_stack_frame_t *stack = stack_orig;
    
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
	if(offset) 
	{
	    return stack->member_declare_node[*offset];
	}
	stack = stack->parent;	
    }
    return 0;
}


anna_sid_t anna_stack_sid_create(anna_stack_frame_t *stack, wchar_t *name)
{
//    anna_stack_frame_t *top = stack;
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
    //anna_object_t *value = stack->member[*offset];
    wprintf(L"%ls %ls = %ls\n", type?type->name:L"<UNKNOWN>", name, L"...");
}

void anna_stack_print(anna_stack_frame_t *stack)
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

int anna_stack_depth(anna_stack_frame_t *stack)
{
    return stack?anna_stack_depth(stack->parent)+1:0;
}

void anna_stack_print_trace(anna_stack_frame_t *stack)
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

anna_type_t *anna_stack_type_create(anna_stack_frame_t *stack)
{
    anna_type_t *res = anna_type_native_create(
	anna_util_identifier_generate(
	    L"StackType",
	    stack->function?&(stack->function->definition->location):0),
	stack_global);
    anna_node_call_t *definition = 
	anna_type_definition_get(res);
    anna_member_add_node(
	definition, ANNA_MID_STACK_PAYLOAD,  L"!stackPayload", 
	0, (anna_node_t *)anna_node_create_identifier(0, L"Null") );
    anna_member_add_node(
	definition, ANNA_MID_STACK_TYPE_PAYLOAD,  L"!stackTypePayload", 
	1, (anna_node_t *)anna_node_create_identifier(0, L"Null") );
    anna_member_create(
	res,
	ANNA_MID_STACK_TYPE_PAYLOAD,
	L"!stackTypePayload",
	1,
	null_type);
    
    (*anna_static_member_addr_get_mid(res, ANNA_MID_STACK_TYPE_PAYLOAD)) = (anna_object_t *)stack;
        
    //anna_prepare_type_implementation(res);
    return res;
}

anna_object_t *anna_stack_wrap(anna_stack_frame_t *stack)
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

anna_stack_frame_t *anna_stack_unwrap(anna_object_t *wrapper)
{
    return *(anna_stack_frame_t **)anna_member_addr_get_mid(wrapper, ANNA_MID_STACK_PAYLOAD);
}

#if 0
static void anna_stack_prepare_member(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    wchar_t *name = (wchar_t *)key_ptr;
    size_t *offset=(size_t *)val_ptr;
    anna_stack_prepare_data *data = (anna_stack_prepare_data *)aux_ptr;
    
//    wprintf(L"Adding member %ls to namespace %ls\n", name, data->type->name);
    
    anna_node_call_t *body = anna_node_create_block(0);
/*
    anna_node_t *param[] =
	{
	    (anna_node_t *)anna_node_create_identifier(
		0,
		L"this"),
	    
	    (anna_node_t *)anna_node_create_identifier(
		0,
		name),
	    
	}
    ;
*/		       
/*
    anna_node_call_add_child(
	body,
	anna_node_create_call(
	    0,
	    anna_node_create_identifier(
		0,
		L"memberGet"),
	    2,
	    param));
*/
    anna_node_call_add_child(
	body,
	(anna_node_t *)anna_node_create_identifier(
	    0,
	    name));
    
    wchar_t *argn[] = 
	{
	    L"this"
	}
    ;

    anna_type_t *argv[] = 
	{
	    data->type
	}
    ;
    string_buffer_t getter_sb;
    sb_init(&getter_sb);
    sb_append(&getter_sb, L"!");
    sb_append(&getter_sb, name);
    sb_append(&getter_sb, L"Getter");
    wchar_t *getter_name = sb_content(&getter_sb);
        
    anna_function_t *result;
    result = anna_function_create(
	getter_name,
	ANNA_FUNCTION_ANONYMOUS,
	body,
	data->stack->member_type[*offset],
	1,
	argv,
	argn,
	data->stack);
    result->member_of = data->type;
    result->mid = anna_method_create(data->type, -1, result->name, 0, result);	
    
    anna_member_create(data->type,
		       -1,
		       name,
		       0,
		       data->stack->member_type[*offset]);
    data->type->member_count--;
    data->type->property_count++;
    anna_member_t *memb = anna_type_member_info_get(data->type, name);
    memb->is_property = 1;
    
    anna_member_t *method = anna_type_member_info_get(data->type, getter_name);
    memb->getter_offset = method->offset;
    
    assert(result->input_count == 1);
    
    anna_function_type_key_t *ggg = anna_function_unwrap_type(result->wrapper->type);
    assert(ggg->argc == 1);
        
    sb_destroy(&getter_sb);
}

void anna_stack_prepare(anna_type_t *type)
{
	    
    //wprintf(L"We have a stack wrapper type %ls!\n", type->name);
    anna_stack_frame_t *stack = 
	(anna_stack_frame_t *)*anna_static_member_addr_get_mid(type, ANNA_MID_STACK_TYPE_PAYLOAD);
    assert(stack);
    assert(stack->function);
    
    //wprintf(L"Find out what we should put in namespace...\n");
    anna_prepare_function(stack->function);
    
    //wprintf(L"Function is prepared\n");
    anna_stack_prepare_data data = 
	{
	    stack,type
	};
    hash_foreach2(&stack->member_string_identifier, &anna_stack_prepare_member, &data);
    //wprintf(L"DONELIDONE\n");
    
}
#endif
