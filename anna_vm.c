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
#include "anna_list.h"
#include "anna_int.h"
#include "anna_float.h"
#include "anna_string.h"
#include "anna_char.h"
#include "anna_stack.h"
#include "anna_function_type.h"

#define ANNA_OP_CONSTANT 0
#define ANNA_OP_CALL 1
#define ANNA_OP_RETURN 2 
#define ANNA_OP_STOP 3 
#define ANNA_OP_GET_VAR 4
#define ANNA_OP_SET_VAR 5
#define ANNA_OP_GET_MEMBER 6
#define ANNA_OP_SET_MEMBER 7
#define ANNA_OP_STRING 8
#define ANNA_OP_LIST 9
#define ANNA_OP_FOLD 10

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

static anna_vmstack_t *anna_vmstack_alloc(size_t sz)
{
    anna_vmstack_t *res = calloc(1, sz);
    res->flags = ANNA_VMSTACK;
    return res;
}


#define anna_frame_push(stack, fun) {					\
    anna_vmstack_t *res = anna_vmstack_alloc(fun->frame_size);		\
    res->parent=0;							\
    res->function = fun;						\
    res->code = fun->code;						\
    (*stack)->top -= fun->input_count;					\
    memcpy(&res->base[0], (*stack)->top, sizeof(anna_object_t *)*fun->input_count); \
    res->top = &res->base[fun->variable_count];				\
    *(++stack) = res;							\
}

#define anna_frame_pop(stack) {anna_object_t *val = anna_peek(stack, 0);--stack;anna_push(stack, val);}

void anna_vm_run(anna_function_t *entry)
{
    anna_vmstack_t **stack_mem = malloc(sizeof(anna_vmstack_t *)*1024);
    anna_vmstack_t **stack = stack_mem;
    *stack = anna_vmstack_alloc(stack_global->count*sizeof(anna_object_t *) + sizeof(anna_vmstack_t));
    memcpy(
	&((*stack)->base[0]),
	&(stack_global->member[0]),
	sizeof(anna_object_t *)*stack_global->count);
    (*stack)->code = malloc(1);
    *(*stack)->code = ANNA_OP_STOP;
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
	    
	    case ANNA_OP_CALL:
	    {
		anna_op_call_t *op = (anna_op_call_t *)(*stack)->code;
		size_t param = op->param;
		anna_object_t *wrapped = anna_peek(stack, param);
		
		anna_function_t *fun = anna_function_unwrap(wrapped);
		
		if(fun->native.function)
		{
		    anna_object_t *res = fun->native.function(
			((*stack)->top-param));
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
/*		wprintf(
		    L"Fold value of type %ls to list %d\n", 
		    val->type->name, 
		    anna_peek(stack, 0));		
*/
		anna_list_add(anna_peek(stack, 0), val);
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

void anna_bc_print(char *code)
{
    wprintf(L"Code:\n");
    
    while(1)
    {
//	wprintf(L"Read instruction at %d\n", code);
	char instruction = *code;
	switch(instruction)
	{
	    case ANNA_OP_STRING:
	    case ANNA_OP_CONSTANT:
	    {
		anna_op_const_t *op = (anna_op_const_t*)code;
		wprintf(L"Push constant of type %ls\n", op->value->type->name);
		code += sizeof(*op);
		break;
	    }
	    
	    case ANNA_OP_LIST:
	    {
		wprintf(L"List creation\n");
		code += sizeof(anna_op_type_t);
		break;
	    }
	    
	    case ANNA_OP_FOLD:
	    {
		wprintf(L"List fold\n");
		code += sizeof(anna_op_null_t);
		break;
	    }
	    
	    case ANNA_OP_CALL:
	    {
		anna_op_call_t *op = (anna_op_call_t *)code;
		size_t param = op->param;
		wprintf(L"Invoke function with %d parameter(s)\n", param);
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

static void anna_vm_compile_i(anna_function_t *fun, anna_node_t *node, char **ptr)
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

	case ANNA_NODE_FLOAT_LITERAL:
	{
	    anna_node_float_literal_t *node2 = (anna_node_float_literal_t *)node;
	    anna_op_const_t op = 
		{
		    ANNA_OP_CONSTANT,
		    anna_float_create(node2->payload)
		}
	    ;
	    memcpy(*ptr, &op, sizeof(anna_op_const_t));
//	    wprintf(L"Write instruction to %d\n", *ptr);
	    *ptr += sizeof(anna_op_const_t);
//	    wprintf(L"next instruction is at %d\n", *ptr);
	    break;
	}

	case ANNA_NODE_STRING_LITERAL:
	{
	    anna_node_string_literal_t *node2 = (anna_node_string_literal_t *)node;
	    anna_op_const_t op = 
		{
		    ANNA_OP_STRING,
		    anna_string_create(node2->payload_size, node2->payload)
		}
	    ;
	    memcpy(*ptr, &op, sizeof(anna_op_const_t));
//	    wprintf(L"Write instruction to %d\n", *ptr);
	    *ptr += sizeof(anna_op_const_t);
//	    wprintf(L"next instruction is at %d\n", *ptr);
	    break;
	}

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *node2 = (anna_node_identifier_t *)node;
	    int distance = 0;
	    anna_object_t *val = anna_stack_get_const(fun->stack_template, node2->name);
	    if(val)
	    {
		anna_op_const_t op = 
		    {
			ANNA_OP_CONSTANT,
			val
		    }
		;
		memcpy(*ptr, &op, sizeof(anna_op_const_t));
		*ptr += sizeof(anna_op_const_t);	    
		
		break;
	    }
	    CRASH;
	    break;
	}
	
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(fun, node2->function, ptr);
	    
	    anna_function_type_key_t *template = anna_function_type_extract(
		node2->function->return_type);
	    
	    int i;
	    
	    int ra = template->argc;
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		ra--;
	    }
	    for(i=0; i<ra; i++)
	    {
		anna_vm_compile_i(fun, node2->child[i], ptr);		
	    }
	    if(template->flags & ANNA_FUNCTION_VARIADIC)
	    {
		anna_op_type_t lop = 
		    {
			ANNA_OP_LIST,
			anna_list_type_get(template->argv[template->argc-1])
		    }
		;
		memcpy(*ptr, &lop, sizeof(anna_op_type_t));
		*ptr += sizeof(anna_op_type_t);	    
		
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(fun, node2->child[i], ptr);		
		    anna_op_null_t fop = 
			{
			    ANNA_OP_FOLD,
			}
		    ;
		    memcpy(*ptr, &fop, sizeof(anna_op_null_t));
		    *ptr += sizeof(anna_op_null_t);
		}
	    }

	    //wprintf(L"Woo argc %d\n", template->argc);
	    
	    anna_op_call_t op = 
		{
		    ANNA_OP_CALL,
		    template->argc
		}
	    ;
	    memcpy(*ptr, &op, sizeof(anna_op_call_t));
	    *ptr += sizeof(anna_op_call_t);	    
	    
	    break;
	}
	
	default:
	{
	    wprintf(L"Unknown AST node %d\n", node->node_type);
	    CRASH;
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
	anna_vm_compile_i(fun, fun->body->child[i], &code_ptr);
    }
    *code_ptr = ANNA_OP_RETURN;
    anna_bc_print(fun->code);
}

