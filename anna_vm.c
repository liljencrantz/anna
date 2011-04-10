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
#define ANNA_INSTR_RETURN 0 
/**
   Pushes a single constant value onto the stack.
 */
#define ANNA_INSTR_CONSTANT 1
/**
   Calls a function. Pops the number of parameters specified in the
   op, then pops the function. The parameters are copied onto a new
   stack frame. 
 */
#define ANNA_INSTR_CALL 2
/**
   Stop bytecode execution and return.
 */
#define ANNA_INSTR_STOP 3 
/**
   Pushes the specified variable to the stack
 */
#define ANNA_INSTR_VAR_GET 4
/**
   Copies the value on the top of the stack to the specified location
   without poping it.
 */
#define ANNA_INSTR_VAR_SET 5
/**
   Pops an object from the stack and pushes the object member
   specified by the op instead.
 */
#define ANNA_INSTR_MEMBER_GET 6
#define ANNA_INSTR_STATIC_MEMBER_GET 7
/**
   Pops a value and an object from the stack and pushes the object member
   specified by the op instead.
 */
#define ANNA_INSTR_MEMBER_SET 8
/**
   Create a new string based on a string literal
 */
#define ANNA_INSTR_STRING 9
/**
   Push an empty list to the stack
 */
#define ANNA_INSTR_LIST 10
/**
  Pop the top value of the stack, and insert it into the new top element, which is assumed to be a list object
 */
#define ANNA_INSTR_FOLD 11
/**
   Pop value from stack, jump if not null
 */
#define ANNA_INSTR_COND_JMP 12
/**
   Pop value from stack, jump if null
 */
#define ANNA_INSTR_NCOND_JMP 13
/**
   Pop value from stack
 */
#define ANNA_INSTR_POP 14
/**
   Negate top value on stack
 */
#define ANNA_INSTR_NOT 15
/**
   Push a duplicate of the current top stack value to the top of the stack
 */
#define ANNA_INSTR_DUP 16
/**
   Pop value from stack, push the specified member of the popped object to the stack, and then push back the original object popped as well. 

   (This is useful when calling a method, we go from OBJ to METHOD, OBJ, which is nifty when we call method, as obj will be the this value)
 */
#define ANNA_INSTR_MEMBER_GET_THIS 17
/**
   Unconditionally jump the specified offset
 */
#define ANNA_INSTR_JMP 18
/**
   Pop value from stack, assumed to be a closure. Push a trampolene for the specified value.
 */
#define ANNA_INSTR_TRAMPOLENE 19
/**
   Pop the top value from the stack (a type) and push a newly allocated (unconstructed) object of the specified type to the stack
 */
#define ANNA_INSTR_CONSTRUCT 20
/**
   If the top value of the stack abides to the type specified in the op, do nothing. Otherwise, replace the current top stack value with the null object.
 */
#define ANNA_INSTR_CAST 21
#define ANNA_INSTR_NATIVE_CALL 22
#define ANNA_INSTR_RETURN_COUNT 23



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
    anna_op_count_t;

typedef struct
{
    char instruction;
    ssize_t offset;
}
    anna_op_off_t;

typedef struct
{
    char instruction;
    anna_native_function_t function;
}
    anna_op_native_call_t;

/*
typedef struct
{
    char instruction;
    anna_vm_callback_t callback;
    void *aux1;
    void *aux2;
    void *aux3;
}
    anna_op_callback_t;
*/
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

static void anna_vmstack_print_parent(anna_vmstack_t *stack)
{
    if(!stack)
	return;
    anna_vmstack_print_parent(stack->parent);
    wprintf(
	L"Function %ls, offset %d\n", 
	stack->function?stack->function->name:L"<null>", 
	stack->function? (stack->code - stack->function->code): -1);
    
}


static inline anna_vmstack_t *anna_frame_push(anna_vmstack_t *caller, anna_object_t *wfun) {
    size_t stack_offset = wfun->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_STACK]->offset;
    anna_vmstack_t *parent = *(anna_vmstack_t **)&wfun->member[stack_offset];
    anna_function_t *fun = anna_function_unwrap(wfun);
    anna_vmstack_t *res = anna_alloc_vmstack(fun->frame_size);
    res->parent=parent;
    res->caller = caller;
    res->function = fun;
    res->code = fun->code;
    caller->top -= (fun->input_count+1);
    memcpy(&res->base[0], caller->top+1,
	   sizeof(anna_object_t *)*fun->input_count);
    memset(&res->base[fun->input_count], 0, sizeof(anna_object_t *)*(fun->variable_count-fun->input_count));
    res->top = &res->base[fun->variable_count];
    
    return res;
}

void anna_vm_init()
{
}

#ifdef ANNA_FULL_GC_ON_SHUTDOWN
void anna_vm_destroy(void)
{
    free(stack_mem);
}
#endif


anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_object_t **argv)
{
    static void *jump_label[] = 
	{
	    &&ANNA_LAB_RETURN, 
	    &&ANNA_LAB_CONSTANT,
	    &&ANNA_LAB_CALL,
	    &&ANNA_LAB_STOP,
	    &&ANNA_LAB_VAR_GET,
	    &&ANNA_LAB_VAR_SET,
	    &&ANNA_LAB_MEMBER_GET,
	    &&ANNA_LAB_STATIC_MEMBER_GET,
	    &&ANNA_LAB_MEMBER_SET,
	    &&ANNA_LAB_STRING,
	    &&ANNA_LAB_LIST,
	    &&ANNA_LAB_FOLD,
	    &&ANNA_LAB_COND_JMP,
	    &&ANNA_LAB_NCOND_JMP,
	    &&ANNA_LAB_POP,
	    &&ANNA_LAB_NOT,
	    &&ANNA_LAB_DUP,
	    &&ANNA_LAB_MEMBER_GET_THIS,
	    &&ANNA_LAB_JMP,
	    &&ANNA_LAB_TRAMPOLENE,
	    &&ANNA_LAB_CONSTRUCT,
	    &&ANNA_LAB_CAST,
	    &&ANNA_LAB_NATIVE_CALL,
	    &&ANNA_LAB_RETURN_COUNT
	}
    ;

    static int vm_count = 0;
    int is_root = vm_count==0;
    vm_count++;
    
    anna_vmstack_t *stack;    
    stack = calloc(1, (argc+1)*sizeof(anna_object_t *) + sizeof(anna_vmstack_t));
    stack->flags = ANNA_VMSTACK;
    al_push(&anna_alloc, stack);
    
    stack->caller = 0;
    
    stack->parent = *(anna_vmstack_t **)anna_member_addr_get_mid(entry,ANNA_MID_FUNCTION_WRAPPER_STACK);
    
    stack->function = 0;
    stack->top = &stack->base[0];
    stack->code = malloc(1);
    *(stack->code) = ANNA_INSTR_STOP;
    
    int i;
    anna_vmstack_push(stack, entry);
    for(i=0; i<argc; i++)
    {
	anna_vmstack_push(stack, argv[i]);
    }
    anna_function_t *root_fun = anna_function_unwrap(entry);
    stack = root_fun->native.function(stack, entry);
    goto *jump_label[*stack->code];

  ANNA_LAB_CONSTANT:
    {
	anna_op_const_t *op = (anna_op_const_t *)stack->code;
	anna_vmstack_push(stack, op->value);
	stack->code += sizeof(*op);
	goto *jump_label[*stack->code];
    }
    
  ANNA_LAB_STRING:
    {
	anna_op_const_t *op = (anna_op_const_t *)stack->code;
	anna_vmstack_push(stack, anna_string_copy(op->value));
	stack->code += sizeof(*op);
	goto *jump_label[*stack->code];
    }
    
  ANNA_LAB_CAST:	    
    {
	
	anna_op_type_t *op = (anna_op_type_t *)stack->code;
	if(!anna_abides(anna_vmstack_peek(stack,0)->type, op->value))
	{
	    anna_vmstack_pop(stack);
	    anna_vmstack_push(stack, null_object);
	}
	
	stack->code += sizeof(*op);
	goto *jump_label[*stack->code];
    }
    
  ANNA_LAB_CALL:
    {
	
	if(unlikely(anna_alloc_count > GC_FREQ))
	{
	    if(is_root)
	    {
		anna_alloc_count=0;
		anna_gc(stack);
	    }
	}
	anna_op_count_t *op = (anna_op_count_t *)stack->code;
	size_t param = op->param;
	anna_object_t *wrapped = anna_vmstack_peek(stack, param);
	anna_function_t *fun = anna_function_unwrap(wrapped);
	
#ifdef ANNA_CHECK_VM
	if(!fun)
	{
	    wprintf(L"Error: Tried to call something that is not a function with %d params. Stack contents:\n", param);
	    anna_vmstack_print(stack);
	    CRASH;
	}
#endif
		
	stack->code += sizeof(*op);
	stack = fun->native.function(stack, wrapped);

	goto *jump_label[*stack->code];
    }
    
    ANNA_LAB_CONSTRUCT:
    {
	anna_op_null_t *op = (anna_op_null_t *)stack->code;
	anna_object_t *wrapped = anna_vmstack_pop(stack);
	
	anna_type_t *tp = anna_type_unwrap(wrapped);
	
	anna_object_t *result = anna_object_create(tp);
	
	anna_object_t **constructor_ptr = anna_static_member_addr_get_mid(
	    tp,
	    ANNA_MID_INIT_PAYLOAD);
	anna_vmstack_push(stack, *constructor_ptr);
	anna_vmstack_push(stack, result);
	stack->code += sizeof(*op);
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_RETURN:
    {
	anna_object_t *val = anna_vmstack_peek(stack, 0);
	stack = stack->caller;
	anna_vmstack_push(stack, val);
//		wprintf(L"Pop frame\n");
	goto *jump_label[*stack->code];
    }
    
    ANNA_LAB_RETURN_COUNT:
    {
	anna_op_count_t *cb = (anna_op_count_t *)stack->code;
	anna_object_t *val = anna_vmstack_peek(stack, 0);
	int i;
		
	for(i=0; i<cb->param; i++)
	{
	    stack = stack->parent;
	}
	stack = stack->caller;
	anna_vmstack_push(stack, val);
	goto *jump_label[*stack->code];
    }

    ANNA_LAB_NATIVE_CALL:
    {
	anna_op_native_call_t *cb = (anna_op_native_call_t *)stack->code;
	stack->code += sizeof(*cb);

	stack = cb->function(stack, 0);
	goto *jump_label[*stack->code];
    }

    ANNA_LAB_STOP:
    {
//		wprintf(L"Pop last frame\n");
	anna_object_t *val = anna_vmstack_peek(stack, 0);
	free(stack->code);
	stack = stack->caller;
	return val;
    }
	    
    ANNA_LAB_VAR_GET:
    {
	anna_op_var_t *op = (anna_op_var_t *)stack->code;
	int i;
	anna_vmstack_t *s = stack;
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
	anna_vmstack_push(stack, s->base[op->offset]);
	stack->code += sizeof(*op);
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_VAR_SET:
    {
	anna_op_var_t *op = (anna_op_var_t *)stack->code;
	int i;
	anna_vmstack_t *s = stack;
	for(i=0; i<op->frame_count; i++)
	    s = s->parent;
	s->base[op->offset] = anna_vmstack_peek(stack, 0);
	stack->code += sizeof(*op);
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_MEMBER_GET:
    {
	anna_op_member_t *op = (anna_op_member_t *)stack->code;
	anna_object_t *obj = anna_vmstack_pop(stack);

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

	    anna_vmstack_push(stack, method);
	    anna_vmstack_push(stack, obj);
	    stack->code += sizeof(*op);		    
	    if(fun->native.function)
	    {
		stack = fun->native.function(stack, method);
	    }
	    else
	    {
		stack = anna_frame_push(stack, method);
	    }
	}
	else
	{
	    anna_object_t *res;
	    res = obj->member[m->offset];		    
	    anna_vmstack_push(stack, res);
		    
	    stack->code += sizeof(*op);
	}
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_STATIC_MEMBER_GET:
    {
	anna_op_member_t *op = (anna_op_member_t *)stack->code;
	anna_object_t *obj = anna_vmstack_pop(stack);

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

	    anna_vmstack_push(stack, method);
	    anna_vmstack_push(stack, obj);
	    stack->code += sizeof(*op);
	    if(fun->native.function)
	    {
		stack = fun->native.function(stack, method);
	    }
	    else
	    {
		stack = anna_frame_push(stack, method);
	    }
	}
	else
	{
	    anna_object_t *res;
	    res = obj->type->static_member[m->offset];
	    anna_vmstack_push(stack, res);
	    stack->code += sizeof(*op);
	}
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_MEMBER_GET_THIS:
    {
	anna_op_member_t *op = (anna_op_member_t *)stack->code;
	anna_object_t *obj = anna_vmstack_pop(stack);
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
	    anna_vmstack_print(stack);
		    
	    CRASH;
	}
#endif 
	if(m->is_property)
	{
//		    anna_object_t *method = obj->type->static_member[m->getter_offset];
	    wprintf(L"PROPERTIES NOT YET IMPLEMENTED!!!\n");
	    CRASH;
	}
	anna_object_t *res;
		
	if(m->is_static) {
	    res = obj->type->static_member[m->offset];
	} else {
	    res = (obj->member[m->offset]);
	}
	anna_vmstack_push(stack, res);
	anna_vmstack_push(stack, obj);

	stack->code += sizeof(*op);
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_MEMBER_SET:
    {
	anna_op_member_t *op = (anna_op_member_t *)stack->code;
	anna_object_t *obj = anna_vmstack_pop(stack);
	anna_object_t *value = anna_vmstack_peek(stack, 0);
		
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
		    
	    anna_vmstack_pop(stack);
	    anna_vmstack_push(stack, method);
	    anna_vmstack_push(stack, obj);
	    anna_vmstack_push(stack, value);
	    stack->code += sizeof(*op);
	    if(fun->native.function)
	    {
		stack = fun->native.function(
		    stack, method);
	    }
	    else
	    {
		stack = anna_frame_push(stack, method);
	    }
	}
	else
	{
	    if(m->is_static) {
		obj->type->static_member[m->offset] = value;
	    } else {
		obj->member[m->offset] = value;
	    }
		    
	    stack->code += sizeof(*op);
	}
	goto *jump_label[*stack->code];
    }

    ANNA_LAB_LIST:
    {
	anna_op_type_t *op = (anna_op_type_t *)stack->code;
	anna_vmstack_push(stack, anna_list_create2(op->value));
	stack->code += sizeof(*op);
	goto *jump_label[*stack->code];
    }

    ANNA_LAB_FOLD:
    {
	anna_object_t *val = anna_vmstack_pop(stack);
	anna_list_add(anna_vmstack_peek(stack, 0), val);
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_POP:
    {
	anna_vmstack_pop(stack);
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_NOT:
    {
	*(stack->top-1) = (*(stack->top-1)==null_object)?anna_int_one:null_object;
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[*stack->code];
    }

    ANNA_LAB_DUP:
    {
	anna_vmstack_push(stack, anna_vmstack_peek(stack, 0));
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[*stack->code];
    }

    ANNA_LAB_JMP:
    {
	anna_op_off_t *op = (anna_op_off_t *)stack->code;
	stack->code += op->offset;
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_COND_JMP:
    {
	anna_op_off_t *op = (anna_op_off_t *)stack->code;
	stack->code += anna_vmstack_pop(stack) != null_object ? op->offset:sizeof(*op);
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_NCOND_JMP:
    {
	anna_op_off_t *op = (anna_op_off_t *)stack->code;
	stack->code += anna_vmstack_pop(stack) == null_object ? op->offset:sizeof(*op);
	goto *jump_label[*stack->code];
    }
	    
    ANNA_LAB_TRAMPOLENE:
    {
	anna_object_t *base = anna_vmstack_pop(stack);
	anna_vmstack_push(stack, anna_vm_trampoline(anna_function_unwrap(base), stack));
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[*stack->code];
    }

}

static size_t anna_bc_op_size(char instruction)
{
    switch(instruction)
    {
	case ANNA_INSTR_STRING:
	case ANNA_INSTR_CONSTANT:
	{
	    return sizeof(anna_op_const_t);
	}
	    
	case ANNA_INSTR_LIST:
	case ANNA_INSTR_CAST:
	{
	    return sizeof(anna_op_type_t);
	}
	    
	case ANNA_INSTR_FOLD:
	case ANNA_INSTR_CONSTRUCT:
	case ANNA_INSTR_RETURN:
	case ANNA_INSTR_STOP:
	case ANNA_INSTR_TRAMPOLENE:
	case ANNA_INSTR_POP:
	case ANNA_INSTR_NOT:
	case ANNA_INSTR_DUP:
	{
	    return sizeof(anna_op_null_t);
	}
	
	case ANNA_INSTR_CALL:
	{
	    return sizeof(anna_op_count_t);
	}
	
	case ANNA_INSTR_VAR_SET:
	case ANNA_INSTR_VAR_GET:
	{
	    return sizeof(anna_op_var_t);
	}
	    
	case ANNA_INSTR_MEMBER_GET:
	case ANNA_INSTR_STATIC_MEMBER_GET:
	case ANNA_INSTR_MEMBER_GET_THIS:
	case ANNA_INSTR_MEMBER_SET:
	{
	    return sizeof(anna_op_member_t);
	}
	
	    
	case ANNA_INSTR_JMP:
	case ANNA_INSTR_COND_JMP:
	case ANNA_INSTR_NCOND_JMP:
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
	    case ANNA_INSTR_STRING:
	    case ANNA_INSTR_CONSTANT:
	    {
		anna_op_const_t *op = (anna_op_const_t*)code;
		wprintf(L"Push constant of type %ls\n\n", op->value->type->name);
		break;
	    }
	    
	    case ANNA_INSTR_LIST:
	    {
		wprintf(L"List creation\n\n");
		break;
	    }
	    
	    case ANNA_INSTR_CAST:
	    {
		wprintf(L"Type cast\n\n");
		break;
	    }
	    
	    case ANNA_INSTR_FOLD:
	    {
		wprintf(L"List fold\n\n");
		break;
	    }
	    
	    case ANNA_INSTR_CALL:
	    {
		anna_op_count_t *op = (anna_op_count_t *)code;
		size_t param = op->param;
		wprintf(L"Call function with %d parameter(s)\n\n", param);
		break;
	    }
	    
	    case ANNA_INSTR_CONSTRUCT:
	    {
		wprintf(L"Construct object\n\n");
		break;
	    }
	    
	    case ANNA_INSTR_RETURN:
	    {
		wprintf(L"Return\n\n");
		return;
	    }
	    
	    case ANNA_INSTR_STOP:
	    {
		wprintf(L"Stop\n\n");
		return;
	    }
	    
	    case ANNA_INSTR_VAR_GET:
	    {
		anna_op_var_t *op = (anna_op_var_t *)code;
		wprintf(L"Get var %d : %d\n\n", op->frame_count, op->offset);
		break;
	    }
	    
	    case ANNA_INSTR_VAR_SET:
	    {
		anna_op_var_t *op = (anna_op_var_t *)code;
		wprintf(L"Set var %d : %d\n\n", op->frame_count, op->offset);
		break;
	    }
	    
	    case ANNA_INSTR_MEMBER_GET:
	    case ANNA_INSTR_STATIC_MEMBER_GET:
	    {
		anna_op_member_t *op = (anna_op_member_t *)code;
		wprintf(L"Get member %d\n\n", op->mid);
		break;
	    }
	    
	    case ANNA_INSTR_MEMBER_GET_THIS:
	    {
		anna_op_member_t *op = (anna_op_member_t *)code;
		wprintf(L"Get member %d and push object as implicit this param\n\n", op->mid);
		break;
	    }
	    
	    case ANNA_INSTR_MEMBER_SET:
	    {
		anna_op_member_t *op = (anna_op_member_t *)code;
		wprintf(L"Set member %d\n\n", op->mid);
		break;
	    }

	    case ANNA_INSTR_POP:
	    {
		wprintf(L"Pop stack\n\n");
		break;
	    }
	    
	    case ANNA_INSTR_NOT:
	    {
		wprintf(L"Invert stack top element\n\n");
		break;
	    }
	    
	    case ANNA_INSTR_DUP:
	    {
		wprintf(L"Duplicate stack top element\n\n");
		break;
	    }
	    
	    case ANNA_INSTR_JMP:
	    {
		anna_op_off_t *op = (anna_op_off_t *)code;
		wprintf(L"Jump %d bytes\n\n", op->offset);
		break;
	    }
	    
	    case ANNA_INSTR_COND_JMP:
	    {
		anna_op_off_t *op = (anna_op_off_t *)code;
		wprintf(L"Conditionally jump %d bytes\n\n", op->offset);
		break;
	    }
	    
	    
	    case ANNA_INSTR_NCOND_JMP:
	    {
		anna_op_off_t *op = (anna_op_off_t *)code;
		wprintf(L"Conditionally not jump %d bytes\n\n", op->offset);
		break;
	    }
	    
	    case ANNA_INSTR_TRAMPOLENE:
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
	    case ANNA_INSTR_STRING:
	    case ANNA_INSTR_CONSTANT:
	    {
		anna_op_const_t *op = (anna_op_const_t*)code;
		anna_alloc_mark_object(op->value);
		break;
	    }
	    
	    case ANNA_INSTR_LIST:
	    case ANNA_INSTR_CAST:
	    {
		anna_op_type_t *op = (anna_op_type_t*)code;
		anna_alloc_mark_type(op->value);
		break;
	    }
/*
	    case ANNA_INSTR_CALLBACK:
	    {
		anna_op_callback_t *op = (anna_op_callback_t*)code;
		anna_alloc_mark_type(op->aux1);
		anna_alloc_mark_type(op->aux2);
		break;
	    }
*/	    
	    case ANNA_INSTR_RETURN:
	    case ANNA_INSTR_STOP:
	    {
		return;
	    }
	    
	    case ANNA_INSTR_FOLD:
	    case ANNA_INSTR_CALL:
	    case ANNA_INSTR_CONSTRUCT:
	    case ANNA_INSTR_VAR_GET:
	    case ANNA_INSTR_VAR_SET:
	    case ANNA_INSTR_STATIC_MEMBER_GET:
	    case ANNA_INSTR_MEMBER_GET:
	    case ANNA_INSTR_MEMBER_GET_THIS:
	    case ANNA_INSTR_MEMBER_SET:
	    case ANNA_INSTR_POP:
	    case ANNA_INSTR_NOT:
	    case ANNA_INSTR_DUP:
	    case ANNA_INSTR_JMP:
	    case ANNA_INSTR_COND_JMP:
	    case ANNA_INSTR_NCOND_JMP:
	    case ANNA_INSTR_TRAMPOLENE:
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

static size_t anna_bc_stack_size(char *code)
{
    size_t pos = 0;
    size_t max = 0;
    while(1)
    {
	char instruction = *code;
	switch(instruction)
	{
	    case ANNA_INSTR_STRING:
	    case ANNA_INSTR_CONSTANT:
	    case ANNA_INSTR_LIST:
	    case ANNA_INSTR_CONSTRUCT:
	    case ANNA_INSTR_VAR_GET:
	    case ANNA_INSTR_MEMBER_GET_THIS:
	    case ANNA_INSTR_DUP:
	    {
		pos++;
		break;
	    }
	    
	    case ANNA_INSTR_FOLD:
	    case ANNA_INSTR_MEMBER_SET:
	    case ANNA_INSTR_POP:
	    {
		pos--;
		break;
	    }

	    case ANNA_INSTR_CALL:
	    {
		anna_op_count_t *op = (anna_op_count_t *)code;
		size_t param = op->param;
		pos -= param;
		break;
	    }
	    
	    case ANNA_INSTR_RETURN:
	    case ANNA_INSTR_RETURN_COUNT:
	    case ANNA_INSTR_STOP:
	    {
		return max;
	    }
	    
	    case ANNA_INSTR_VAR_SET:
	    case ANNA_INSTR_MEMBER_GET:
	    case ANNA_INSTR_STATIC_MEMBER_GET:
	    case ANNA_INSTR_NOT:
	    case ANNA_INSTR_JMP:
	    case ANNA_INSTR_COND_JMP:
	    case ANNA_INSTR_NCOND_JMP:
	    case ANNA_INSTR_TRAMPOLENE:
	    case ANNA_INSTR_CAST:
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
		sizeof(anna_op_count_t) +
		sizeof(anna_op_off_t) +
		anna_vm_size(fun, (anna_node_t *)node2->block2) +
		sizeof(anna_op_count_t);
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

	    return sz1 + sz2 + sizeof(anna_op_off_t)*2 + sizeof(anna_op_count_t) + sizeof(anna_op_const_t) + sizeof(anna_op_null_t);
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
	
	case ANNA_NODE_RETURN:
	{
	    anna_node_wrapper_t *node2 = (anna_node_wrapper_t *)node;
	    
	    return anna_vm_size(fun, node2->payload) + sizeof(anna_op_count_t);
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;

	    size_t res = 
		anna_vm_size(fun, node2->function) + sizeof(anna_op_count_t);

	    anna_function_type_t *template;
	    int ra;
	    if(node->node_type==ANNA_NODE_CALL)
	    {
		template = anna_function_type_extract(
		    node2->function->return_type);
		ra = template->input_count;
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
		ra = template->input_count-1;
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
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    return anna_vm_size(fun, node2->object) + sizeof(anna_op_member_t);
	}
	
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    return anna_vm_size(fun, node2->object) + anna_vm_size(fun, node2->value) + sizeof(anna_op_member_t);
	}
		
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    size_t res = 
		anna_vm_size(fun, node2->object) + 
		sizeof(anna_op_count_t) + sizeof(anna_op_member_t);
	    
	    anna_type_t *obj_type = node2->object->return_type;
	    anna_member_t *mem = anna_member_get(obj_type, node2->mid);
	    
	    anna_function_type_t *template = anna_function_type_extract(
		mem->type);
	    
	    int i;
	    
	    int ra = template->input_count-1;
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
    anna_op_count_t cop = 
	{
	    op,
	    argc
	}
    ;
    memcpy(*ptr, &cop, sizeof(anna_op_count_t));
    *ptr += sizeof(anna_op_count_t);	    
}

static void anna_vm_native_call(char **ptr, int op, anna_native_function_t fun)
{
    anna_op_native_call_t cop = 
	{
	    op,
	    fun
	}
    ;
    memcpy(*ptr, &cop, sizeof(anna_op_native_call_t));
    *ptr += sizeof(anna_op_native_call_t);	    
}

static void anna_vm_const(char **ptr, anna_object_t *val)
{
    anna_op_const_t op = 
	{
	    ANNA_INSTR_CONSTANT,
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
	    ANNA_INSTR_STRING,
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
	    anna_vm_var(ptr, ANNA_INSTR_VAR_SET, 0, sid.offset);
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
		ptr, ANNA_INSTR_NCOND_JMP, 
		2*sizeof(anna_op_off_t) +
		anna_vm_size(fun, (anna_node_t *)node2->block1) + 
		sizeof(anna_op_count_t));
	    anna_vm_compile_i(fun, (anna_node_t *)node2->block1, ptr, 0);
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 0);
	    anna_vm_jmp(
		ptr, ANNA_INSTR_JMP,
		sizeof(anna_op_off_t) + 
		anna_vm_size(fun, (anna_node_t *)node2->block2) +
		sizeof(anna_op_count_t));
	    anna_vm_compile_i(fun, (anna_node_t *)node2->block2, ptr, 0);
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 0);

	    break;
	}

	case ANNA_NODE_RETURN:
	{
	    anna_node_wrapper_t *node2 = (anna_node_wrapper_t *)node;
	    anna_vm_compile_i(fun, node2->payload, ptr, 0);
	    assert(node2->steps>=0);
	    anna_vm_call(ptr, ANNA_INSTR_RETURN_COUNT, node2->steps);
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
		ptr, ANNA_INSTR_NCOND_JMP, 
		2*sizeof(anna_op_off_t) +
		sz2 +sizeof(anna_op_null_t) +
		sizeof(anna_op_count_t));  // -1
	    anna_vm_null(ptr, ANNA_INSTR_POP); // -1
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0); // +1
	    anna_vm_call(ptr, ANNA_INSTR_CALL, 0); // 0
	    anna_vm_jmp(
		ptr, ANNA_INSTR_JMP,
		-( sizeof(anna_op_off_t) +sizeof(anna_op_null_t) +
		   sz1 + sz2 +
		   sizeof(anna_op_count_t))); // 0
	    break;
	}

	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;

	    anna_vm_compile_i(fun, node2->arg1, ptr, 0);
	    anna_vm_null(ptr, ANNA_INSTR_DUP);
	    anna_vm_jmp(
		ptr, ANNA_INSTR_COND_JMP,
		sizeof(anna_op_off_t) + sizeof(anna_op_null_t) + anna_vm_size(fun, node2->arg2));
	    anna_vm_null(ptr, ANNA_INSTR_POP);
	    anna_vm_compile_i(fun, node2->arg2, ptr, 0);
	    break;
	}

	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *node2 = (anna_node_cond_t *)node;

	    anna_vm_compile_i(fun, node2->arg1, ptr, 0);
	    
	    anna_vm_null(ptr, ANNA_INSTR_DUP);
	    anna_vm_jmp(
		ptr,ANNA_INSTR_NCOND_JMP,
		sizeof(anna_op_off_t) + sizeof(anna_op_null_t) + 
		anna_vm_size(fun, node2->arg2));
	    anna_vm_null( ptr, ANNA_INSTR_POP);
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
		anna_vm_member(ptr, ANNA_INSTR_STATIC_MEMBER_GET, anna_mid_get(node2->name));
		break;
	    }

	    anna_sid_t sid = anna_stack_sid_create(
		fun->stack_template, node2->name);

	    anna_vm_var(
		ptr,
		ANNA_INSTR_VAR_GET,
		sid.frame,
		sid.offset);	    
	    
	    break;
	}
	
	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(fun, node2->child[0], ptr, 0);
	    anna_vm_type(ptr, ANNA_INSTR_CAST, node2->return_type);
	    break;
	}
	
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(fun, node2->function, ptr, 0);
	    
	    anna_function_type_t *template;
	    int ra;
	    if(node->node_type==ANNA_NODE_CALL)
	    {
		template = anna_function_type_extract(
		    node2->function->return_type);
		ra = template->input_count;
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
		anna_vm_null(ptr, ANNA_INSTR_CONSTRUCT);
		ra = template->input_count-1;
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
		    ANNA_INSTR_LIST,
		    anna_list_type_get(template->input_type[template->input_count-1]));
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(fun, node2->child[i], ptr, 0);		
		    anna_vm_null(
			ptr,
			ANNA_INSTR_FOLD);
		}
	    }
	    
	    //wprintf(L"Woo argc %d\n", template->input_count);
	    anna_vm_call(
		ptr,
		ANNA_INSTR_CALL,
		template->input_count);
	    
	    if(node->node_type==ANNA_NODE_CONSTRUCT)
	    {
		//anna_vm_null(ptr, ANNA_INSTR_POP);
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
		anna_vm_member(ptr, ANNA_INSTR_MEMBER_SET, anna_mid_get(node2->name));
		break;
	    }

	    anna_sid_t sid = anna_stack_sid_create(
		fun->stack_template, node2->name);
	    
	    anna_vm_var(
		ptr,
		ANNA_INSTR_VAR_SET,
		sid.frame,
		sid.offset);	    
	    
	    break;
	}
	
	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    anna_vm_compile_i(fun, node2->object, ptr, 0);
	    anna_type_t *type = node2->object->return_type;
	    anna_member_t *m = type->mid_identifier[node2->mid];
	    anna_vm_member(ptr, m->is_static?ANNA_INSTR_STATIC_MEMBER_GET:ANNA_INSTR_MEMBER_GET, node2->mid);
	    break;
	}
	
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *node2 = (anna_node_member_access_t *)node;
	    anna_vm_compile_i(fun, node2->value, ptr, 0);
	    anna_vm_compile_i(fun, node2->object, ptr, 0);
	    anna_vm_member(ptr, ANNA_INSTR_MEMBER_SET, node2->mid);
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
		ANNA_INSTR_TRAMPOLENE);
	    break;
	}
	
	
	case ANNA_NODE_MEMBER_CALL:
	{
	    anna_node_call_t *node2 = (anna_node_call_t *)node;
	    anna_vm_compile_i(fun, node2->object, ptr, 0);

	    anna_vm_member(ptr, ANNA_INSTR_MEMBER_GET_THIS, node2->mid);
	    
	    anna_type_t *obj_type = node2->object->return_type;
	    anna_member_t *mem = anna_member_get(obj_type, node2->mid);
	    
	    anna_function_type_t *template = anna_function_type_extract(
		mem->type);
	    
	    int i;
	    
	    int ra = template->input_count-1;
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
		    ANNA_INSTR_LIST,
		    anna_list_type_get(template->input_type[template->input_count-1]));
		
		
		for(; i<node2->child_count; i++)
		{
		    anna_vm_compile_i(fun, node2->child[i], ptr, 0);		
		    anna_vm_null(ptr, ANNA_INSTR_FOLD);
		}
	    }
	    
	    anna_vm_call(ptr, ANNA_INSTR_CALL, template->input_count);
	    break;
	}	
	
	default:
	{
	    wprintf(L"Unknown AST node %d\n", node->node_type);
	    CRASH;
	}
    }
    if(drop_output){
	anna_vm_null(ptr, ANNA_INSTR_POP);
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
	sz = anna_bc_op_size(ANNA_INSTR_CONSTANT) + anna_bc_op_size(ANNA_INSTR_RETURN);
	
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
    
    fun->frame_size = sizeof(anna_vmstack_t) + sizeof(anna_object_t *)*(fun->variable_count + anna_bc_stack_size(fun->code)) + 2*sizeof(void *);;
    fun->definition = fun->body = 0;
    fun->native.function = anna_frame_push;
    
//    anna_bc_print(fun->code);
}

anna_vmstack_t *anna_vm_callback_native(
    anna_vmstack_t *parent, 
    anna_native_function_t callback, int paramc, anna_object_t **param,
    anna_object_t *entry, int argc, anna_object_t **argv)
{
    size_t ss = (paramc+argc+3)*sizeof(anna_object_t *) + sizeof(anna_vmstack_t);
    size_t cs = sizeof(anna_op_count_t) + sizeof(anna_op_native_call_t) + sizeof(anna_op_null_t);
    anna_vmstack_t *stack = calloc(1,ss+cs);
    stack->flags = ANNA_VMSTACK;
    al_push(&anna_alloc, stack);
    stack->caller = parent;

    stack->parent = *(anna_vmstack_t **)anna_member_addr_get_mid(entry,ANNA_MID_FUNCTION_WRAPPER_STACK);
    
    stack->function = 0;
    stack->top = &stack->base[0];
    stack->code = ((char *)stack)+ss;

    char *code = stack->code;
    anna_vm_call(&code, ANNA_INSTR_CALL, argc);
    anna_vm_native_call(&code, ANNA_INSTR_NATIVE_CALL, callback);
    anna_vm_null(&code, ANNA_INSTR_RETURN);
    
    anna_vmstack_push(stack, null_object);
    int i;    
    for(i=0; i<paramc; i++)
    {
	anna_vmstack_push(stack, param[i]);
    }
    anna_vmstack_push(stack, entry);
    for(i=0; i<argc; i++)
    {
	anna_vmstack_push(stack, argv[i]);
    }
    return stack;
}

void anna_vm_callback_reset(
    anna_vmstack_t *stack, 
    anna_object_t *entry, int argc, anna_object_t **argv)
{
	int i;    
	anna_vmstack_push(stack, entry);
	for(i=0; i<argc; i++)
	{
	    anna_vmstack_push(stack, argv[i]);
	}
	stack->code -= (sizeof(anna_op_count_t)+sizeof(anna_op_native_call_t));	
}

/**
   This method is the best ever! It does nothing and returns a null
   object. All method calls on the null object run this. 
*/
anna_vmstack_t *anna_vm_null_function(anna_vmstack_t *stack, anna_object_t *me)
{
    char *code = stack->code;
    code -= sizeof(anna_op_count_t);
    anna_op_count_t *op = (anna_op_count_t *)code;
    anna_vmstack_drop(stack,op->param+1);
    anna_vmstack_push(stack, null_object);
    return stack;
}

