#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "anna.h"
#include "anna_vm.h"
#include "anna_vm_internal.h"
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

char *anna_vmstack_static_ptr;
char anna_vmstack_static_data[48192];

static void anna_vmstack_print(anna_vmstack_t *stack);


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

static int stack_idx(anna_vmstack_t *stack)
{
    return stack ? 1 + stack_idx(stack->caller): 0;
}


static int stack_sum(anna_vmstack_t *stack)
{
    
    int res = 0xdeadbeef;
    char *chr = (char *)&stack->base[0];
    char *end = (char *)stack->top;
    
    for(; chr < end; chr++)
    {
	res = res ^ (res << 5) ^ *chr;
    }
    return res;
}

static void stack_describe(anna_vmstack_t *stack)
{
    if(stack)
    {
	wprintf(
	    L"Frame %d:\tParent: %d\tDynamic: %ls\tSize: %d\tUsed: %d \tAddress: %d\tChecksum: %d\tFunction: %ls\n",
	    stack_idx(stack),
	    stack_idx(stack->parent), 
	    (stack->flags & ANNA_VMSTACK_STATIC)?L"no":L"yes",
	    stack->function?stack->function->frame_size:-1,
	    (char *)stack->top - (char *)&stack->base[0],
	    stack,
	    stack_sum(stack),
	    stack->function?stack->function->name:L"<null>");
	
	//anna_vmstack_print(stack);
	stack_describe(stack->caller);
    }
}

anna_vmstack_t *anna_frame_to_heap(anna_vmstack_t *stack)
{
    anna_vmstack_t *ptr = stack;
    anna_vmstack_t *first_copy = 0;
    anna_vmstack_t *prev = 0;
    
    if(!(ptr->flags & ANNA_VMSTACK_STATIC))
    {
	return ptr;
    }
/*    
    wprintf(L"BEFORE:\n");
    stack_describe(stack);
*/  
    while(ptr && (ptr->flags & ANNA_VMSTACK_STATIC))
    {
	anna_vmstack_t *copy = anna_alloc_vmstack(ptr->function->frame_size);
	if(!first_copy)
	    first_copy = copy;
	if(prev)
	{
	    prev->caller = copy;
	}
	
	anna_frame_return(ptr);
	memcpy(copy, ptr, (char *)ptr->top - (char *)ptr);
	ptr->code = (char *)copy;
	copy->top = &copy->base[ptr->top - &ptr->base[0]];
	
	prev = copy;
	ptr = ptr->caller;	
    }
    
    ptr = first_copy;
    while(ptr && ptr->flags & ANNA_VMSTACK_STATIC)
    {
	ptr->flags = ptr->flags & ~ANNA_VMSTACK_STATIC;
	if(ptr->parent && ptr->parent->flags & ANNA_VMSTACK_STATIC)
	{
	    ptr->parent = (anna_vmstack_t *)ptr->parent->code;
	}
	ptr = ptr->caller;
    }
    /*  
    wprintf(L"\nAFTER:\n");
    stack_describe(first_copy);
    */
    return first_copy;    
}


static void anna_vmstack_print(anna_vmstack_t *stack)
{
    anna_entry_t **p = &stack->base[0];
    wprintf(L"\tFrame content:\n");
    while(p!=stack->top)
    {
	if(!*p){
	    wprintf(L"\tError: Null slot\n");	    
	}
	else
	{
	    wprintf(L"\t%ls\n", anna_as_obj(*p)->type->name);
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


void anna_vm_init()
{
    anna_vmstack_static_ptr = &anna_vmstack_static_data[0];
}

#ifdef ANNA_FULL_GC_ON_SHUTDOWN
void anna_vm_destroy(void)
{
    free(stack_mem);
}
#endif

anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_object_t **argv)
{
    static void * jump_label[] = 
	{
	    &&ANNA_LAB_RETURN, 
	    &&ANNA_LAB_CONSTANT,
	    &&ANNA_LAB_CALL,
	    &&ANNA_LAB_STOP,
	    &&ANNA_LAB_VAR_GET,
	    &&ANNA_LAB_VAR_SET,
	    &&ANNA_LAB_MEMBER_GET,
	    &&ANNA_LAB_STATIC_MEMBER_GET,
	    &&ANNA_LAB_PROPERTY_GET,
	    &&ANNA_LAB_STATIC_PROPERTY_GET,
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
	    &&ANNA_LAB_RETURN_COUNT,
	    &&ANNA_LAB_RETURN_COUNT_BREAK, //26
	    &&ANNA_LAB_CHECK_BREAK, //27

	    0, 0, 0, 0, 0, //32
	    0, 0, 0, 0, 0, 0, 0, 0, //40
	    0, 0, 0, 0, 0, 0, 0, 0, //48
	    0, 0, 0, 0, 0, 0, 0, 0, //56
	    0, 0, 0, 0, 0, 0, 0, //63
	    &&ANNA_LAB_ADD_INT,
	    &&ANNA_LAB_SUB_INT,
	    &&ANNA_LAB_MUL_INT,
	    &&ANNA_LAB_DIV_INT,
	    &&ANNA_LAB_INCREASE_ASSIGN_INT,
	    &&ANNA_LAB_DECREASE_ASSIGN_INT,
	    &&ANNA_LAB_BITAND_INT,
	    &&ANNA_LAB_BITOR_INT,
	    &&ANNA_LAB_BITXOR_INT,

	    &&ANNA_LAB_EQ_INT,
	    &&ANNA_LAB_NEQ_INT,
	    &&ANNA_LAB_LT_INT,
	    &&ANNA_LAB_LTE_INT,
	    &&ANNA_LAB_GTE_INT,
	    &&ANNA_LAB_GT_INT,

	    &&ANNA_LAB_ADD_FLOAT,
	    &&ANNA_LAB_SUB_FLOAT,
	    &&ANNA_LAB_MUL_FLOAT,
	    &&ANNA_LAB_DIV_FLOAT,
	    &&ANNA_LAB_EXP_FLOAT,
	    &&ANNA_LAB_INCREASE_ASSIGN_FLOAT,
	    &&ANNA_LAB_DECREASE_ASSIGN_FLOAT,
	}
    ;
    
    static int vm_count = 0;
    int is_root = vm_count==0;
    
    anna_vmstack_t *stack;    
    stack = calloc(1, (argc+1)*sizeof(anna_object_t *) + sizeof(anna_vmstack_t));
    stack->flags = ANNA_VMSTACK;
    al_push(&anna_alloc, stack);
    
    stack->caller = 0;
    
    stack->parent = *(anna_vmstack_t **)anna_entry_get_addr(entry,ANNA_MID_FUNCTION_WRAPPER_STACK);
    
    stack->function = 0;
    stack->top = &stack->base[0];
    stack->code = malloc(1);
    *(stack->code) = ANNA_INSTR_STOP;
    
    int i;
    anna_vmstack_push_object(stack, entry);
    for(i=0; i<argc; i++)
    {
	anna_vmstack_push_object(stack, argv[i]);
    }
    anna_function_t *root_fun = anna_function_unwrap(entry);
    stack = root_fun->native(stack, entry);

    goto *jump_label[(int)*stack->code];

  ANNA_LAB_CONSTANT:
    {
	anna_op_const_t *op = (anna_op_const_t *)stack->code;
	anna_vmstack_push_entry(stack, op->value);
	
	stack->code += sizeof(*op);
	goto *jump_label[(int)*stack->code];
    }
    
  ANNA_LAB_STRING:
    {
	anna_op_const_t *op = (anna_op_const_t *)stack->code;
	anna_vmstack_push_object(stack, anna_string_copy(anna_as_obj_fast(op->value)));
	stack->code += sizeof(*op);
	goto *jump_label[(int)*stack->code];
    }
    
  ANNA_LAB_CAST:	    
    {
	
	anna_op_type_t *op = (anna_op_type_t *)stack->code;
	if(!anna_abides(anna_vmstack_peek_object(stack,0)->type, op->value))
	{
	    anna_vmstack_pop_object(stack);
	    anna_vmstack_push_object(stack, null_object);
	}
	
	stack->code += sizeof(*op);
	goto *jump_label[(int)*stack->code];
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
	anna_object_t *wrapped = anna_vmstack_peek_object_fast(stack, param);
	if(unlikely(wrapped == null_object))
	{
	    stack->code += sizeof(*op);
	    anna_vmstack_drop(stack, param);
	}
	else
	{
//	    wprintf(L"Call at offset %d\n", stack->code - stack->function->code);/
	    anna_function_t *fun = anna_function_unwrap(wrapped);
//	    wprintf(L"Call function %ls with %d params\n", fun->name, param);
	    
#ifdef ANNA_CHECK_VM
	    if(!fun)
	    {
		wprintf(L"Error: Tried to call something that is not a function with %d params. Stack contents:\n", param);
		anna_vmstack_print(stack);
		CRASH;
	    }
#endif
	    
	    stack->code += sizeof(*op);
	    stack = fun->native(stack, wrapped);
	}
	
	goto *jump_label[(int)*stack->code];
    }
    
  ANNA_LAB_CONSTRUCT:
    {
	anna_op_null_t *op = (anna_op_null_t *)stack->code;
	anna_object_t *wrapped = anna_vmstack_pop_object_fast(stack);
	
	anna_type_t *tp = anna_type_unwrap(wrapped);
	
	anna_object_t *result = anna_object_create(tp);
	
	anna_entry_t **constructor_ptr = anna_entry_get_addr_static(
	    tp,
	    ANNA_MID_INIT_PAYLOAD);
	anna_vmstack_push_object(stack, anna_as_obj_fast(*constructor_ptr));
	anna_vmstack_push_object(stack, result);
	stack->code += sizeof(*op);
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_RETURN:
    {
	anna_entry_t *val = anna_vmstack_peek_entry(stack, 0);
	anna_frame_return(stack);
	
	stack = stack->caller;
	anna_vmstack_push_entry(stack, val);
//		wprintf(L"Pop frame\n");
	goto *jump_label[(int)*stack->code];
    }
    
    ANNA_LAB_RETURN_COUNT:
    {
	anna_op_count_t *cb = (anna_op_count_t *)stack->code;
	anna_entry_t *val = anna_vmstack_peek_entry(stack, 0);
	int i;
		
	for(i=0; i<cb->param; i++)
	{
	    anna_frame_return(stack);
	    stack = stack->parent;
	}
	anna_frame_return(stack);
	stack = stack->caller;
	anna_vmstack_push_entry(stack, val);
	goto *jump_label[(int)*stack->code];
    }

    ANNA_LAB_RETURN_COUNT_BREAK:
    {
	anna_op_count_t *cb = (anna_op_count_t *)stack->code;
	anna_entry_t *val = anna_vmstack_peek_entry(stack, 0);
	int i;
		
	for(i=0; i<cb->param; i++)
	{
	    anna_frame_return(stack);
	    stack = stack->parent;
	}
	anna_frame_return(stack);
	stack = stack->caller;
	anna_vmstack_push_entry(stack, val);
	stack->flags |= ANNA_VMSTACK_BREAK;
	goto *jump_label[(int)*stack->code];
    }

    ANNA_LAB_CHECK_BREAK:
    {
	anna_vmstack_push_entry( 
	    stack, 
	    stack->flags & ANNA_VMSTACK_BREAK ? anna_from_int(1) : anna_from_obj(null_object));
	// Clear the break flag on check
	stack->flags  = stack->flags & ~ANNA_VMSTACK_BREAK;
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
	
    }
    

  ANNA_LAB_NATIVE_CALL:
    {
	anna_op_native_call_t *cb = (anna_op_native_call_t *)stack->code;
	stack->code += sizeof(*cb);

	stack = cb->function(stack, 0);
	goto *jump_label[(int)*stack->code];
    }

  ANNA_LAB_STOP:
    {
//		wprintf(L"Pop last frame\n");
	anna_object_t *val = anna_vmstack_peek_object(stack, 0);
	free(stack->code);
	anna_frame_return(stack);
	stack = stack->caller;
	return val;
    }
	    
  ANNA_LAB_VAR_GET:
    {
	anna_op_var_t *op = (anna_op_var_t *)stack->code;
	int i;
	anna_vmstack_t *s = stack;
	for(i=0; i<op->frame_count; i++)
	{

	    s = s->parent;
#ifdef ANNA_CHECK_VM
	    if(!s)
	    {
		wprintf(
		    L"Var get op to invalid stack frame: %d %d %ls\n",
		    op->frame_count, op->offset,
		    stack->function->name);
		anna_vmstack_print_parent(stack);
		stack_describe(stack);
		
		CRASH;
	    }
#endif
	}
#ifdef ANNA_CHECK_VM
	if(!s->base[op->offset])
	{
	    wprintf(
		L"Var get op on unassigned var: %d %d\n",
		op->frame_count, op->offset);
		    
	    CRASH;
	}
#endif
	anna_vmstack_push_entry(stack, s->base[op->offset]);
	stack->code += sizeof(*op);
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_VAR_SET:
    {
	anna_op_var_t *op = (anna_op_var_t *)stack->code;
	int i;
	anna_vmstack_t *s = stack;
	for(i=0; i<op->frame_count; i++)
	    s = s->parent;
	s->base[op->offset] = anna_vmstack_peek_entry(stack, 0);
	stack->code += sizeof(*op);
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_MEMBER_GET:
    {
	anna_op_member_t *op = (anna_op_member_t *)stack->code;
	anna_object_t *obj = anna_vmstack_pop_object(stack);
	anna_member_t *m = obj->type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
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
	if(unlikely(obj == null_object))
	{
	    anna_vmstack_push_object(stack, null_object);
	}
	else
	{
	    anna_entry_t *res = obj->member[m->offset];		    
	    anna_vmstack_push_entry(stack, res);
	}
	
	stack->code += sizeof(*op);
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_STATIC_MEMBER_GET:
    {
	anna_op_member_t *op = (anna_op_member_t *)stack->code;
	anna_object_t *obj = anna_vmstack_pop_object(stack);

	anna_member_t *m = obj->type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
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
	anna_vmstack_push_entry(stack, obj->type->static_member[m->offset]);
	stack->code += sizeof(*op);

	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_PROPERTY_GET:
    {
	anna_op_member_t *op = (anna_op_member_t *)stack->code;
	anna_object_t *obj = anna_vmstack_pop_object(stack);
	anna_member_t *m = obj->type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
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

	if(unlikely(obj == null_object))
	{
	    anna_vmstack_push_object(stack, null_object);	    
	    stack->code += sizeof(*op);		    
	}
	else
	{
	    anna_object_t *method = anna_as_obj_fast(obj->type->static_member[m->getter_offset]);
	    anna_function_t *fun = anna_function_unwrap(method);
	    
	    anna_vmstack_push_object(stack, method);
	    anna_vmstack_push_object(stack, obj);
	    stack->code += sizeof(*op);		    
	    stack = fun->native(stack, method);
	}
	
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_STATIC_PROPERTY_GET:
    {
	anna_op_member_t *op = (anna_op_member_t *)stack->code;
	anna_object_t *obj = anna_vmstack_pop_object(stack);

	anna_member_t *m = obj->type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
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
	
	anna_object_t *method = anna_as_obj_fast(obj->type->static_member[m->getter_offset]);
	anna_function_t *fun = anna_function_unwrap(method);
	
	anna_vmstack_push_object(stack, method);
	anna_vmstack_push_object(stack, obj);
	stack->code += sizeof(*op);
	stack = fun->native(stack, method);

	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_MEMBER_GET_THIS:
    {
	anna_op_member_t *op = (anna_op_member_t *)stack->code;
//	wprintf(L"Get method member %d, %ls\n", op->mid, anna_mid_get_reverse(op->mid));
	anna_object_t *obj = anna_vmstack_pop_object(stack);
#ifdef ANNA_CHECK_VM
	if(!obj){
	    debug(
		D_CRITICAL,L"Popped null ptr for member get op %ls\n",
		anna_mid_get_reverse(op->mid));
	    CRASH;
	}
#endif
	anna_member_t *m = obj->type->mid_identifier[op->mid];
#ifdef ANNA_CHECK_VM
	if(!m){
	    debug(
		D_CRITICAL,L"Object %ls does not have a member named %ls\n",
		obj->type->name, anna_mid_get_reverse(op->mid));
	    anna_vmstack_print(stack);
	    
	    CRASH;
	}
#endif 
	anna_entry_t *res = obj->type->static_member[m->offset];
	
	anna_vmstack_push_entry(stack, res);
	anna_vmstack_push_object(stack, obj);
		
	stack->code += sizeof(*op);
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_MEMBER_SET:
    {
	anna_op_member_t *op = (anna_op_member_t *)stack->code;
	anna_object_t *obj = anna_vmstack_pop_object(stack);
	anna_entry_t *value = anna_vmstack_peek_entry(stack, 0);
	
	anna_member_t *m = obj->type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
	if(!m)
	{
	    debug(
		D_CRITICAL,
		L"Object of type %ls does not have a member of named %ls (mid %d)\n",
		obj->type->name,
		anna_mid_get_reverse(op->mid), op->mid);    
	    CRASH;
	}
#endif

	if(m->is_property)
	{
	    anna_object_t *method = anna_as_obj_fast(obj->type->static_member[m->setter_offset]);
	    anna_function_t *fun = anna_function_unwrap(method);
	    
	    anna_vmstack_pop_object(stack);
	    anna_vmstack_push_object(stack, method);
	    anna_vmstack_push_object(stack, obj);
	    anna_vmstack_push_entry(stack, value);
	    stack->code += sizeof(*op);
	    stack = fun->native(
		stack, method);
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
	goto *jump_label[(int)*stack->code];
    }

  ANNA_LAB_LIST:
    {
	anna_op_type_t *op = (anna_op_type_t *)stack->code;
	anna_vmstack_push_object(stack, anna_list_create2(op->value));
	stack->code += sizeof(*op);
	goto *jump_label[(int)*stack->code];
    }

  ANNA_LAB_FOLD:
    {
	anna_entry_t *val = anna_vmstack_pop_entry(stack);
#ifdef ANNA_CHECK_VM
	anna_object_t *lst = anna_vmstack_peek_object(stack, 0);
	if(lst->type->mid_identifier[ANNA_MID_LIST_PAYLOAD] == 0)
	{
	    debug(
		D_CRITICAL,
		L"Tried to fold value into something that is not a list.\n");
	    debug(
		D_CRITICAL,
		L"Non-list:\n");
	    anna_object_print(lst);
	    
	    if(anna_is_obj(val))
	    {
		debug(
		    D_CRITICAL,
 		    L"Value:\n");
		anna_object_print(anna_as_obj(val));
	    }
	    
	    CRASH;
	}
#endif	
	anna_list_add(anna_vmstack_peek_object(stack, 0), val);
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_POP:
    {
	anna_vmstack_pop_entry(stack);
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_NOT:
    {
	*(stack->top-1) = anna_entry_null(*(stack->top-1))?anna_from_int(1):anna_from_obj(null_object);
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }

  ANNA_LAB_DUP:
    {
	anna_vmstack_push_entry(stack, anna_vmstack_peek_entry(stack, 0));
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }

  ANNA_LAB_JMP:
    {
	anna_op_off_t *op = (anna_op_off_t *)stack->code;
	stack->code += op->offset;
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_COND_JMP:
    {
	anna_op_off_t *op = (anna_op_off_t *)stack->code;
	stack->code += !anna_entry_null(anna_vmstack_pop_entry(stack)) ? op->offset:sizeof(*op);
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_NCOND_JMP:
    {
	anna_op_off_t *op = (anna_op_off_t *)stack->code;
	stack->code += anna_entry_null(anna_vmstack_pop_entry(stack)) ? op->offset:sizeof(*op);
	goto *jump_label[(int)*stack->code];
    }
	    
  ANNA_LAB_TRAMPOLENE:
    {
	stack = anna_frame_to_heap(stack);
	anna_object_t *base = anna_vmstack_pop_object_fast(stack);
	anna_vmstack_push_object(stack, anna_vm_trampoline(anna_function_unwrap(base), stack));
	
	stack->code += sizeof(anna_op_null_t);
	goto *jump_label[(int)*stack->code];
    }

#include "anna_vm_short_circut.c"

}

size_t anna_bc_op_size(char instruction)
{
    if(anna_instr_is_short_circut(instruction))
    {
	return sizeof(anna_op_null_t);	    
    }
    
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
	case ANNA_INSTR_CHECK_BREAK:
	{
	    return sizeof(anna_op_null_t);	    
	}
	
	case ANNA_INSTR_CALL:
	case ANNA_INSTR_RETURN_COUNT:
	case ANNA_INSTR_RETURN_COUNT_BREAK:
	{
	    return sizeof(anna_op_count_t);
	}
	
	case ANNA_INSTR_VAR_SET:
	case ANNA_INSTR_VAR_GET:
	{
	    return sizeof(anna_op_var_t);
	}
	    
	case ANNA_INSTR_PROPERTY_GET:
	case ANNA_INSTR_STATIC_PROPERTY_GET:
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

	if(anna_instr_is_short_circut(instruction))
	{
	    wprintf(L"Short circut arithmetic operator %d\n\n", instruction);
	}
	else
	{
	    
	    switch(instruction)
	    {
		case ANNA_INSTR_STRING:
		case ANNA_INSTR_CONSTANT:
		{
		    anna_op_const_t *op = (anna_op_const_t*)code;
		    wprintf(L"Push constant of type %ls\n\n", 
			    anna_as_obj(op->value)->type->name);
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
	    
		case ANNA_INSTR_RETURN_COUNT:
		{
		    anna_op_count_t *op = (anna_op_count_t *)code;
		    wprintf(L"Pop value, pop %d call frames and push value\n\n", op->param);
		    break;
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
		    wprintf(L"Get member %ls\n\n", anna_mid_get_reverse(op->mid));
		    break;
		}
	    
		case ANNA_INSTR_PROPERTY_GET:
		case ANNA_INSTR_STATIC_PROPERTY_GET:
		{
		    anna_op_member_t *op = (anna_op_member_t *)code;
		    wprintf(L"Get property %ls\n\n", anna_mid_get_reverse(op->mid));
		    break;
		}
	    
		case ANNA_INSTR_MEMBER_GET_THIS:
		{
		    anna_op_member_t *op = (anna_op_member_t *)code;
		    wprintf(L"Get member %ls and push object as implicit this param\n\n", anna_mid_get_reverse(op->mid));
		    break;
		}
	    
		case ANNA_INSTR_MEMBER_SET:
		{
		    anna_op_member_t *op = (anna_op_member_t *)code;
		    wprintf(L"Set member %ls\n\n", anna_mid_get_reverse(op->mid));
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

	if(!anna_instr_is_short_circut(instruction))
	{
	    switch(instruction)
	    {
		case ANNA_INSTR_STRING:
		case ANNA_INSTR_CONSTANT:
		{
		    anna_op_const_t *op = (anna_op_const_t*)code;
		    anna_alloc_mark_entry(op->value);
		    break;
		}
	    
		case ANNA_INSTR_LIST:
		case ANNA_INSTR_CAST:
		{
		    anna_op_type_t *op = (anna_op_type_t*)code;
		    anna_alloc_mark_type(op->value);
		    break;
		}

		case ANNA_INSTR_RETURN:
		case ANNA_INSTR_RETURN_COUNT:
		case ANNA_INSTR_RETURN_COUNT_BREAK:
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
		case ANNA_INSTR_PROPERTY_GET:
		case ANNA_INSTR_STATIC_PROPERTY_GET:
		case ANNA_INSTR_MEMBER_GET_THIS:
		case ANNA_INSTR_MEMBER_SET:
		case ANNA_INSTR_POP:
		case ANNA_INSTR_NOT:
		case ANNA_INSTR_DUP:
		case ANNA_INSTR_JMP:
		case ANNA_INSTR_COND_JMP:
		case ANNA_INSTR_NCOND_JMP:
		case ANNA_INSTR_TRAMPOLENE:
		case ANNA_INSTR_CHECK_BREAK:
		{
		    break;
		}
	    
		default:
		{
		    wprintf(L"Unknown opcode %d during GC\n", instruction);
		    CRASH;
		}
	    }
	}
	
	code += anna_bc_op_size(*code);
    }
}


/**
   This method is the best ever! It does nothing and returns a null
   object. All method calls on the null object run this. 
*/
anna_vmstack_t *anna_vm_null_function(anna_vmstack_t *stack, anna_object_t *me)
{
    char *code = stack->code;
    /* We rewind the code pointr one function call to get to the
     * instruction that called us. TWe then check how many parameters
     * we were called with, and drop that number of parameters and
     * finally push a null value to the stack */
    code -= sizeof(anna_op_count_t);
    anna_op_count_t *op = (anna_op_count_t *)code;
    anna_vmstack_drop(stack,op->param+1);
    anna_vmstack_push_object(stack, null_object);
    return stack;
}


