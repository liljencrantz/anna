#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <wchar.h>

#include "anna/fallback.h"
#include "anna/base.h"
#include "anna/util.h"
#include "anna/common.h"
#include "anna/node.h"
#include "anna/use.h"
#include "anna/stack.h"
#include "anna/node_create.h"
#include "anna/type.h"
#include "anna/function.h"
#include "anna/misc.h"
#include "anna/member.h"
#include "anna/alloc.h"
#include "anna/intern.h"
#include "anna/vm.h"
#include "anna/mid.h"
#include "anna/lib/lang/string.h"
#include "anna/object.h"

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
			anna_entry_t initial_value,
			int flags)
{
    assert(name);
    
    if(stack->flags & ANNA_STACK_DECLARE)
    {
	return;
    }

    if(name[0]==L'!' && (wcscmp(name, L"!unused")!=0))
	return;    
    
    if(anna_entry_null_ptr(initial_value))
    {
	anna_error(
	    (anna_node_t *)0,
	    L"Tried to declare variable %ls with invalid initial value\n",
	    name);
	return;
    }

    assert(type);
    assert(stack);
    //anna_message(L"Declare %ls to be of type %ls\n", name, type->name);
    
    size_t *old_offset = hash_get(&stack->member_string_identifier, name);
    if(old_offset)
    {
	anna_error(
	    (anna_node_t *)0,
	    L"Tried to redeclare variable %ls\n",
	    name);
	return;
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
	anna_mid_get(name),
	ANNA_MEMBER_STATIC,
	type);    
    stack->flags = stack->flags & ~ANNA_STACK_DECLARE;
    *anna_entry_get_addr_static(
	res,
	mid) = (anna_entry_t )initial_value;

}

void anna_stack_declare2(anna_stack_template_t *stack, 
			 anna_node_declare_t *declare_node)
{
    if(!declare_node->name)
	CRASH;

    assert(stack);
    //anna_message(L"Declare %ls to be of type %ls\n", name, type->name);
    
    size_t *old_offset = hash_get(&stack->member_string_identifier, declare_node->name);
    if(old_offset)
    {
	anna_error(
	    (anna_node_t *)declare_node,
	    L"Tried to redeclare variable %ls\n",
	    declare_node->name);
	return;
    }
    anna_stack_ensure_capacity(stack, stack->count+1);
    
    size_t *offset = calloc(1,sizeof(size_t));
    *offset = stack->count++;
    hash_put(&stack->member_string_identifier, wcsdup(declare_node->name), offset);
    stack->member_flags[*offset] = 0;
    stack->member_declare_node[*offset] = declare_node;

    anna_type_t *res = anna_stack_wrap(stack)->type;
    stack->flags |= ANNA_STACK_DECLARE;
    int mid = anna_member_create(res, anna_mid_get(declare_node->name), 1, 0);
    stack->flags = stack->flags & ~ANNA_STACK_DECLARE;
    anna_member_t *memb = anna_member_get(res, mid);
    memb->attribute = (anna_node_call_t *)anna_node_clone_deep((anna_node_t *)declare_node->attribute);
}

anna_stack_template_t *anna_stack_template_search(
    anna_stack_template_t *stack,
    wchar_t *name)
{
    if(!stack)
    {
	anna_message(L"Critical: Null stack!\n");
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
	    anna_use_t *use = al_get(&stack->import, i);
	    
//	    anna_message(L"LALALA %ls %ls\n", name, use->type->name);
	    
	    anna_stack_template_t *import =
		anna_stack_unwrap(
		    anna_as_obj(anna_node_static_invoke_try(use->node, use->node->stack)));
	    
	    if(import)
	    {
		size_t *offset = (size_t *)hash_get(&import->member_string_identifier, name);
		if(offset) 
		{
		    return import;
		}
	    }
	    else if(anna_type_member_info_get(use->type, name))
	    {
		return 0;
	    }
	}
	stack = stack->parent;
    }
    return 0;
}

struct anna_use *anna_stack_search_use(
    anna_stack_template_t *stack,
    wchar_t *name)
{
    if(!stack)
    {
	anna_message(L"Critical: Null stack!\n");
	CRASH;	
    }    
  
    assert(name);
    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
	if(offset) 
	{
	    return 0;
	}
	
	int i;
	for(i=0; i<al_get_count(&stack->import); i++)
	{
	    anna_use_t *use = al_get(&stack->import, i);
	    anna_type_prepare_member(use->type, anna_mid_get(name));
	    if(anna_type_member_info_get(use->type, name))
	    {
		return use;
	    }
	}
	stack = stack->parent;
    }
    return 0;
}

anna_entry_t anna_stack_macro_get(
    anna_stack_template_t *stack,
    wchar_t *name)
{
    if(!stack)
    {
	anna_message(L"Critical: Null stack!\n");
	CRASH;	
    }  
    assert(name);

    while(stack)
    {
	int i;
	for(i=0; i<al_get_count(&stack->expand); i++)
	{
	    anna_use_t *use = al_get_fast(&stack->expand, i);
	    anna_stack_template_t *exp = 
		anna_stack_unwrap(
		    anna_as_obj(anna_node_static_invoke_try(use->node, use->node->stack)));
	    size_t *offset = (size_t *)hash_get(&exp->member_string_identifier, name);
	    
	    if(offset) 
	    {
		return anna_entry_get(
		    exp->wrapper, anna_mid_get(name));
	    }
	}
	stack = stack->parent;
    }
    return anna_from_obj(0);
}

anna_entry_t anna_stack_template_get(anna_stack_template_t *stack, wchar_t *name)
{
    size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
    if(offset) 
    {
	return anna_entry_get(
	    stack->wrapper, anna_mid_get(name));
    }
    return anna_from_obj(0);
}

void anna_stack_set(anna_stack_template_t *stack, wchar_t *name, anna_entry_t value)
{
    anna_use_t *use = anna_stack_search_use(stack, name);
    if(use)
    {
	anna_member_t *memb = anna_type_member_info_get(use->type, name);
	if(anna_member_is_static(memb))
	{
	    use->type->static_member[memb->offset] = value;
	}
	else
	{
	    CRASH;
	}
	return;
    }
    
//    anna_message(L"Set %ls to %ls\n", name, value->type->name);
    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    if(!f)
	return;

    anna_type_t *res = anna_stack_wrap(f)->type;
    *anna_entry_get_addr_static(
	res,
	anna_mid_get(name)) = (anna_entry_t )value;
}

anna_entry_t anna_stack_get(anna_stack_template_t *stack, wchar_t *name)
{
    anna_use_t *use = anna_stack_search_use(stack, name);
    if(use)
    {
	anna_member_t *memb = anna_type_member_info_get(use->type, name);
	if(anna_member_is_static(memb))
	{
	    return use->type->static_member[memb->offset];
	}
	debug(D_CRITICAL, L"Called stack_get on variable %ls, but it is accessed through use clause\n", name);
	CRASH;
    }

    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    if(!f)
	return anna_from_obj(0);
    anna_type_t *res = anna_stack_wrap(f)->type;
    return anna_entry_get_static(res, anna_mid_get(name));
}

anna_type_t *anna_stack_get_type(anna_stack_template_t *stack, wchar_t *name)
{
    anna_use_t *use = anna_stack_search_use(stack, name);
    if(use)
    {
	anna_member_t *memb = anna_type_member_info_get(use->type, name);
	return memb->type;
    }
    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    if(!f)
	return 0;
    anna_type_t *res = anna_stack_wrap(f)->type;
    anna_member_t *memb = anna_member_get(res, anna_mid_get(name));
    if(!memb->type)
    {
	anna_node_declare_t *decl = anna_stack_get_declaration(f, name);
	if(decl)
	{
	    anna_node_calculate_type((anna_node_t *)decl);
	    if(
		decl->return_type && 
		decl->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		anna_stack_set_type(f, name, decl->return_type);
	    }
	}
    }
    if(!memb->type)
    {
	return 0;
    }
    
    return memb->type;
}

anna_entry_t anna_stack_get_try(anna_stack_template_t *stack, wchar_t *name)
{
    if(!stack)
    {
	anna_message(L"Critical: Null stack!\n");
	CRASH;	
    }    

    while(stack)
    {
	size_t *offset = (size_t *)hash_get(&stack->member_string_identifier, name);
	if(offset) 
	{
	    anna_node_declare_t *decl = anna_stack_get_declaration(stack, name);
	    if(decl)
	    {
		anna_node_calculate_type((anna_node_t *)decl);
	    }
	    
	    if(anna_stack_get_flag(stack, name) & ANNA_STACK_READONLY)
	    {
		return anna_entry_get(
		    stack->wrapper, anna_mid_get(name));
	    }
	    break;
	}
	
	int i;
	for(i=0; i<al_get_count(&stack->import); i++)
	{
	    anna_use_t *use = al_get(&stack->import, i);
	    if(anna_type_member_info_get(use->type, name))
	    {
		anna_object_t *obj = anna_as_obj(
		    anna_node_static_invoke_try(
			use->node,
			use->node->stack));
		if(!obj)
		{
		    return anna_from_obj(0);
		}
		anna_entry_t *res = anna_entry_get_addr(
		    obj,
		    anna_mid_get(name));
		return res?*res:anna_from_obj(0);
	    }
	}
	stack = stack->parent;
    }
    return anna_from_obj(0);
}

int anna_stack_get_flag(anna_stack_template_t *stack, wchar_t *name)
{
    anna_use_t *use = anna_stack_search_use(stack, name);
    if(use)
    {
	debug(D_CRITICAL, L"Called stack_get_flag on variable %ls, but it is accessed through use clause\n", name);
	CRASH;
    }

    anna_stack_template_t *f = anna_stack_template_search(stack, name);
    return f->member_flags[*(size_t *)hash_get(&f->member_string_identifier, name)];
}

void anna_stack_set_flag(anna_stack_template_t *stack, wchar_t *name, int value)
{
    anna_use_t *use = anna_stack_search_use(stack, name);
    if(use)
    {
/*
	anna_member_t *memb = anna_type_member_info_get(use->type, name);
	memb->flag = value;
*/
	return;
    }

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
    if(!stack)
    {
	CRASH;
    }
    
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
      anna_message(L"Critical: Tried to create sid for unknown variable: %ls\n", name);
      CRASH;
    */
}

static void anna_print_stack_member(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    wchar_t *name = (wchar_t *)key_ptr;
    anna_stack_template_t *stack = (anna_stack_template_t *)aux_ptr;
    anna_type_t *res = anna_stack_wrap(stack)->type;
    anna_type_t *type = anna_member_get(res, anna_mid_get(name))->type;
    anna_message(L"%ls %ls = %ls\n", type?type->name:L"<UNKNOWN>", name, L"...");
}

void anna_stack_print(anna_stack_template_t *stack)
{
    if(!stack)
	return;
    
    anna_message(
	L"Stack frame with %d members belonging to function %ls:\n",
	stack->count,
	stack->function?stack->function->name:L"<null>");
    hash_foreach2(&stack->member_string_identifier, &anna_print_stack_member, stack);
    
    int i;
    for(i=0; i<al_get_count(&stack->import); i++)
    {
	anna_use_t *use = al_get(&stack->import, i);
	anna_message(L"    use %ls\n", use->type->name);
    }
    anna_stack_print(stack->parent);
}

int anna_stack_check(anna_stack_template_t *stack, int i)
{

    if(!stack)
    {
	return 0;
    }

    if(i<=0)
    {
	return 1;
    }

    return anna_stack_check(stack->parent, i-1);
}

void anna_stack_print_trace(anna_stack_template_t *stack)
{
    anna_message(L"Stack trace:\n");
    while(stack)
    {
	anna_message(
	    L"Stack frame belonging to function %ls\n",
	    stack->function?stack->function->name:L"<null>");
	stack = stack->parent;
    }
}
/*
static void anna_stack_to_string_item(
    anna_stack_template_t *stack, string_buffer_t *sb)
{
    if(!stack)
	return;
    anna_stack_to_string_item(stack->parent, sb);
    sb_printf(sb, L"%ls.", stack->name ? stack->name : L"?");
}
*/
static anna_type_t *anna_stack_type_create(anna_stack_template_t *stack)
{
    anna_type_t *res = anna_type_create(
	stack->name ? stack->name:L"AnonymousStackType", 0);
    res->stack = stack;
    
    anna_member_create(res, ANNA_MID_STACK_PAYLOAD, 0, null_type);
    anna_member_create(
	res,
	ANNA_MID_STACK_TYPE_PAYLOAD,
	ANNA_MEMBER_STATIC | ANNA_MEMBER_ALLOC,
	null_type);

    anna_entry_set_static_obj(res, ANNA_MID_STACK_TYPE_PAYLOAD, (anna_object_t *)stack);

    anna_type_close(res);
    return res;
}

anna_object_t *anna_stack_wrap(anna_stack_template_t *stack)
{
    if(!stack->wrapper)
    {
	anna_type_t *t = anna_stack_type_create(stack);
	stack->wrapper = anna_object_create(t);
	stack->wrapper->member[0] = anna_from_obj((anna_object_t *)stack);
    }
    return stack->wrapper;
}

anna_stack_template_t *anna_stack_unwrap(anna_object_t *wrapper)
{
    if(!wrapper)
	return 0;
    anna_stack_template_t **res = (anna_stack_template_t **)anna_entry_get_addr(wrapper, ANNA_MID_STACK_PAYLOAD);
    return res?*res:0;
}

void anna_stack_name(anna_stack_template_t *stack, wchar_t *name)
{
    stack->name = anna_intern(name);
}

void anna_stack_document(anna_stack_template_t *stack, wchar_t *documentation)
{
    anna_type_document(
	anna_stack_wrap(stack)->type,
	documentation);
}
