#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "anna.h"
#include "anna_vm.h"
#include "anna_function.h"
#include "anna_node.h"
#include "anna_list.h"
#include "anna_int.h"
#include "anna_float.h"
#include "anna_string.h"
#include "anna_char.h"
#include "anna_stack.h"
#include "anna_function_type.h"
#include "anna_member.h"
#include "anna_type.h"
#include "anna_alloc.h"

/**
   Pops one value from the top stack frame, the pops the top stack
   frame itself, and pushes the value ont the new top stack.
 */
#define ANNA_OP_RETURN 0 
/**
   Pushes a single constant value onto the stack.
 */
#define ANNA_OP_CONSTANT 1
/**
   Calls a function. Pops the number of parameters specified in the
   op, then pops the function. The parameters are copied onto a new
   stack frame. 
 */
#define ANNA_OP_CALL 2
/**
   Stop bytecode execution and return.
 */
#define ANNA_OP_STOP 3 
/**
   Pushes the specified variable to the stack
 */
#define ANNA_OP_VAR_GET 4
/**
   Copies the value on the top of the stack to the specified location
   without poping it.
 */
#define ANNA_OP_VAR_SET 5
/**
   Pops an object from the stack and pushes the object member
   specified by the op instead.
 */
#define ANNA_OP_MEMBER_GET 6
#define ANNA_OP_STATIC_MEMBER_GET 21
/**
   Pops a value and an object from the stack and pushes the object member
   specified by the op instead.
 */
#define ANNA_OP_MEMBER_SET 7
#define ANNA_OP_STRING 8
#define ANNA_OP_LIST 9
#define ANNA_OP_FOLD 10
#define ANNA_OP_COND_JMP 11
#define ANNA_OP_NCOND_JMP 12
#define ANNA_OP_POP 13
#define ANNA_OP_NOT 14
#define ANNA_OP_DUP 15
#define ANNA_OP_MEMBER_GET_THIS 16
#define ANNA_OP_JMP 17
#define ANNA_OP_TRAMPOLENE 18
#define ANNA_OP_CONSTRUCT 19
#define ANNA_OP_CAST 20

static anna_vmstack_t **stack_mem;
static anna_vmstack_t **stack;

size_t anna_vm_stack_frame_count()
{
    return stack - stack_mem+1;
}

anna_vmstack_t *anna_vm_stack_get(size_t idx)
{
    return stack_mem[idx];
}

typedef struct 
{
    char instruction;
    int __padding;
}
    anna_op_null_t;

typedef struct
{
    char instruction;
    anna_object_t *value;
}
    anna_op_const_t;

typedef struct
{
    char instruction;
    anna_type_t *value;
}
    anna_op_type_t;

typedef struct
{
    char instruction;
    short frame_count;
    int offset;
}
    anna_op_var_t;

typedef struct
{
    char instruction;
    int mid;
}
    anna_op_member_t;

typedef struct
{
    char instruction;
    int param;
}
    anna_op_call_t;

typedef struct
{
    char instruction;
    ssize_t offset;
}
    anna_op_off_t;

static void anna_push(anna_vmstack_t **stack, anna_object_t *val)
{
/*
    if(!val)
    {
	debug(
	    D_CRITICAL,L"Pushed null ptr\n");
	CRASH;	
    }
*/  
    anna_object_t ** top =((*stack)->top);
    *(top)= val;
    (*stack)->top++;
}

static anna_object_t *anna_pop(anna_vmstack_t **stack)
{
    (*stack)->top--;
    anna_object_t *top = *((*stack)->top);
    return top;
}

static anna_object_t *anna_peek(anna_vmstack_t **stack, size_t off)
{
    return *((*stack)->top-1-off);
}

static inline anna_object_t *anna_vm_trampoline(
    anna_function_t *fun,
    anna_vmstack_t *stack)
{
    anna_object_t *orig = fun->wrapper;
    anna_object_t *res = anna_object_create(orig->type);
    size_t payload_offset = orig->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_PAYLOAD]->offset;
    size_t stack_offset = orig->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_STACK]->offset;
    
    memcpy(&res->member[payload_offset],
	   &orig->member[payload_offset],
	   sizeof(anna_function_t *));    
    memcpy(&res->member[stack_offset],
	   &stack,
	   sizeof(anna_vmstack_t *));
    return res;
}

static void anna_vmstack_print(anna_vmstack_t *stack)
{
    anna_object_t **p = &stack->base[0];
    wprintf(L"Stack content:\n");
    while(p!=stack->top)
    {
	if(!*p){
	    wprintf(L"Error: Null slot\n");
	    
	}
	else
	{
	    anna_function_t *fun = anna_function_unwrap((*p));
	    if(fun)
	    {
		wprintf(L"Function: %ls\n", fun->name);
	    }
	    else
	    {
		wprintf(L"%ls\n", (*p)->type->name);
	    }
	}
	
	p++;
    }
}

#define anna_frame_push(stack, wfun) {					\
        size_t stack_offset = wfun->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_STACK]->offset; \
	anna_vmstack_t *parent = *(anna_vmstack_t **)&wfun->member[stack_offset];\
	anna_function_t *fun = anna_function_unwrap(wfun);		\
	anna_vmstack_t *res = anna_alloc_vmstack(fun->frame_size);	\
	res->parent=parent;						\
	res->function = fun;						\
	res->code = fun->code;						\
	(*stack)->top -= (fun->input_count+1);				\
	memcpy(&res->base[0], (*stack)->top+1,				\
	       sizeof(anna_object_t *)*fun->input_count);		\
	res->top = &res->base[fun->variable_count];			\
	*(++stack) = res;						\
    }

void anna_vm_init()
{
    stack_mem = malloc(sizeof(anna_vmstack_t *)*1024);
    stack = stack_mem-1;
}

#ifdef ANNA_FULL_GC_ON_SHUTDOWN
void anna_vm_destroy(void)
{
    free(stack_mem);
}
#endif

anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_object_t **argv)
{
    anna_function_t *fun = anna_function_unwrap(entry);
    if(fun->native.function)
    {
	return fun->native.function(argv);
    }    
    
    stack++;
    
    *stack = anna_alloc_vmstack((argc+1)*sizeof(anna_object_t *) + sizeof(anna_vmstack_t));
    (*stack)->parent = *(anna_vmstack_t **)anna_member_addr_get_mid(entry,ANNA_MID_FUNCTION_WRAPPER_STACK);
    
    (*stack)->function = 0;
    (*stack)->top = &(*stack)->base[0];
    (*stack)->code = malloc(1);
    *(*stack)->code = ANNA_OP_STOP;
    
    int i;
    
    anna_push(stack, entry);
    for(i=0; i<argc; i++)
    {
	anna_push(stack, argv[i]);
    }
    anna_frame_push(stack, entry);
    
    while(1)
    {
	char instruction = *(*stack)->code;
//	wprintf(L"STACK SIZE %d\n", (*stack)->top - &(*stack)->base[0]);
	switch(instruction)
	{
	    case ANNA_OP_CONSTANT:
	    {
		anna_op_const_t *op = (anna_op_const_t *)(*stack)->code;
		anna_push(stack, op->value);
		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_STRING:
	    {
		anna_op_const_t *op = (anna_op_const_t *)(*stack)->code;
		anna_push(stack, anna_string_copy(op->value));
		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_CAST:
	    {
		anna_op_type_t *op = (anna_op_type_t *)(*stack)->code;
		if(!anna_abides(anna_peek(stack,0)->type, op->value))
		{
		    anna_pop(stack);
		    anna_push(stack, null_object);
		}
		
		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_CALL:
	    {
		anna_op_call_t *op = (anna_op_call_t *)(*stack)->code;
		size_t param = op->param;
		anna_object_t *wrapped = anna_peek(stack, param);
		
		anna_function_t *fun = anna_function_unwrap(wrapped);
		#if ANNA_CHECK_VM
		if(!fun)
		{
		    wprintf(L"Error: Tried to call something that is not a function with %d params. Stack contents:\n", param);
		    anna_vmstack_print(*stack);
		    CRASH;
		}
		#endif

		if(fun->native.function)
		{
		    anna_object_t *res = fun->native.function(
			((*stack)->top-param));
		    if(!res)
		    {
			anna_function_print(fun);
			CRASH;
		    }
		    
		    
		    (*stack)->top -= (param+1);
		    anna_push(stack, res);
		    (*stack)->code += sizeof(*op);		    
		}
		else
		{
		    (*stack)->code += sizeof(*op);
		    anna_frame_push(stack, wrapped);
		}
		
		break;
	    }
	    
	    case ANNA_OP_CONSTRUCT:
	    {
		anna_op_null_t *op = (anna_op_null_t *)(*stack)->code;
		anna_object_t *wrapped = anna_pop(stack);

		anna_type_t *tp = anna_type_unwrap(wrapped);
		
		anna_object_t *result = anna_object_create(tp);
		
		anna_object_t **constructor_ptr = anna_static_member_addr_get_mid(
		    tp,
		    ANNA_MID_INIT_PAYLOAD);
		anna_push(stack, *constructor_ptr);
		anna_push(stack, result);
		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_RETURN:
	    {
		anna_object_t *val = anna_peek(stack, 0);
		--stack;
		anna_push(stack, val);
//		wprintf(L"Pop frame\n");
		break;
	    }
	    
	    case ANNA_OP_STOP:
	    {
//		wprintf(L"Pop last frame\n");
		anna_object_t *val = anna_peek(stack, 0);
		free((*stack)->code);
		--stack;
		return val;
	    }
	    
	    case ANNA_OP_VAR_GET:
	    {
		anna_op_var_t *op = (anna_op_var_t *)(*stack)->code;
		int i;
		anna_vmstack_t *s = *stack;
		for(i=0; i<op->frame_count; i++)
		    s = s->parent;
		#if ANNA_CHECK_VM
		if(!s->base[op->offset])
		{
		    wprintf(
			L"Var get op on unassigned var: %d %d\n",
			op->frame_count, op->offset);
		    
		    CRASH;
		}
		#endif
		anna_push(stack, s->base[op->offset]);
		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_VAR_SET:
	    {
		anna_op_var_t *op = (anna_op_var_t *)(*stack)->code;
		int i;
		anna_vmstack_t *s = *stack;
		for(i=0; i<op->frame_count; i++)
		    s = s->parent;
		s->base[op->offset] = anna_peek(stack, 0);
		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_MEMBER_GET:
	    {
		anna_op_member_t *op = (anna_op_member_t *)(*stack)->code;
		anna_object_t *obj = anna_pop(stack);

		anna_member_t *m = obj->type->mid_identifier[op->mid];

		#if ANNA_CHECK_VM
		if(!m)
		{
		    debug(
			D_CRITICAL,
			L"Object of type %ls does not have a member of type %ls\n",
			obj->type->name,
			anna_mid_get_reverse(op->mid));    
		    CRASH;
		}
		#endif 

		if(m->is_property)
		{
		    
		    anna_object_t *method = obj->type->static_member[m->getter_offset];
		    anna_function_t *fun = anna_function_unwrap(method);

		    if(fun->native.function)
		    {
			anna_object_t *res = fun->native.function(
			    &obj);
			anna_push(stack, res);
			(*stack)->code += sizeof(*op);		    
		    }
		    else
		    {
			anna_push(stack, method);
			anna_push(stack, obj);
			(*stack)->code += sizeof(*op);
			anna_frame_push(stack, method);
		    }
		}
		else
		{
		    anna_object_t *res;
		    res = obj->member[m->offset];		    
		    anna_push(stack, res);
		    
		    (*stack)->code += sizeof(*op);
		}
		break;
	    }
	    
	    case ANNA_OP_STATIC_MEMBER_GET:
	    {
		anna_op_member_t *op = (anna_op_member_t *)(*stack)->code;
		anna_object_t *obj = anna_pop(stack);

		anna_member_t *m = obj->type->mid_identifier[op->mid];

		#if ANNA_CHECK_VM
		if(!m)
		{
		    debug(
			D_CRITICAL,
			L"Object of type %ls does not have a member of type %ls\n",
			obj->type->name,
			anna_mid_get_reverse(op->mid));    
		    CRASH;
		}
		#endif 

		if(m->is_property)
		{
		    
		    anna_object_t *method = obj->type->static_member[m->getter_offset];
		    anna_function_t *fun = anna_function_unwrap(method);

		    if(fun->native.function)
		    {
			anna_object_t *res = fun->native.function(
			    &obj);
			anna_push(stack, res);
			(*stack)->code += sizeof(*op);		    
		    }
		    else
		    {
			anna_push(stack, method);
			anna_push(stack, obj);
			(*stack)->code += sizeof(*op);
			anna_frame_push(stack, method);
		    }
		}
		else
		{
		    anna_object_t *res;
		    res = obj->type->static_member[m->offset];
		    anna_push(stack, res);
		    (*stack)->code += sizeof(*op);
		}
		break;
	    }
	    
	    case ANNA_OP_MEMBER_GET_THIS:
	    {
		anna_op_member_t *op = (anna_op_member_t *)(*stack)->code;
		anna_object_t *obj = anna_pop(stack);
		#if ANNA_CHECK_VM
		if(!obj){
		    debug(
			D_CRITICAL,L"Popped null ptr for member get op %ls\n",
			anna_mid_get_reverse(op->mid));
		    CRASH;
		}
		#endif
		anna_member_t *m = obj->type->mid_identifier[op->mid];
		#if ANNA_CHECK_VM
		if(!m){
		    debug(
			D_CRITICAL,L"Object %ls does not have a member named %ls\n",
			obj->type->name, anna_mid_get_reverse(op->mid));
		    anna_vmstack_print(*stack);
		    
		    CRASH;
		}
		#endif 
		if(m->is_property)
		{
		    anna_object_t *method = obj->type->static_member[m->getter_offset];
		    wprintf(L"PROPERTIES NOT YET IMPLEMENTED!!!\n");
		    CRASH;
		}
		anna_object_t *res;
		
		if(m->is_static) {
		    res = obj->type->static_member[m->offset];
		} else {
		    res = (obj->member[m->offset]);
		}
		anna_push(stack, res);
		anna_push(stack, obj);

		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_MEMBER_SET:
	    {
		anna_op_member_t *op = (anna_op_member_t *)(*stack)->code;
		anna_object_t *obj = anna_pop(stack);
		anna_object_t *value = anna_peek(stack, 0);
		
		anna_member_t *m = obj->type->mid_identifier[op->mid];

		#if ANNA_CHECK_VM
		if(!m)
		{
		    debug(
			D_CRITICAL,
			L"Object of type %ls does not have a member of type %ls\n",
			obj->type->name,
			anna_mid_get_reverse(op->mid));    
		    CRASH;
		}
		#endif 

		if(m->is_property)
		{
		    anna_object_t *method = obj->type->static_member[m->setter_offset];
		    anna_function_t *fun = anna_function_unwrap(method);
		    
		    if(fun->native.function)
		    {
			anna_push(stack, obj);
			anna_push(stack, value);
			anna_object_t *res = fun->native.function(
			    (*stack)->top-2);
			anna_pop(stack);
			anna_pop(stack);			
			(*stack)->code += sizeof(*op);		    
		    }
		    else
		    {
			anna_pop(stack);
			anna_push(stack, method);
			anna_push(stack, obj);
			anna_push(stack, value);
			(*stack)->code += sizeof(*op);
			anna_frame_push(stack, method);
		    }
		}
		else
		{
		    anna_object_t *res;		    
		    if(m->is_static) {
			obj->type->static_member[m->offset] = value;
		    } else {
			obj->member[m->offset] = value;
		    }
		    
		    (*stack)->code += sizeof(*op);
		}
		break;
	    }

	    case ANNA_OP_LIST:
	    {
		anna_op_type_t *op = (anna_op_type_t *)(*stack)->code;
		anna_push(stack, anna_list_create2(op->value));
		(*stack)->code += sizeof(*op);
		break;
	    }

	    case ANNA_OP_FOLD:
	    {
		anna_object_t *val = anna_pop(stack);
		anna_list_add(anna_peek(stack, 0), val);
		(*stack)->code += sizeof(anna_op_null_t);
		break;
	    }
	    
	    case ANNA_OP_POP:
	    {
		anna_pop(stack);
		(*stack)->code += sizeof(anna_op_null_t);
		break;
	    }
	    
	    case ANNA_OP_NOT:
	    {
		*((*stack)->top-1) = (*((*stack)->top-1)==null_object)?anna_int_one:null_object;
		(*stack)->code += sizeof(anna_op_null_t);
		break;
	    }

	    case ANNA_OP_DUP:
	    {
		anna_push(stack, anna_peek(stack, 0));
		(*stack)->code += sizeof(anna_op_null_t);
		break;
	    }

	    case ANNA_OP_JMP:
	    {
		anna_op_off_t *op = (anna_op_off_t *)(*stack)->code;
		(*stack)->code += op->offset;
		break;
	    }
	    
	    case ANNA_OP_COND_JMP:
	    {
		anna_op_off_t *op = (anna_op_off_t *)(*stack)->code;
		(*stack)->code += anna_pop(stack) != null_object ? op->offset:sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_NCOND_JMP:
	    {
		anna_op_off_t *op = (anna_op_off_t *)(*stack)->code;
		(*stack)->code += anna_pop(stack) == null_object ? op->offset:sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_TRAMPOLENE:
	    {
		anna_object_t *base = anna_pop(stack);
		anna_push(stack, anna_vm_trampoline(anna_function_unwrap(base), *stack));
		(*stack)->code += sizeof(anna_op_null_t);
		break;
	    }
	    
	    default:
	    {
		wprintf(L"Unknown opcode %d\n", instruction);
		CRASH;
	    }
	}
    }
}

size_t anna_bc_op_size(char instruction)
{
    switch(instruction)
    {
	case ANNA_OP_STRING:
	case ANNA_OP_CONSTANT:
	{
	    return sizeof(anna_op_const_t);
	}
	    
	case ANNA_OP_LIST:
	case ANNA_OP_CAST:
	{
	    return sizeof(anna_op_type_t);
	}
	    
	case ANNA_OP_FOLD:
	case ANNA_OP_CONSTRUCT:
	case ANNA_OP_RETURN:
	case ANNA_OP_STOP:
	case ANNA_OP_TRAMPOLENE:
	case ANNA_OP_POP:
	case ANNA_OP_NOT:
	case ANNA_OP_DUP:
	{
	    return sizeof(anna_op_null_t);
	}
	
	case ANNA_OP_CALL:
	{
	    return sizeof(anna_op_call_t);
	}
	
	case ANNA_OP_VAR_SET:
	case ANNA_OP_VAR_GET:
	{
	    return sizeof(anna_op_var_t);
	}
	    
	case ANNA_OP_MEMBER_GET:
	case ANNA_OP_STATIC_MEMBER_GET:
	case ANNA_OP_MEMBER_GET_THIS:
	case ANNA_OP_MEMBER_SET:
	{
	    return sizeof(anna_op_member_t);
	}
	
	    
	case ANNA_OP_JMP:
	case ANNA_OP_COND_JMP:
	case ANNA_OP_NCOND_JMP:
	{
	    return sizeof(anna_op_off_t);
	}
		    
	default:
	{
	    wprintf(L"Unknown opcode %d\n", instruction);
	    CRASH;
	}
    }
}



void anna_bc_print(char *code)
{
    wprintf(L"Code:\n");
    char *base = code;
    while(1)
    {
	wprintf(L"%d: ", code-base);
	char instruction = *code;
	switch(instruction)
	{
	    case ANNA_OP_STRING:
	    case ANNA_OP_CONSTANT:
	    {
		anna_op_const_t *op = (anna_op_const_t*)code;
		wprintf(L"Push constant of type %ls\n\n", op->value->type->name);
		break;
	    }
	    
	    case ANNA_OP_LIST:
	    {
		wprintf(L"List creation\n\n");
		break;
	    }
	    
	    case ANNA_OP_CAST:
	    {
		wprintf(L"Type cast\n\n");
		break;
	    }
	    
	    case ANNA_OP_FOLD:
	    {
		wprintf(L"List fold\n\n");
		break;
	    }
	    
	    case ANNA_OP_CALL:
	    {
		anna_op_call_t *op = (anna_op_call_t *)code;
		size_t param = op->param;
		wprintf(L"Call function with %d parameter(s)\n\n", param);
		break;
	    }
	    
	    case ANNA_OP_CONSTRUCT:
	    {
		wprintf(L"Construct object\n\n");
		break;
	    }
	    
	    case ANNA_OP_RETURN:
	    {
		wprintf(L"Return\n\n");
		return;
	    }
	    
	    case ANNA_OP_STOP:
	    {
		wprintf(L"Stop\n\n");
		return;
	    }
	    
	    case ANNA_OP_VAR_GET:
	    {
		anna_op_var_t *op = (anna_op_var_t *)code;
		wprintf(L"Get var %d : %d\n\n", op->frame_count, op->offset);
		break;
	    }
	    
	    case ANNA_OP_VAR_SET:
	    {
		anna_op_var_t *op = (anna_op_var_t *)code;
		wprintf(L"Set var %d : %d\n\n", op->frame_count, op->offset);
		break;
	    }
	    
	    case ANNA_OP_MEMBER_GET:
	    case ANNA_OP_STATIC_MEMBER_GET:
	    {
		anna_op_member_t *op = (anna_op_member_t *)code;
		wprintf(L"Get member %d\n\n", op->mid);
		break;
	    }
	    
	    case ANNA_OP_MEMBER_GET_THIS:
	    {
		anna_op_member_t *op = (anna_op_member_t *)code;
		wprintf(L"Get member %d and push object as implicit this param\n\n", op->mid);
		break;
	    }
	    
	    case ANNA_OP_MEMBER_SET:
	    {
		anna_op_member_t *op = (anna_op_member_t *)code;
		wprintf(L"Set member %d\n\n", op->mid);
		break;
	    }

	    case ANNA_OP_POP:
	    {
		wprintf(L"Pop stack\n\n");
		break;
	    }
	    
	    case ANNA_OP_NOT:
	    {
		wprintf(L"Invert stack top element\n\n");
		break;
	    }
	    
	    case ANNA_OP_DUP:
	    {
		wprintf(L"Duplicate stack top element\n\n");
		break;
	    }
	    
	    case ANNA_OP_JMP:
	    {
		anna_op_off_t *op = (anna_op_off_t *)code;
		wprintf(L"Jump %d bytes\n\n", op->offset);
		break;
	    }
	    
	    case ANNA_OP_COND_JMP:
	    {
		anna_op_off_t *op = (anna_op_off_t *)code;
		wprintf(L"Conditionally jump %d bytes\n\n", op->offset);
		break;
	    }
	    
	    
	    case ANNA_OP_NCOND_JMP:
	    {
		anna_op_off_t *op = (anna_op_off_t *)code;
		wprintf(L"Conditionally not jump %d bytes\n\n", op->offset);
		break;
	    }
	    
	    case ANNA_OP_TRAMPOLENE:
	    {
		wprintf(L"Create trampolene\n\n");
		break;
	    }
	    
	    default:
	    {
		wprintf(L"Unknown opcode %d during print\n", instruction);
		CRASH;
	    }
	}
	code += anna_bc_op_size(*code);
    }
}

void anna_vm_mark_code(anna_function_t *f)
{
    char *code = f->code;
    while(1)
    {
	char instruction = *code;
	switch(instruction)
	{
	    case ANNA_OP_STRING:
	    case ANNA_OP_CONSTANT:
	    {
		anna_op_const_t *op = (anna_op_const_t*)code;
		anna_alloc_mark_object(op->value);
		break;
	    }

	    case ANNA_OP_LIST:
	    case ANNA_OP_CAST:
	    {
		anna_op_type_t *op = (anna_op_type_t*)code;
		anna_alloc_mark_object(op->value);
		break;
	    }
	    
	    case ANNA_OP_RETURN:
	    case ANNA_OP_STOP:
	    {
		return;
	    }
	    
	    case ANNA_OP_FOLD:
	    case ANNA_OP_CALL:
	    case ANNA_OP_CONSTRUCT:
	    case ANNA_OP_VAR_GET:
	    case ANNA_OP_VAR_SET:
	    case ANNA_OP_STATIC_MEMBER_GET:
	    case ANNA_OP_MEMBER_GET:
	    case ANNA_OP_MEMBER_GET_THIS:
	    case ANNA_OP_MEMBER_SET:
	    case ANNA_OP_POP:
	    case ANNA_OP_NOT:
	    case ANNA_OP_DUP:
	    case ANNA_OP_JMP:
	    case ANNA_OP_COND_JMP:
	    case ANNA_OP_NCOND_JMP:
	    case ANNA_OP_TRAMPOLENE:
	    {
		break;
	    }
	    
	    default:
	    {
		wprintf(L"Unknown opcode %d during GC\n", instruction);
		CRASH;
	    }
	}
	code += anna_bc_op_size(*code);
    }
}

size_t anna_bc_stack_size(char *code)
{
    size_t pos = 0;
    size_t max = 0;
    char *base = code;
    while(1)
    {
	char instruction = *code;
	switch(instruction)
	{
	    case ANNA_OP_STRING:
	    case ANNA_OP_CONSTANT:
	    case ANNA_OP_LIST:
	    case ANNA_OP_CONSTRUCT:
	    case ANNA_OP_VAR_GET:
	    case ANNA_OP_MEMBER_GET_THIS:
	    case ANNA_OP_DUP:
	    {
		pos++;
		break;
	    }
	    
	    case ANNA_OP_FOLD:
	    case ANNA_OP_MEMBER_SET:
	    case ANNA_OP_POP:
	    {
		pos--;
		break;
	    }

	    case ANNA_OP_CALL:
	    {
		anna_op_call_t *op = (anna_op_call_t *)code;
		size_t param = op->param;
		pos -= param;
		break;
	    }
	    
	    case ANNA_OP_RETURN:
	    case ANNA_OP_STOP:
	    {
		return max;
	    }
	    
	    case ANNA_OP_VAR_SET:
	    case ANNA_OP_MEMBER_GET:
	    case ANNA_OP_STATIC_MEMBER_GET:
	    case ANNA_OP_NOT:
	    case ANNA_OP_JMP:
	    case ANNA_OP_COND_JMP:
	    case ANNA_OP_NCOND_JMP:
	    case ANNA_OP_TRAMPOLENE:
	    case ANNA_OP_CAST:
	    {
		break;
	    }
	    
	    default:
	    {
		wprintf(L"Unknown opcode %d during size calculation\n", instruction);
		CRASH;
	    }
	}
	max = maxi(max, pos);
	code += anna_bc_op_size(*code);
    }
}

static size_t anna_vm_size(anna_function_t *fun, anna_node_t *node)
{
    switch(node->node_type)
    {

	case ANNA_NODE_TYPE:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_NULL:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	{
	    return sizeof(anna_op_const_t);
	}

	case ANNA_NODE_CLOSURE:
	{
	    return sizeof(anna_op_const_t) + sizeof(anna_op_null_t);
	}

	case ANNA_NODE_DECLARE:
	{
	    anna_node_declare_t *node2 = (anna_node_declare_t *)node;
	    return sizeof(anna_op_var_t) + 
		anna_vm_size(fun, node2->value);;
	}

	case ANNA_NODE_IF:
	{
	    anna_node_if_t *node2 = (anna_node_if_t *)node;

	    size_t res = anna_vm_size(fun, node2->cond) + 
		sizeof(anna_op_off_t) +
		anna_vm_size(fun, (anna_node_t *)node2->block1) +
		sizeof(anna_op_call_t) +
		sizeof(anna_op_off_t) +
		anna_vm_size(fun, (anna_node_t *)node2->block2) +
		sizeof(anna_op_call_t);
	    return res;
	}

	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;
	    return anna_vm_size(fun, node2->arg1) +
		sizeof(anna_op_off_t) +
		2*sizeof(anna_op_null_t) +
		anna_vm_size(fun, node2->arg2);
	}

	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;
	    return anna_vm_size(fun, node2->arg1) +
		sizeof(anna_op_off_t) +
		2*sizeof(anna_op_null_t) +
		anna_vm_size(fun, node2->arg2);
	}

	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;
	    
	    size_t sz1 = anna_vm_size(fun, node2->arg1);
	    size_t sz2 = anna_vm_size(fun, node2->arg2);

	    return sz1 + sz2 + sizeof(anna_op_off_t)*2 + sizeof(anna_op_call_t) + sizeof(anna_op_const_t) + sizeof(anna_op_null_t);
	}

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *node2 = (anna_node_identifier_t *)node;
	    if(anna_stack_get_ro(fun->stack_template, node2->name))
	    {
		return sizeof(anna_op_const_t);
	    }
	    
	    anna_stack_template_t *import = anna_stack_template_search(fun->stack_template, node2->name);
	    if(import->is_namespace)
	    {
		return sizeof(anna_op_const_t)+ sizeof(anna_op_member_t);
	    }

	    return sizeof(anna_op_var_t);
	    
	    break;
	}
	
	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    
	    return anna_vm_size(fun, node2->child[0]) + sizeof(anna_op_type_t);
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;

	    size_t res = 
		anna_vm_size(fun, node2->function) + sizeof(anna_op_call_t);

	    anna_function_type_key_t *template;
	    int ra;
	    if(node->node_type==ANNA_NODE_CALL)
	    {
		template = anna_function_type_extract(
		    node2->function->return_type);
		ra = template->argc;
	    }
	    else
	    {
		anna_node_type_t *tn = (anna_node_type_t *)node2->function;
//		anna_type_print(tn->payload);
		
		anna_object_t **constructor_ptr = anna_static_member_addr_get_mid(
		    tn->payload,
		    ANNA_MID_INIT_PAYLOAD);
		assert(constructor_ptr);
		template = anna_function_type_extract(
		    (*constructor_ptr)->type);
		res += sizeof(anna_op_null_t);
		ra = template->argc-1;
	    }
	    
	    int i;	    
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		ra--;
	    }


	    for(i=0; i<ra; i++)
	    {
		res += anna_vm_size(fun, node2->child[i]);
	    }
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		res += sizeof(anna_op_type_t);
		
		for(; i<node2->child_count; i++)
		{
		    res += anna_vm_size(fun, node2->child[i]);
		    res += sizeof(anna_op_null_t);
		}
	    }

	    return res;
	}

	case ANNA_NODE_ASSIGN:
	{
	    
	    anna_node_assign_t *node2 = (anna_node_assign_t *)node;
	    size_t res = anna_vm_size(fun, node2->value);
	    anna_stack_template_t *import = anna_stack_template_search(fun->stack_template, node2->name);
	    if(import->is_namespace)
	    {
		return res + 
		    sizeof(anna_op_const_t) +
		    sizeof(anna_op_member_t);
		break;
	    }
	    return res + 
		sizeof(anna_op_var_t);

	    break;
	}
	
	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_get_t *node2 = (anna_node_member_get_t *)node;
	    return anna_vm_size(fun, node2->object) + sizeof(anna_op_member_t);
	}
	
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_set_t *node2 = (anna_node_member_set_t *)node;
	    return anna_vm_size(fun, node2->object) + anna_vm_size(fun, node2->value) + sizeof(anna_op_member_t);
	}
		
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_member_call_t *node2 = (anna_node_member_call_t *)node;
	    size_t res = 
		anna_vm_size(fun, node2->object) + 
		sizeof(anna_op_call_t) + sizeof(anna_op_member_t);
	    
	    anna_type_t *obj_type = node2->object->return_type;
	    anna_member_t *mem = anna_member_get(obj_type, node2->mid);
	    
	    anna_function_type_key_t *template = anna_function_type_extract(
		mem->type);
	    
	    int i;
	    
	    int ra = template->argc-1;
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		res += anna_vm_size(fun, node2->child[i]);
	    }
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		res += sizeof(anna_op_type_t);
		
		for(; i<node2->child_count; i++)
		{
		    res += anna_vm_size(fun, node2->child[i]);
		    res += sizeof(anna_op_null_t);
		}
	    }
	    
	    return res;
	}	
	
	default:
	{
	    wprintf(L"Unknown AST node %d\n", node->node_type);
	    CRASH;
	}
    }
}

static void anna_vm_call(char **ptr, int op, int argc)
{
    anna_op_call_t cop = 
	{
	    op,
	    argc
	}
    ;
    memcpy(*ptr, &cop, sizeof(anna_op_call_t));
    *ptr += sizeof(anna_op_call_t);	    
}

static void anna_vm_const(char **ptr, anna_object_t *val)
{
    anna_op_const_t op = 
	{
	    ANNA_OP_CONSTANT,
	    val
	}
    ;
    memcpy(*ptr, &op, sizeof(anna_op_const_t));
    *ptr += sizeof(anna_op_const_t);	    
}

static void anna_vm_string(char **ptr, anna_object_t *val)
{
    anna_op_const_t op = 
	{
	    ANNA_OP_STRING,
	    val
	}
    ;
    memcpy(*ptr, &op, sizeof(anna_op_const_t));
    *ptr += sizeof(anna_op_const_t);	    
}

static void anna_vm_member(char **ptr, int op, mid_t val)
{
    anna_op_member_t mop = 
	{
	    op,
	    val
	}
    ;
    memcpy(*ptr, &mop, sizeof(anna_op_const_t));
    *ptr += sizeof(anna_op_member_t);	    
}

static void anna_vm_type(char **ptr, int op, anna_type_t *val)
{
    anna_op_type_t lop = 
	{
	    op,
	    val
	}
    ;
    memcpy(*ptr, &lop, sizeof(anna_op_type_t));
    *ptr += sizeof(anna_op_type_t);	    
}

static void anna_vm_jmp(char **ptr, int op, ssize_t offset)
{
    anna_op_off_t jop = 
	{
	    op,
	    offset
	}
    ;
    memcpy(*ptr, &jop, sizeof(anna_op_off_t));
    *ptr += sizeof(anna_op_off_t);
}

static void anna_vm_var(char **ptr, int op, size_t frame, size_t offset)
{
    anna_op_var_t vop = 
	{
	    op,
	    frame,
	    offset
	}
    ;
    memcpy(*ptr, &vop, sizeof(anna_op_var_t));
    *ptr += sizeof(anna_op_var_t);	    
}

static void anna_vm_null(char **ptr, int op)
{
    anna_op_null_t jop = 
	{
	    op,
	}
    ;
    memcpy(*ptr, &jop, sizeof(anna_op_null_t));
    *ptr += sizeof(anna_op_null_t);
}

static void anna_vm_compile_i(anna_function_t *fun, anna_node_t *node, char **ptr, int drop_output)
{
    switch(node->node_type)
    {
	case ANNA_NODE_NULL:
	{
	    anna_vm_const(ptr, null_object);
	    break;
	}

	case ANNA_NODE_DECLARE:
	{
	    anna_node_declare_t *node2 = (anna_node_declare_t *)node;

	    anna_vm_compile_i(fun, node2->value, ptr, 0);	    
	    anna_sid_t sid = anna_stack_sid_create(fun->stack_template, node2->name);	    
	    anna_vm_var(ptr, ANNA_OP_VAR_SET, 0, sid.offset);
	    break;
	}

	case ANNA_NODE_DUMMY:
	{
	    anna_node_dummy_t *node2 = (anna_node_dummy_t *)node;
	    anna_vm_const(ptr,node2->payload);
	    break;
	}

	case ANNA_NODE_INT_LITERAL:
	{
	    anna_node_int_literal_t *node2 = (anna_node_int_literal_t *)node;
	    anna_vm_const(ptr,anna_int_create(node2->payload));
	    break;
	}

	case ANNA_NODE_CHAR_LITERAL:
	{
	    anna_node_char_literal_t *node2 = (anna_node_char_literal_t *)node;
	    anna_vm_const(ptr,anna_char_create(node2->payload));
	    break;
	}

	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_node_float_literal_t *node2 = (anna_node_float_literal_t *)node;
	    anna_vm_const(ptr,anna_float_create(node2->payload));
	    break;
	}

	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *node2 = (anna_node_type_t *)node;
	    anna_vm_const(ptr,anna_type_wrap(node2->payload));
	    break;
	}

	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *node2 = (anna_node_string_literal_t *)node;
	    anna_vm_string(ptr, anna_string_create(node2->payload_size, node2->payload));
	    break;
	}

	case ANNA_NODE_IF:
	{
	    anna_node_if_t *node2 = (anna_node_if_t *)node;
	    anna_vm_compile_i(fun, node2->cond, ptr, 0);
	    anna_vm_jmp(
		ptr, ANNA_OP_NCOND_JMP, 
		2*sizeof(anna_op_off_t) +
		anna_vm_size(fun, (anna_node_t *)node2->block1) + 
		sizeof(anna_op_call_t));
	    anna_vm_compile_i(fun, (anna_node_t *)node2->block1, ptr, 0);
	    anna_vm_call(ptr, ANNA_OP_CALL, 0);
	    anna_vm_jmp(
		ptr, ANNA_OP_JMP,
		sizeof(anna_op_off_t) + 
		anna_vm_size(fun, (anna_node_t *)node2->block2) +
		sizeof(anna_op_call_t));
	    anna_vm_compile_i(fun, (anna_node_t *)node2->block2, ptr, 0);
	    anna_vm_call(ptr, ANNA_OP_CALL, 0);

	    break;
	}

	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;
	    
	    size_t sz1 = anna_vm_size(fun, node2->arg1);
	    size_t sz2 = anna_vm_size(fun, node2->arg2);
	    
	    
	    anna_vm_const(ptr, null_object); // +1
	    anna_vm_compile_i(fun, node2->arg1, ptr, 0); // +1
	    anna_vm_jmp(
		ptr, ANNA_OP_NCOND_JMP, 
		2*sizeof(anna_op_off_t) +
		sz2 +sizeof(anna_op_null_t) +
		sizeof(anna_op_call_t));  // -1
	    anna_vm_null(ptr, ANNA_OP_POP); // -1
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0); // +1
	    anna_vm_call(ptr, ANNA_OP_CALL, 0); // 0
	    anna_vm_jmp(
		ptr, ANNA_OP_JMP,
		-( sizeof(anna_op_off_t) +sizeof(anna_op_null_t) +
		   sz1 + sz2 +
		   sizeof(anna_op_call_t))); // 0
	    break;
	}

	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;

	    anna_vm_compile_i(fun, node2->arg1, ptr, 0);
	    anna_vm_null(ptr, ANNA_OP_DUP);
	    anna_vm_jmp(
		ptr, ANNA_OP_COND_JMP,
		sizeof(anna_op_off_t) + sizeof(anna_op_null_t) + anna_vm_size(fun, node2->arg2));
	    anna_vm_null(ptr, ANNA_OP_POP);
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0);
	    break;
	}

	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;

	    anna_vm_compile_i(fun, node2->arg1, ptr, 0);
	    
	    anna_vm_null(ptr, ANNA_OP_DUP);
	    anna_vm_jmp(
		ptr,ANNA_OP_NCOND_JMP,
		sizeof(anna_op_off_t) + sizeof(anna_op_null_t) + 
		anna_vm_size(fun, node2->arg2));
	    anna_vm_null( ptr, ANNA_OP_POP);
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0);
	    
	    break;
	}

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *node2 = (anna_node_identifier_t *)node;
	    
	    if(anna_stack_get_ro(fun->stack_template, node2->name))
	    {
		anna_object_t *val = anna_stack_get_str(fun->stack_template, node2->name);
		anna_vm_const(ptr, val);
		break;
	    }

	    anna_stack_template_t *import = anna_stack_template_search(fun->stack_template, node2->name);
	    if(import->is_namespace)
	    {
		
		anna_vm_const(ptr, anna_stack_wrap(import));
		anna_vm_member(ptr, ANNA_OP_STATIC_MEMBER_GET, anna_mid_get(node2->name));
		break;
	    }

	    anna_sid_t sid = anna_stack_sid_create(
		fun->stack_template, node2->name);

	    anna_vm_var(
		ptr,
		ANNA_OP_VAR_GET,
		sid.frame,
		sid.offset);	    
	    
	    break;
	}
	
	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(fun, node2->child[0], ptr, 0);
	    anna_vm_type(ptr, ANNA_OP_CAST, node2->return_type);
	    break;
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(fun, node2->function, ptr, 0);
	    
	    anna_function_type_key_t *template;
	    int ra;
	    if(node->node_type==ANNA_NODE_CALL)
	    {
		template = anna_function_type_extract(
		    node2->function->return_type);
		ra = template->argc;
	    }
	    else
	    {
		anna_node_type_t *tn = (anna_node_type_t *)node2->function;
//		anna_type_print(tn->payload);
		
		anna_object_t **constructor_ptr = anna_static_member_addr_get_mid(
		    tn->payload,
		    ANNA_MID_INIT_PAYLOAD);
		assert(constructor_ptr);
		template = anna_function_type_extract(
		    (*constructor_ptr)->type);
		anna_vm_null(ptr, ANNA_OP_CONSTRUCT);
		ra = template->argc-1;
	    }
	    
	    int i;
	    
	    
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		anna_vm_compile_i(fun, node2->child[i], ptr, 0);		
	    }
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		anna_vm_type(
		    ptr,
		    ANNA_OP_LIST,
		    anna_list_type_get(template->argv[template->argc-1]));
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(fun, node2->child[i], ptr, 0);		
		    anna_vm_null(
			ptr,
			ANNA_OP_FOLD);
		}
	    }
	    
	    //wprintf(L"Woo argc %d\n", template->argc);
	    anna_vm_call(
		ptr,
		ANNA_OP_CALL,
		template->argc);
	    
	    if(node->node_type==ANNA_NODE_CONSTRUCT)
	    {
		//anna_vm_null(ptr, ANNA_OP_POP);
	    }
	    break;
	}

	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *node2 = (anna_node_assign_t *)node;
	    anna_vm_compile_i(fun, node2->value, ptr, 0);
	    anna_stack_template_t *import = anna_stack_template_search(fun->stack_template, node2->name);
	    if(import->is_namespace)
	    {
		anna_vm_const(ptr, anna_stack_wrap(import));
		anna_vm_member(ptr, ANNA_OP_MEMBER_SET, anna_mid_get(node2->name));
		break;
	    }

	    anna_sid_t sid = anna_stack_sid_create(
		fun->stack_template, node2->name);
	    
	    anna_vm_var(
		ptr,
		ANNA_OP_VAR_SET,
		sid.frame,
		sid.offset);	    
	    
	    break;
	}
	
	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_get_t *node2 = (anna_node_member_get_t *)node;
	    anna_vm_compile_i(fun, node2->object, ptr, 0);
	    anna_type_t *type = node2->object->return_type;
	    anna_member_t *m = type->mid_identifier[node2->mid];
	    anna_vm_member(ptr, m->is_static?ANNA_OP_STATIC_MEMBER_GET:ANNA_OP_MEMBER_GET, node2->mid);
	    break;
	}
	
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_set_t *node2 = (anna_node_member_set_t *)node;
	    anna_vm_compile_i(fun, node2->value, ptr, 0);
	    anna_vm_compile_i(fun, node2->object, ptr, 0);
	    anna_vm_member(ptr, ANNA_OP_MEMBER_SET, node2->mid);
	    break;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *node2 = (anna_node_closure_t *)node;
	    anna_vm_const(
		ptr,
		anna_function_wrap(node2->payload));
	    anna_vm_null(
		ptr,
		ANNA_OP_TRAMPOLENE);
	    break;
	}
	
	
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_member_call_t *node2 = (anna_node_member_call_t *)node;
	    anna_vm_compile_i(fun, node2->object, ptr, 0);

	    anna_vm_member(ptr, ANNA_OP_MEMBER_GET_THIS, node2->mid);
	    
	    anna_type_t *obj_type = node2->object->return_type;
	    anna_member_t *mem = anna_member_get(obj_type, node2->mid);
	    
	    anna_function_type_key_t *template = anna_function_type_extract(
		mem->type);
	    
	    int i;
	    
	    int ra = template->argc-1;
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		anna_vm_compile_i(fun, node2->child[i], ptr, 0);		
	    }
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		anna_vm_type(
		    ptr,
		    ANNA_OP_LIST,
		    anna_list_type_get(template->argv[template->argc-1]));
		
		
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(fun, node2->child[i], ptr, 0);		
		    anna_vm_null(ptr, ANNA_OP_FOLD);
		}
	    }
	    
	    anna_vm_call(ptr, ANNA_OP_CALL, template->argc);
	    break;
	}	
	
	default:
	{
	    wprintf(L"Unknown AST node %d\n", node->node_type);
	    CRASH;
	}
    }
    if(drop_output){
	anna_vm_null(ptr, ANNA_OP_POP);
    }

}

void anna_vm_compile(
    anna_function_t *fun)
{
    if(!fun->body)
    {
	fun->variable_count = fun->input_count;
	fun->frame_size = sizeof(anna_vmstack_t) + sizeof(anna_object_t *)*(fun->variable_count);
	return;
    }
    
//    wprintf(L"Compile really awesome function named %ls\n", fun->name);
    
    int i;
    fun->variable_count = fun->stack_template->count;

    int is_empty = fun->body->child_count == 0;
    
    size_t sz;
    if(is_empty)
    {
	sz = anna_bc_op_size(ANNA_OP_CONSTANT) + anna_bc_op_size(ANNA_OP_RETURN);
	
    }
    else
    {
	sz=1;
	for(i=0; i<fun->body->child_count; i++)
	{
	    sz += anna_vm_size(fun, fun->body->child[i]) + sizeof(anna_op_null_t);
	}
    }
    
    fun->code = calloc(sz, 1);
    char *code_ptr = fun->code;
    if(is_empty)
    {
	anna_vm_const(&code_ptr, null_object);
    }
    else
    {
	for(i=0; i<fun->body->child_count; i++)
	{
	    anna_vm_compile_i(fun, fun->body->child[i], &code_ptr, i != (fun->body->child_count-1));
	}
    }
    
    fun->frame_size = sizeof(anna_vmstack_t) + sizeof(anna_object_t *)*(fun->variable_count + anna_bc_stack_size(fun->code));
//    anna_bc_print(fun->code);
}

