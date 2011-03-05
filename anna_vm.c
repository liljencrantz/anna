#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_vm.h"
#include "anna_function.h"
#include "anna_node.h"
#include "anna_int.h"

#define ANNA_OP_CONSTANT 0
#define ANNA_OP_CALL 1
#define ANNA_OP_RETURN 2 
#define ANNA_OP_STOP 3 
#define ANNA_OP_GET_VAR 4
#define ANNA_OP_SET_VAR 5
#define ANNA_OP_GET_MEMBER 6
#define ANNA_OP_SET_MEMBER 7

typedef struct 
{
    char instruction;
    short _padding;
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
    short frame_count;
    int offset;
}
    anna_op_var_t;

typedef struct
{
    char instruction;
    short offset;
}
    anna_op_member_t;

typedef struct
{
    char instruction;
    short param;
}
    anna_op_call_t;

struct anna_vmstack
{
    int flags;
    struct anna_vmstack *parent;    
    anna_function_t *function;
    char *code;    
    anna_object_t **top;
    anna_object_t *base[];
};

typedef struct anna_vmstack anna_vmstack_t;

static void anna_push(anna_vmstack_t **stack, anna_object_t *val)
{
    anna_object_t ** top =((*stack)->top);
    *(top++)= val;
}

static anna_object_t *anna_pop(anna_vmstack_t **stack)
{
    return *((*(--stack))->top);
}

static anna_object_t *anna_peek(anna_vmstack_t **stack, size_t off)
{
    return *((*stack)->top-1-off);
}

static anna_vmstack_t *anna_vmstack_alloc(size_t sz)
{
    anna_vmstack_t *res = calloc(1, sz);
    res->flags = ANNA_VMSTACK;
    return res;
}


static inline void anna_frame_push(
    anna_vmstack_t **stack, anna_function_t *fun)
{
    anna_vmstack_t *res = anna_vmstack_alloc(fun->frame_size);
    res->parent=0;
    res->function = fun;
    res->code = fun->code;
    (*stack)->top -= fun->input_count;
    memcpy(&res->base[0], (*stack)->top, sizeof(anna_object_t *)*fun->input_count);
    res->top = &res->base[fun->variable_count];
    *(++stack) = res;
}

static inline void anna_frame_pop(
    anna_vmstack_t **stack)
{
    anna_object_t *val = anna_peek(stack, 0);
    --stack;
    anna_push(stack, val);
}

void anna_vm_run(anna_function_t *entry)
{
    anna_vmstack_t **stack_mem = malloc(sizeof(anna_vmstack_t *)*1024);
    anna_vmstack_t **stack = stack_mem;
    *stack = anna_vmstack_alloc(stack_global->count);
    memcpy(
	&(*stack)->base[0],
	&stack_global->member[0],
	sizeof(anna_object_t *)*stack_global->count);
    stack++;
    
    while(1)
    {
	char instruction = *(*stack)->code;
	switch(instruction)
	{
	    case ANNA_OP_CONSTANT:
	    {
		anna_op_const_t *op = (anna_op_const_t *)(*stack)->code;
		anna_push(stack, op->value);
		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_CALL:
	    {
		anna_op_call_t *op = (anna_op_call_t *)(*stack)->code;
		size_t param = op->param;
		anna_object_t *wrapped = anna_peek(stack, param+1);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		
		if(fun->native.function)
		{
		    anna_object_t *res = fun->native.function(
			((*stack)->top-1-param));
		    (*stack)->top -= (param+1);
		    anna_push(stack, res);
		    (*stack)->code += sizeof(*op);		    
		}
		else
		{
		    (*stack)->code += sizeof(*op);
		    anna_frame_push(stack, fun);
		}
		break;
	    }
	    
	    case ANNA_OP_RETURN:
	    {
		anna_frame_pop(stack);		
		break;
	    }
	    
	    case ANNA_OP_STOP:
	    {
		free(stack_mem);
		return;
	    }
	    
	    case ANNA_OP_GET_VAR:
	    {
		anna_op_var_t *op = (anna_op_var_t *)(*stack)->code;
		int i;
		anna_vmstack_t *s = *stack;
		for(i=0; i<op->frame_count; i++)
		    s = s->parent;
		anna_push(stack, s->base[op->offset]);
		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_SET_VAR:
	    {
		anna_op_var_t *op = (anna_op_var_t *)(*stack)->code;
		int i;
		anna_vmstack_t *s = *stack;
		for(i=0; i<op->frame_count; i++)
		    s = s->parent;
		s->base[op->offset] = anna_pop(stack);
		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_GET_MEMBER:
	    {
		anna_op_member_t *op = (anna_op_member_t *)(*stack)->code;
		anna_object_t *obj = anna_peek(stack, 0);
		anna_push(stack, obj->member[op->offset]);
		(*stack)->code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_SET_MEMBER:
	    {
		anna_op_member_t *op = (anna_op_member_t *)(*stack)->code;
		anna_object_t *obj = anna_peek(stack, 1);
		obj->member[op->offset] = anna_pop(stack);
		(*stack)->code += sizeof(*op);
		break;
	    }
	}
    }
}

void anna_bc_print(char *code)
{
    wprintf(L"Code:\n");
    
    while(1)
    {
//	wprintf(L"Read instruction at %d\n", code);
	char instruction = *code;
	switch(instruction)
	{
	    case ANNA_OP_CONSTANT:
	    {
		anna_op_const_t *op = (anna_op_const_t*)code;
		wprintf(L"Push constant of type %ls\n", op->value->type->name);
		code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_CALL:
	    {
		anna_op_call_t *op = (anna_op_call_t *)code;
		size_t param = op->param;
		wprintf(L"Invoke function with %d parameters:\n", param);
		code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_RETURN:
	    {
		wprintf(L"Return\n");
		return;
	    }
	    
	    case ANNA_OP_STOP:
	    {
		wprintf(L"Stop\n");
		return;
	    }
	    
	    case ANNA_OP_GET_VAR:
	    {
		anna_op_var_t *op = (anna_op_var_t *)code;
		wprintf(L"Get var %d : %d\n", op->frame_count, op->offset);
		code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_SET_VAR:
	    {
		anna_op_var_t *op = (anna_op_var_t *)code;
		wprintf(L"Set var %d : %d\n", op->frame_count, op->offset);
		code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_GET_MEMBER:
	    {
		anna_op_member_t *op = (anna_op_member_t *)code;
		wprintf(L"Get member %d\n", op->offset);
		code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_SET_MEMBER:
	    {
		anna_op_member_t *op = (anna_op_member_t *)code;
		wprintf(L"Set member %d\n", op->offset);
		code += sizeof(*op);
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

static void anna_vm_compile_i(anna_node_t *node, char **ptr)
{
    switch(node->node_type)
    {
	case ANNA_NODE_INT_LITERAL:
	{
	    anna_node_int_literal_t *node2 = (anna_node_int_literal_t *)node;
	    anna_op_const_t op = 
		{
		    ANNA_OP_CONSTANT,
		    anna_int_create(node2->payload)
		}
	    ;
	    memcpy(*ptr, &op, sizeof(anna_op_const_t));
//	    wprintf(L"Write instruction to %d\n", *ptr);
	    *ptr += sizeof(anna_op_const_t);
//	    wprintf(L"next instruction is at %d\n", *ptr);
	    break;
	}
    }
}

void anna_vm_compile(
    anna_function_t *fun)
{
    int i;
    fun->variable_count = fun->stack_template->count;
    fun->frame_size = 128;
    fun->code = malloc(4096);
    char *code_ptr = fun->code;
    for(i=0; i<fun->body->child_count; i++)
    {
	anna_vm_compile_i(fun->body->child[i], &code_ptr);
    }
    *code_ptr = ANNA_OP_RETURN;
    anna_bc_print(fun->code);
}

