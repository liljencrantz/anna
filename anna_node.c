#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "common.h"
#include "wutil.h"
#include "anna_node.h"
#include "anna_node_check.h"
#include "anna_node_wrapper.h"
#include "anna_int.h"
#include "anna_float.h"
#include "anna_string.h"
#include "anna_char.h"
#include "anna_function.h"
#include "anna_prepare.h"
#include "anna_type.h"
#include "anna_module.h"
#include "anna_node_create.h"
#include "anna_member.h"
#include "anna_function_type.h"
#include "anna_list.h"

#define NODE_CHECK(test, node, ...) if(!(test)) {anna_error(node, __VA_ARGS__);}

#include "anna_node_prepare.c"

void anna_node_set_location(anna_node_t *node, anna_location_t *l)
{
    if(l)    
    {
	memcpy(&node->location, l, sizeof(anna_location_t));
    }
    else
    {
	memset(&node->location, 0, sizeof(anna_location_t));	
    }
    
}

anna_node_call_t *node_cast_call(anna_node_t *node) 
{
    
    if(node->node_type!=ANNA_NODE_CALL)
    {
	anna_error(node, L"Expected a call node");
	CRASH;
    }
    
    return (anna_node_call_t *)node;
}

anna_node_identifier_t *node_cast_identifier(anna_node_t *node) 
{
    if(node->node_type!=ANNA_NODE_IDENTIFIER)
    {
	anna_error(node, L"Expected an identifier node, got node of type %d", node->node_type);
	CRASH;
    }
    return (anna_node_identifier_t *)node;
}

anna_node_int_literal_t *node_cast_int_literal(anna_node_t *node) 
{
    assert(node->node_type==ANNA_NODE_INT_LITERAL);
    return (anna_node_int_literal_t *)node;
}

anna_node_string_literal_t *node_cast_string_literal(anna_node_t *node) 
{
    assert(node->node_type==ANNA_NODE_STRING_LITERAL);
    return (anna_node_string_literal_t *)node;
}


void anna_node_call_add_child(anna_node_call_t *call, anna_node_t *child)
{
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 8 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(anna_node_t *));
	if(!call->child) 
	{
	    wprintf(L"Out of memory\n");
	    CRASH;
	}	
	call->child_capacity = new_capacity;
    }
//    wprintf(L"LALALA %d %d\n", call->child_capacity, call->child_count);
    
    call->child[call->child_count++] = child;
}

void anna_node_call_prepend_child(anna_node_call_t *call, anna_node_t *child)
{
    assert(call->node_type == ANNA_NODE_CALL);
    
    if(call->child_count==0) 
    {
	anna_node_call_add_child(call, child);
	return;
    }
    
    if(call->child_capacity == call->child_count) 
    {
	size_t new_capacity = call->child_capacity < 4 ? 8 : call->child_capacity*2;
	call->child = realloc(call->child, new_capacity*sizeof(anna_node_t *));
	if(!call->child) 
	{
	    wprintf(L"Out of memory\n");
	    CRASH;
	}	
	call->child_capacity = new_capacity;
    }
    memmove(&call->child[1], &call->child[0], sizeof(anna_node_t *)*call->child_count);
    
    call->child[0] = child;
    call->child_count++;
}

void anna_node_call_set_function(anna_node_call_t *call, anna_node_t *function)
{
    call->function = function;
}
/*
static int anna_node_identifier_is_function(anna_node_identifier_t *id, anna_stack_frame_t *stack)
{
    anna_type_t *type = anna_stack_get_type(stack, id->name);
    if(!type)
	return 0;
    //CHECK(type, id, L"Unknown identifier: %ls", id->name);
    return !!anna_static_member_addr_get_mid(type, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
}
*/
anna_function_t *anna_node_macro_get(anna_node_call_t *node, anna_stack_frame_t *stack)
{
/*
    wprintf(L"Checking for macros in node (%d)\n", node->function->node_type);
    anna_node_print(node);
*/
    switch(node->function->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	{
//	    wprintf(L"It's an identifier\n");
	    anna_node_identifier_t *name=(anna_node_identifier_t *)node->function;

	    anna_object_t **obj = anna_stack_addr_get_str(stack, name->name);
	    if(obj && *obj != null_object)
	    {
		
		anna_function_t *func=anna_function_unwrap(*obj);
		//wprintf(L"Tried to find object %ls on stack, got %d, revealing internal function ptr %d\n", name->name, obj, func);
		
		if(func && (func->flags & ANNA_FUNCTION_MACRO))
		{
		    return func;
		}
	    }
	    
	    break;
	}


	case ANNA_NODE_CALL:
	{
//	    wprintf(L"It's a call\n");
	    anna_node_call_t *call=(anna_node_call_t *)node->function;
	    if(call->function->node_type != ANNA_NODE_IDENTIFIER)
	    {
		break;
	    }
	    anna_node_identifier_t *name=(anna_node_identifier_t *)call->function;	    
	    
	    if(wcscmp(name->name, L"__memberGet__") == 0)
	    {
//		wprintf(L"It's a member lookup\n");

		if(call->child_count == 2 && 
		   call->child[1]->node_type == ANNA_NODE_IDENTIFIER)
		{
		    anna_node_identifier_t *member_name=
			(anna_node_identifier_t *)call->child[1];
//		    wprintf(L"Looking up member %ls\n", member_name->name);
		    anna_object_t **obj = anna_stack_addr_get_str(stack, member_name->name);
		    
		    if(obj && (*obj)->type != null_type)
		    {
			anna_function_t *func=anna_function_unwrap(*obj);
			
			//wprintf(L"Found variable! %ls\n", func->name);

			if(func && (func->flags & ANNA_FUNCTION_MACRO))
			{
			    //wprintf(L"Found macro!\n");
			    
			    return func;
			}
		    }
		    
		}	
	    }
	}
	
/*	
	case ANNA_NODE_MEMBER_GET_WRAP:
	case ANNA_NODE_MEMBER_GET:
	{
	wprintf(L"Looking for macro in member get node\n");
	    
	anna_node_member_get_t *get = (anna_node_member_get_t *)node;
	wchar_t *name = anna_mid_get_reverse(get->mid);
	wprintf(L"Got a name! %ls\n", name);
	anna_object_t **obj = anna_stack_addr_get_str(stack, name);
	if(obj)
	{
	anna_function_t *func=anna_function_unwrap(*obj);
	    
	if(func && func->flags == ANNA_FUNCTION_MACRO)
	{
	return func;
	}
	}
	    
	break;
	}
*/	
	default:
	{
/*	    wprintf(L"Function is not an identifier, not a macro:\n");
	    anna_node_print(node->function);
	    wprintf(L"\n");
*/
	}
	
    }
    return 0;
    
}

static anna_object_t *anna_node_call_invoke(anna_node_call_t *this, anna_stack_frame_t *stack)
{
    //wprintf(L"anna_node_call_invoke with stack %d\n", stack);
    anna_object_t *obj = anna_node_invoke(this->function, stack);
    if(obj == null_object){
        //wprintf(L"Invoked null object!\n");      
	return obj;
    }
    
    return anna_function_wrapped_invoke(obj, 0, this->child_count, this->child, stack);
}

static anna_object_t *anna_node_int_literal_invoke(anna_node_int_literal_t *this, anna_stack_frame_t *stack)
{
    return anna_int_create(this->payload);
}

static anna_object_t *anna_node_float_literal_invoke(anna_node_float_literal_t *this, anna_stack_frame_t *stack)
{
    return anna_float_create(this->payload);
}

static anna_object_t *anna_node_string_literal_invoke(anna_node_string_literal_t *this, anna_stack_frame_t *stack)
{
    return anna_string_create(this->payload_size, this->payload);
}

static anna_object_t *anna_node_char_literal_invoke(anna_node_char_literal_t *this, anna_stack_frame_t *stack)
{
    return anna_char_create(this->payload);
}

static anna_object_t *anna_node_identifier_invoke(anna_node_identifier_t *this, anna_stack_frame_t *stack)
{
    return anna_stack_get_str(stack, this->name);
    //wprintf(L"Lookup on identifier \"%ls\", sid is %d,%d\n", this->name, this->sid.frame, this->sid.offset);
#ifdef ANNA_CHECK_SID_ENABLED
    anna_sid_t real_sid = anna_stack_sid_create(stack, this->name);
    if(memcmp(&this->sid, &real_sid, sizeof(anna_sid_t))!=0)
    {
        anna_error(
	    (anna_node_t *)this,
	    L"Critical: Cached sid (%d, %d) different from invoke time sid (%d, %d) for node %ls",
	    this->sid.frame,
	    this->sid.offset,
	    real_sid.frame,
	    real_sid.offset,
	    this->name);
	anna_stack_print_trace(stack);	
	
	CRASH;
    }
    anna_object_t *res = anna_stack_get_sid(stack, this->sid);
    if(!res)
    {
        anna_error((anna_node_t *)this, L"Critical: Identifier «%ls» had invalid value on stack!\n",this->name);
	anna_stack_print(stack);	
	CRASH;
    }
    return res;
#else
    return anna_stack_get_sid(stack, this->sid);
#endif  
}

static anna_object_t *anna_node_assign_invoke(anna_node_assign_t *this, anna_stack_frame_t *stack)
{
    anna_object_t *result = anna_node_invoke(this->value, stack);
    anna_stack_set_str(stack, this->name, result);
    return result;
}

anna_type_t *anna_node_get_return_type(anna_node_t *this, anna_stack_frame_t *stack)
{
    return 0;
}


void anna_node_validate(anna_node_t *this, anna_stack_frame_t *stack)
{
}



static anna_object_t *anna_node_member_get_invoke(anna_node_member_get_t *this, 
					   anna_stack_frame_t *stack)
{
    //wprintf(L"ACCESSING MEMBER %ls\n", anna_mid_get_reverse(this->mid));
    /*
      wprintf(L"Run member get node:\n");
      anna_node_print(this);
    */
    assert(this->object);
    anna_object_t *obj = anna_node_invoke(this->object, stack);
    if(!obj)
    {
	anna_error(this->object, L"Critical: Node evaluated to null pointer:");
	anna_node_print(this->object);
	CRASH;
    }
    anna_member_t *m = obj->type->mid_identifier[this->mid];
    if(!m)
    {
	anna_error(this->object, L"Critical: Object %ls does not have a member %ls",
		   obj->type->name,
		   anna_mid_get_reverse(this->mid));
    }

    if(m->is_property)
    {
	anna_object_t *method = obj->type->static_member[m->getter_offset];
	return anna_function_wrapped_invoke(method, obj, 0, 0, stack);	
    }
    
    anna_object_t *res;
    
    if(m->is_static) {
	res = obj->type->static_member[m->offset];
    } else {
	res = (obj->member[m->offset]);
    }
    
    return res;
  
    //return *anna_member_addr_get_mid(anna_node_invoke(this->object, stack), this->mid);
}

static anna_object_t *anna_node_member_set_invoke(anna_node_member_set_t *this, 
					   anna_stack_frame_t *stack)
{
    /*
      wprintf(L"Run member set node:\n");
      anna_node_print(this);
    */
    assert(this->object);
    anna_object_t *obj = anna_node_invoke(this->object, stack);
    if(!obj)
    {
	anna_error(this->object, L"Critical: Node evaluated to null pointer:");
	anna_node_print(this->object);
	CRASH;
    }

    anna_member_t *m = obj->type->mid_identifier[this->mid];
    
    if(!m)
    {
	anna_error(this->object, L"Critical: Object %ls does not have a member %ls",
		   obj->type->name,
		   anna_mid_get_reverse(this->mid));
    }
    if(m->is_property)
    {
	anna_object_t *method = obj->type->static_member[m->setter_offset];
	anna_node_call_t *call = anna_node_create_call(0, 0, 1, &this->value);
	return anna_function_wrapped_invoke(method, obj, call->child_count, call->child, stack);
    }
    else 
    {
	anna_object_t *val = anna_node_invoke(this->value, stack);
	if(!val)
	{
	    anna_error(this->value, L"Critical: Node evaluated to null pointer:");
	    anna_node_print(this->value);
	    CRASH;
	}

	if(m->is_static) {
	    obj->type->static_member[m->offset]=val;
	} else {
	    obj->member[m->offset]=val;
	}
	return val;
    }

    //return *anna_member_addr_get_mid(anna_node_invoke(this->object, stack), this->mid);
}

static anna_object_t *anna_node_member_get_wrap_invoke(
    anna_node_member_get_t *this, 
    anna_stack_frame_t *stack)
{
    /*
      wprintf(L"Run wrapped member get node:\n");  
      anna_node_print(this);
    */
    assert(this->object);
    anna_object_t *obj = anna_node_invoke(this->object, stack);
    if(!obj)
    {
	anna_error(this->object, L"Critical: Node evaluated to null pointer:");
	anna_node_print(this->object);
	CRASH;
    }
    //anna_object_print(obj);
    
    anna_member_t *m = obj->type->mid_identifier[this->mid];
    if(!m)
    {
	anna_error(this->object, L"Critical: Object %ls does not have a member %ls",
		   obj->type->name,
		   anna_mid_get_reverse(this->mid));
    }

    anna_object_t *res;
    if(m->is_property)
    {
	anna_object_t *method = obj->type->static_member[m->getter_offset];
	res =  anna_function_wrapped_invoke(method, obj, 0, 0, stack);
    }
    else 
    {
	if(m->is_static) {
	    res = obj->type->static_member[m->offset];
	} else {
	    res = (obj->member[m->offset]);
	}
    }
        
    if(!res)
    {
	anna_error(
	    this->object, L"Critical: Object %ls does not have a member %ls",
	    obj->type->name,
	    anna_mid_get_reverse(this->mid));
    }
    anna_object_t *wrapped = anna_method_wrap(res, obj);
    if(!wrapped)
    {
	anna_error(this->object, L"Critical: Failed to wrap object:");
	anna_node_print(this->object);
	CRASH;
    }    
    return wrapped;
}

static anna_object_t *anna_trampoline(
    anna_function_t *fun,
    anna_stack_frame_t *stack)
{
    anna_object_t *orig = fun->wrapper;
    anna_object_t *res = anna_object_create(orig->type);
    
    if(!anna_member_addr_get_mid(res,ANNA_MID_FUNCTION_WRAPPER_PAYLOAD))
    {
/*	anna_member_t *m = obj->type->mid_identifier[mid];
	if(!m) 
	{
	    return 0;
	}
*/
	wprintf(L"Critical: Bad trampoline input\n");
	anna_object_print(res);
	//anna_function_print(orig->member[0]);
	
	CRASH;
    }

//    wprintf(L"Creating a trampoline for function %ls with shiny new stack:\n", anna_function_unwrap(orig)->name);
//    anna_stack_print(stack);

    memcpy(anna_member_addr_get_mid(res,ANNA_MID_FUNCTION_WRAPPER_PAYLOAD),
	   anna_member_addr_get_mid(orig,ANNA_MID_FUNCTION_WRAPPER_PAYLOAD),
	   sizeof(anna_function_t *));    
    memcpy(anna_member_addr_get_mid(res,ANNA_MID_FUNCTION_WRAPPER_STACK),
	   &stack,
	   sizeof(anna_stack_frame_t *));

    return res;
}

anna_object_t *anna_node_invoke(anna_node_t *this, 
				anna_stack_frame_t *stack)
{
    //wprintf(L"anna_node_invoke with stack %d\n", stack);
    //wprintf(L"invoke %d\n", this->node_type);    
    if(!this)
    {
	wprintf(L"Critical: Invoke null node\n");
	CRASH;
    }
    
    ANNA_CHECK_NODE_PREPARED(this);
    
    switch(this->node_type)
    {
	case ANNA_NODE_CALL:
	    return anna_node_call_invoke(
		(anna_node_call_t *)this, 
		stack);

	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_member_call_t *this2 = (anna_node_member_call_t *)this;
	    anna_object_t *obj = anna_node_invoke(this2->object, stack);
	    anna_member_t *m = obj->type->mid_identifier[this2->mid];
	    if(!m)
	    {
		anna_error(this2->object, L"Critical: Object %ls does not have a member %ls",
			   obj->type->name,
			   anna_mid_get_reverse(this2->mid));
	    }
	    anna_object_t *res;
	    if(m->is_property)
	    {
		anna_object_t *method = obj->type->static_member[m->getter_offset];
		res =  anna_function_wrapped_invoke(method, obj, 0, 0, stack);
	    }
	    else 
	    {
		if(m->is_static) {
		    res = obj->type->static_member[m->offset];
		} else {
		    res = (obj->member[m->offset]);
		}
	    }
	    
	    if(!res)
	    {
		anna_error(
		    this2->object, L"Critical: Object %ls does not have a member %ls",
		    obj->type->name,
		    anna_mid_get_reverse(this2->mid));
	    }
	    
	    if(res == null_object){
		return null_object;
	    }
/*
	    wprintf(L"MEMBER CALL on object:\n");
	    anna_object_print(obj);
	    wprintf(L"Member:\n");
	    anna_object_print(res);
	    wprintf(L"\nNode:\n");
*/
	    return anna_function_wrapped_invoke(res, m->is_method?obj:0, this2->child_count, this2->child, stack);
	}
	
	case ANNA_NODE_CONSTRUCT:
	{
	    anna_node_call_t *call = 
		(anna_node_call_t *)this;
	    //wprintf(L"Wee, calling construct with %d parameter, %d\n", call->child_count, call->child[0]);
	    return anna_construct(
		anna_type_unwrap(
		    anna_node_invoke(
			call->function,
			stack)),
		call,
		stack);
	}
    
	case ANNA_NODE_RETURN:
	{
	    anna_node_return_t *node = (anna_node_return_t *)this;
	    anna_object_t *result = anna_node_invoke(node->payload, stack);
	    stack->stop=1;
	    return result;
	}
	
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_BLOB:
	{
	    anna_node_dummy_t *node = (anna_node_dummy_t *)this;
	    return node->payload;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *node = (anna_node_closure_t *)this;
	    
	    if(wcscmp(node->payload->wrapper->type->name, L"!FakeFunctionType") == 0)
	    {
		anna_error(
		    this, 
		    L"Closure content not a proper function: %ls",
		    node->payload->wrapper->type->name);
		CRASH;
	    }
	    return anna_trampoline(node->payload, stack);
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    return anna_type_wrap(f);
	}

	case ANNA_NODE_INT_LITERAL:
	    return anna_node_int_literal_invoke((anna_node_int_literal_t *)this, stack);

	case ANNA_NODE_FLOAT_LITERAL:
	    return anna_node_float_literal_invoke((anna_node_float_literal_t *)this, stack);

	case ANNA_NODE_IDENTIFIER:
	    return anna_node_identifier_invoke((anna_node_identifier_t *)this, stack);	    

	case ANNA_NODE_STRING_LITERAL:
	    return anna_node_string_literal_invoke((anna_node_string_literal_t *)this, stack);

	case ANNA_NODE_CHAR_LITERAL:
	    return anna_node_char_literal_invoke((anna_node_char_literal_t *)this, stack);

	case ANNA_NODE_NULL:
	    return null_object;

	case ANNA_NODE_ASSIGN:
	    return anna_node_assign_invoke((anna_node_assign_t *)this, stack);
	    
	case ANNA_NODE_CONST:
	case ANNA_NODE_DECLARE:
	    return anna_node_assign_invoke((anna_node_assign_t *)this, stack);
	    
	case ANNA_NODE_MEMBER_GET:
	    return anna_node_member_get_invoke((anna_node_member_get_t *)this, stack);

	case ANNA_NODE_MEMBER_SET:
	    return anna_node_member_set_invoke((anna_node_member_set_t *)this, stack);

	case ANNA_NODE_MEMBER_GET_WRAP:
	    return anna_node_member_get_wrap_invoke((anna_node_member_get_t *)this, stack);

	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *n = (anna_node_cond_t *)this;
	    anna_object_t *o1 = anna_node_invoke(n->arg1, stack);
	    if(o1 == null_object)
	    {
		return anna_node_invoke(n->arg2, stack);
	    }
	    else 
	    {
		return o1;
	    }
	}

	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *n = (anna_node_cond_t *)this;
	    anna_object_t *o1 = anna_node_invoke(n->arg1, stack);
	    if(o1 == null_object)
	    {
		return null_object;
	    }
	    else 
	    {
		return anna_node_invoke(n->arg2, stack);
	    }
	}

	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *n = (anna_node_cond_t *)this;
	    anna_object_t *res = null_object;
	    anna_object_t *fun = anna_node_invoke((anna_node_t *)n->arg2, stack);
	    while(anna_node_invoke(n->arg1, stack) != null_object)
	    {
		wprintf(L"Tralala\n");
		res = anna_function_wrapped_invoke(fun, 0, 0, 0, stack);
	    }
	    return res;
	}

	case ANNA_NODE_IF:
	{
	    anna_node_if_t *n = (anna_node_if_t *)this;
	    anna_object_t *o1 = anna_node_invoke(n->cond, stack);
	    anna_object_t *fun = anna_node_invoke((anna_node_t *)((o1!= null_object)?n->block1:n->block2), stack);
//	    wprintf(L"if: Evaluated condition, result was %ls\n", o1 != null_object?L"true":L"False");
	    
	    return anna_function_wrapped_invoke(fun, 0, 0, 0, stack);
	}
	
	default:
	    wprintf(L"HOLP! Unknown node type %d found during invoke!\n", this->node_type);
	    CRASH;
    }    
}



static size_t anna_node_size(anna_node_t *n)
{
    switch(n->node_type)
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	    return sizeof(anna_node_call_t);
	    
	case ANNA_NODE_IDENTIFIER:
	    return sizeof(anna_node_identifier_t);

	case ANNA_NODE_INT_LITERAL:
	    return sizeof(anna_node_int_literal_t);
	case ANNA_NODE_STRING_LITERAL:
	    return sizeof(anna_node_string_literal_t);
	case ANNA_NODE_CHAR_LITERAL:
	    return sizeof(anna_node_char_literal_t);
	case ANNA_NODE_FLOAT_LITERAL:
	    return sizeof(anna_node_float_literal_t);
	case ANNA_NODE_NULL:
	    return sizeof(anna_node_t);
	case ANNA_NODE_BLOB:
	case ANNA_NODE_DUMMY:
	    return sizeof(anna_node_dummy_t);
	case ANNA_NODE_CLOSURE:
	    return sizeof(anna_node_closure_t);
	case ANNA_NODE_ASSIGN:
	    return sizeof(anna_node_assign_t);
	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	    return sizeof(anna_node_member_get_t);
	case ANNA_NODE_MEMBER_SET:
	    return sizeof(anna_node_member_set_t);
	case ANNA_NODE_RETURN:
	    return sizeof(anna_node_return_t);
	default:
	    anna_error(n, L"Unknown node type while determining size\n");
	    CRASH;
    }
    
}

anna_node_t *anna_node_clone_shallow(anna_node_t *n)
{
    size_t sz = anna_node_size(n);
    anna_node_t *r = malloc(sz);
    memcpy(r,n,sz);
    r->wrapper=0;
    ANNA_UNPREPARED(r);
    return r;
}

anna_node_t *anna_node_clone_deep(anna_node_t *n)
{
    switch(n->node_type)
    {
	/*
	  Clone a call node. This invlolves a shallow clone of the
	  node as well as calling the deep clone of every child and
	  the function node.
	*/
	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	{
	    anna_node_t *r = anna_node_clone_shallow(n);
	    int i;
	    anna_node_call_t *r2=(anna_node_call_t *)r;
	    anna_node_call_t *n2=(anna_node_call_t *)n;
	    r2->child = malloc(sizeof(anna_node_t *)*r2->child_capacity);
	    memcpy(r2->child, n2->child, sizeof(anna_node_t *)*r2->child_count);
	    r2->function = anna_node_clone_deep(r2->function);

	    for(i=0;i<r2->child_count; i++)
	    {
		r2->child[i] = anna_node_clone_deep(r2->child[i]);
	    }
	    return r;
	}
	
	/*
	  These nodes are not mutable and they have no child nodes, so
	  we can return them as is.
	*/
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_BLOB:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_CLOSURE:
	case ANNA_NODE_IDENTIFIER:
	    return anna_node_clone_shallow(n);
	    
	    /*
	      These nodes are not yet handled, but that should be
	      perfectly ok for now, since they are only ever created
	      by the prepare system, and cloning a prepared AST is
	      never supported or possible.
	    */
	case ANNA_NODE_ASSIGN:
	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	case ANNA_NODE_MEMBER_SET:
	case ANNA_NODE_RETURN:
	default:
	    anna_error(n, L"Unsupported node type %d for deep copy!\n", n->node_type);
	    CRASH;
	
    }
}

anna_node_t *anna_node_replace(anna_node_t *tree, anna_node_identifier_t *from, anna_node_t *to)
{
    switch(tree->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *tree2 = (anna_node_identifier_t *)tree;
	    return (wcscmp(tree2->name,from->name)==0)?
		anna_node_clone_deep(to):tree;
	}

	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_BLOB:
	case ANNA_NODE_CLOSURE:
	{
	    return tree;
	}


	case ANNA_NODE_IMPORT:
	{
	    anna_node_import_t *this2 =(anna_node_import_t *)anna_node_clone_shallow(tree);	    
	    this2->payload = anna_node_replace(this2->payload,
					       from, to);
	    return (anna_node_t *)this2;	    
	}

	case ANNA_NODE_RETURN:
	{
	    anna_node_return_t *this2 =(anna_node_return_t *)anna_node_clone_shallow(tree);
	    this2->payload = anna_node_replace(this2->payload,
					       from, to);
	    return (anna_node_t *)this2;
	}

	case ANNA_NODE_CALL:
	case ANNA_NODE_CONSTRUCT:
	{
	    int i;
	    anna_node_call_t *this2 =(anna_node_call_t *)anna_node_clone_shallow(tree);	    
	    this2->function = anna_node_replace(this2->function,
						from, to);
	    for(i=0;i<this2->child_count;i++)
	    {
		this2->child[i] = anna_node_replace(this2->child[i],
						    from, to);
	    }
	    return (anna_node_t *)this2;	    
	}
	
	default:
	    wprintf(L"OOPS! Unknown node type when replacing: %d\n", tree->node_type);
	    CRASH;
    }
    
}

int anna_node_compare(anna_node_t *node1, anna_node_t *node2)
{
    if(node1->node_type != node2->node_type)
    {
        return 0;
    }
   
    switch(node1->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *id1 = (anna_node_identifier_t *)node1;
	    anna_node_identifier_t *id2 = (anna_node_identifier_t *)node2;
	    return wcscmp(id1->name, id2->name) == 0;
	}
	
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *n1 = (anna_node_call_t *)node1;
	    anna_node_call_t *n2 = (anna_node_call_t *)node2;
	    if(n1->child_count != n2->child_count)
		return 0;
	    if(!anna_node_compare(n1->function, n2->function))
		return 0;
	    int i;
	    for(i=0; i<n1->child_count;i++)
	    {
		if(!anna_node_compare(n1->child[i], n2->child[i]))
		    return 0;
	    }
	    return 1;
	}
	
	case ANNA_NODE_INT_LITERAL:
	{
	    anna_node_int_literal_t *n1 = (anna_node_int_literal_t *)node1;
	    anna_node_int_literal_t *n2 = (anna_node_int_literal_t *)node2;
	    return n1->payload == n2->payload;
	}
	
	case ANNA_NODE_CHAR_LITERAL:
	{
	    anna_node_char_literal_t *n1 = (anna_node_char_literal_t *)node1;
	    anna_node_char_literal_t *n2 = (anna_node_char_literal_t *)node2;
	    return n1->payload == n2->payload;
	}
	
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_node_float_literal_t *n1 = (anna_node_float_literal_t *)node1;
	    anna_node_float_literal_t *n2 = (anna_node_float_literal_t *)node2;
	    return n1->payload == n2->payload;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *n1 = (anna_node_string_literal_t *)node1;
	    anna_node_string_literal_t *n2 = (anna_node_string_literal_t *)node2;
	    if(n1->payload_size != n2->payload_size)
		return 0;
	    return wcscmp(n1->payload, n2->payload) == 0;	   
	}
	
	default:
	    wprintf(L"OOPS! Unknown node type when comparing: %d\n", node1->node_type);
	    CRASH;
	
    }

}

void anna_node_each(anna_node_t *this, anna_node_function_t fun, void *aux)
{
    fun(this, aux);
    switch(this->node_type)
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_SPECIALIZE:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    anna_node_each(n->function, fun, aux);
	    int i;
	    for(i=0; i<n->child_count; i++)
		anna_node_each(n->child[i], fun, aux);
	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	{	    
	    anna_node_member_call_t *n = (anna_node_member_call_t *)this;
	    anna_node_each(n->object, fun, aux);
	    int i;
	    for(i=0; i<n->child_count; i++)
		anna_node_each(n->child[i], fun, aux);
	    break;
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *n = (anna_node_assign_t *)this;
	    anna_node_each(n->value, fun, aux);
	    break;   
	}

	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_get_t *n = (anna_node_member_get_t *)this;
	    anna_node_each(n->object, fun, aux);
	    break;   
	}

	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_get_t *n = (anna_node_member_get_t *)this;
	    anna_node_each(n->object, fun, aux);
	    break;   
	}

	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	{
	    anna_node_declare_t *n = (anna_node_declare_t *)this;
	    anna_node_each(n->type, fun, aux);
	    anna_node_each(n->value, fun, aux);
	    break;   
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_set_t *n = (anna_node_member_set_t *)this;
	    anna_node_each(n->object, fun, aux);
	    anna_node_each(n->value, fun, aux);
	    break;   
	}

	case ANNA_NODE_WHILE:
	case ANNA_NODE_OR:
	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *n = (anna_node_cond_t *)this;
	    anna_node_each(n->arg1, fun, aux);
	    anna_node_each(n->arg2, fun, aux);
	    break;   
	}
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *c = (anna_node_if_t *)this;
	    anna_node_each(c->cond, fun, aux);
	    anna_node_each((anna_node_t *)c->block1, fun, aux);
	    anna_node_each((anna_node_t *)c->block2, fun, aux);
	    break;
	}	

	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_CLOSURE:
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_RETURN:
	case ANNA_NODE_TYPE_LOOKUP:
	case ANNA_NODE_TYPE:
	{
	    break;   
	}
	
	default:
	    wprintf(L"OOPS! Unknown node type when iterating over AST: %d\n", this->node_type);
	    CRASH;
    }    
    
}


typedef struct
{
    int node_type;
    array_list_t *al;
}
    anna_node_find_each_t;


static void anna_node_find_each(anna_node_t *node, void *aux)
{
    anna_node_find_each_t *data = (anna_node_find_each_t *)aux;
    if(node->node_type == data->node_type)
    {
	al_push(data->al, node);
    }
}

void anna_node_find(anna_node_t *this, int node_type, array_list_t *al)
{
    anna_node_find_each_t data = 
	{
	    node_type, al
	}
    ;
    
    anna_node_each(this, &anna_node_find_each, &data);
}


