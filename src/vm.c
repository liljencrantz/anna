#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "anna/common.h"
#include "anna/base.h"
#include "anna/vm.h"
#include "anna/vm_internal.h"
#include "anna/function.h"
#include "anna/node.h"
#include "anna/lib/lang/list.h"
#include "anna/lib/lang/int.h"
#include "anna/lib/lang/float.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/lang/char.h"
#include "anna/stack.h"
#include "anna/function_type.h"
#include "anna/member.h"
#include "anna/type.h"
#include "anna/alloc.h"
#include "anna/mid.h"

#define OP_LEAVE(context) goto *jump_label[(int)*(context)->frame->code] 

#define OP_ENTER(context) 

//wprintf(L"Weee, instruction %d at offset %d\n", *context->frame->code, context->frame->code - context->frame->function->code)

char *anna_context_static_ptr;
char anna_context_static_data[ANNA_CONTEXT_SZ];

__attr_unused __cold static void anna_context_print_parent(anna_context_t *context);

static inline void anna_frame_return(anna_activation_frame_t *frame)
{
    if(frame->flags & ANNA_ACTIVATION_FRAME_STATIC)
    {
	anna_context_static_ptr -= frame->function->frame_size;
//	assert(stack == anna_context_static_ptr);
    }
}

static inline void anna_context_frame_return(anna_context_t *stack)
{
    stack->top = stack->frame->return_stack_top;
    anna_frame_return(stack->frame);
    stack->frame = stack->frame->dynamic_frame;
}

static inline void anna_context_frame_return_static(anna_context_t *stack)
{
    anna_frame_return(stack->frame);
    stack->frame = stack->frame->static_frame;
}

static inline anna_object_t *anna_vm_trampoline(
    anna_function_t *fun,
    anna_context_t *stack)
{
    anna_object_t *orig = fun->wrapper;
    anna_object_t *res = anna_object_create(orig->type);
    
    size_t payload_offset = orig->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_PAYLOAD]->offset;
    
    size_t stack_offset = orig->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_STACK]->offset;
    
    memcpy(&res->member[payload_offset],
	   &orig->member[payload_offset],
	   sizeof(anna_function_t *));    
    memcpy(&res->member[stack_offset],
	   &stack->frame,
	   sizeof(anna_context_t *));

    return res;
}

__cold static int frame_idx(anna_activation_frame_t *frame)
{
    return frame ? 1 + frame_idx(frame->dynamic_frame): 0;
}

__cold static int frame_checksum(anna_activation_frame_t *frame)
{
    
    int res = 0xdeadbeef;
    char *chr = (char *)&frame->slot[0];
    char *end = (char *)&frame->slot[frame->function->variable_count];
    
    for(; chr < end; chr++)
    {
	res = res ^ (res << 5) ^ *chr;
    }
    return res;
}

__attr_unused __cold static void anna_stack_describe(anna_context_t *stack)
{
    anna_entry_t **p = &stack->stack[0];
    while(p<stack->top)
    {
/*	if(anna_is_int(*p))
	{
	    wprintf(L"Doo dee doo %d\n", anna_as_int(*p));
	}
	else*/
	{
	    wprintf(L"lalala %ls\n", anna_is_obj(*p)?anna_as_obj(*p)->type->name: L"?");
	}
	
	p++;
    }
}

__attr_unused __cold static void frame_describe(anna_activation_frame_t *frame)
{
    if(frame)
    {
	wprintf(
	    L"Frame %d:\tParent: %d\tDynamic: %ls\tSize: %d\tUsed: %d \tAddress: %d\tChecksum: %d\tFunction: %ls\n",
	    frame_idx(frame),
	    frame_idx(frame->static_frame), 
	    (frame->flags & ANNA_ACTIVATION_FRAME_STATIC)?L"no":L"yes",
	    frame->function->variable_count,
	    frame,
	    frame_checksum(frame),
	    frame->function->name);
	
	//anna_context_print(stack);
	frame_describe(frame->dynamic_frame);
    }
}

anna_activation_frame_t *anna_frame_to_heap(anna_activation_frame_t *frame)
{
    /*
      Move an activation record from the stack to the heap. A heap
      allocated frame may never point to a statically allocated frame,
      so this means we have to go through the list of activation
      frames and move all of them to the heap. This code is quite
      tricky since it has to update all the static_frame and
      dynamic_frame pointers as well. Don't touch it unless you know
      what you are doing!
     */

    anna_activation_frame_t *ptr = frame;
    anna_activation_frame_t *first_copy = 0;
    anna_activation_frame_t *prev = 0;
    
    if(!(ptr->flags & ANNA_ACTIVATION_FRAME_STATIC))
    {
	return ptr;
    }
/*
    wprintf(L"BEFORE:\n");
    frame_describe(frame);
*/
    while(ptr && (ptr->flags & ANNA_ACTIVATION_FRAME_STATIC))
    {
	anna_activation_frame_t *copy = anna_alloc_activation_frame(ptr->function->frame_size);
	if(!first_copy)
	    first_copy = copy;
	if(prev)
	{
	    prev->dynamic_frame = copy;
	}
	
	anna_frame_return(ptr);
	memcpy(copy, ptr, ptr->function->frame_size);
	/* 
	   Save a pointer to the new heap allocated frame inside the
	   old stack allocated frame. We'll use this value when going
	   through the frame list a second time and fixing up all the
	   static_frame pointers.
	 */
	ptr->code = (char *)copy;
	prev = copy;
	ptr = ptr->dynamic_frame;
    }
    
    ptr = first_copy;
    /* 
       Perform a second iteration through the loop in order to repoint
       all the static_frame pointers. This can't be done in the first run-through, since the parent pointers haven't been created yet.
     */
    while(ptr && ptr->flags & ANNA_ACTIVATION_FRAME_STATIC)
    {
	ptr->flags = ptr->flags & ~ANNA_ACTIVATION_FRAME_STATIC;
	if(ptr->static_frame && ptr->static_frame->flags & ANNA_ACTIVATION_FRAME_STATIC)
	{
	    ptr->static_frame = (anna_activation_frame_t *)ptr->static_frame->code;
	}
	ptr = ptr->dynamic_frame;
    }

/*
    wprintf(L"\nAFTER:\n");
    frame_describe(first_copy);
*/
    return first_copy;
}

__attr_unused __cold static void anna_frame_print(anna_activation_frame_t *frame)
{
//    anna_entry_t **p = &stack->slot[0];
    wprintf(L"\tFrame content (bottom to top):\n");
/*    while(p!=stack->top)
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
*/
}

__attr_unused __cold static void anna_context_print_parent(anna_context_t *stack)
{
/*    if(!stack)
	return;
    anna_context_print_parent(stack->parent);
    wprintf(
	L"Function %ls, offset %d\n", 
	stack->function?stack->function->name:L"<null>", 
	stack->function? (stack->code - stack->function->code): -1);
*/
}

__cold void anna_vm_init()
{
    anna_context_static_ptr = &anna_context_static_data[0];

}

#ifdef ANNA_FULL_GC_ON_SHUTDOWN
void anna_vm_destroy(void)
{
    free(stack_mem);
}
#endif

anna_object_t *anna_vm_run(anna_object_t *entry, int argc, anna_entry_t **argv)
{
    static void * jump_label[] = 
	{
	    &&ANNA_LAB_STOP,
	    &&ANNA_LAB_CONSTANT,
	    &&ANNA_LAB_CALL,
	    &&ANNA_LAB_RETURN, 
	    &&ANNA_LAB_VAR_GET,
	    &&ANNA_LAB_VAR_SET,
	    &&ANNA_LAB_MEMBER_GET,
	    &&ANNA_LAB_STATIC_MEMBER_GET,
	    &&ANNA_LAB_PROPERTY_GET,
	    &&ANNA_LAB_STATIC_PROPERTY_GET,
	    &&ANNA_LAB_MEMBER_SET,
	    0,
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
	    &&ANNA_LAB_STATIC_MEMBER_SET, // 28
	    &&ANNA_LAB_TYPE_OF, // 29

	    0, 0, 0, //32
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
	    &&ANNA_LAB_NEXT_ASSIGN_INT,
	    &&ANNA_LAB_PREV_ASSIGN_INT,
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

	    &&ANNA_LAB_EQ_FLOAT,
	    &&ANNA_LAB_NEQ_FLOAT,
	    &&ANNA_LAB_LT_FLOAT,
	    &&ANNA_LAB_LTE_FLOAT,
	    &&ANNA_LAB_GTE_FLOAT,
	    &&ANNA_LAB_GT_FLOAT,

	}
    ;
    
    static int vm_count = 0;
    int is_root = vm_count==0;
    
    anna_context_t *stack;  
    size_t ss = 4096 * sizeof(anna_entry_t *);
    size_t afs = (argc+1)*sizeof(anna_entry_t *) + sizeof(anna_activation_frame_t);
    stack = anna_slab_alloc(ss);
    stack->size = ss;
    
    stack->frame = anna_alloc_activation_frame(afs);
    stack->frame->dynamic_frame = 0;
    
    stack->frame->static_frame = *(anna_activation_frame_t **)anna_entry_get_addr(entry,ANNA_MID_FUNCTION_WRAPPER_STACK);
    
    anna_function_t *fun = anna_alloc_function();
    fun->code = malloc(1);
    *(fun->code) = ANNA_INSTR_STOP;
    fun->frame_size = afs;
    fun->input_count = argc;
    fun->variable_count = 0;

    stack->frame->function = fun;
    stack->top = &stack->stack[0];
    stack->frame->code = stack->frame->function->code;
    stack->frame->return_stack_top = stack->top;
//    *(stack->code) = ANNA_INSTR_STOP;
    
    int i;
    anna_context_push_object(stack, entry);
    for(i=0; i<argc; i++)
    {
	anna_context_push_entry(stack, argv[i]);
    }
    anna_function_t *root_fun = anna_function_unwrap(entry);
    stack->function_object = entry;
    root_fun->native(stack);

//    wprintf(L"Lalala, run function %ls\n", root_fun->name);
//    if(root_fun->code)
//	anna_bc_print(root_fun->code);
    
    OP_LEAVE(stack);	

  ANNA_LAB_CONSTANT:
    {
	OP_ENTER(stack);	

	anna_op_const_t *op = (anna_op_const_t *)stack->frame->code;
	anna_context_push_entry(stack, op->value);
	
	stack->frame->code += sizeof(*op);
	OP_LEAVE(stack);	
    }
    
  ANNA_LAB_CAST:	    
    {
	OP_ENTER(stack);	
	anna_op_type_t *op = (anna_op_type_t *)stack->frame->code;
	if(!anna_abides(anna_context_peek_object(stack,0)->type, op->value))
	{
	    anna_context_pop_object(stack);
	    anna_context_push_object(stack, null_object);
	}
	
	stack->frame->code += sizeof(*op);
	OP_LEAVE(stack);	
    }
    
  ANNA_LAB_CALL:
    {
	OP_ENTER(stack);	

	if(is_root)
	{
	    anna_alloc_check_gc(stack);
	}
	
	anna_op_count_t *op = (anna_op_count_t *)stack->frame->code;
	size_t param = op->param;
	anna_object_t *wrapped = anna_context_peek_object_fast(stack, param);

#ifdef ANNA_CHECK_VM
	if(!wrapped)
	{
	    wprintf(
		L"Error: Tried to call null pointer at offset %d of function %ls\n", 
		stack->frame->code - stack->frame->function->code, stack->frame->function->name);
	    
	    CRASH;
	}
	
#endif

	if(unlikely(wrapped == null_object))
	{
	    stack->frame->code += sizeof(*op);
	    anna_context_drop(stack, param);
	}
	else
	{
	    anna_function_t *fun = anna_function_unwrap_fast(wrapped);
	    
//	    wprintf(L"Call function %ls with %d params\n", fun->name, param);
	    
#ifdef ANNA_CHECK_VM
	    if(!fun)
	    {
		debug(D_CRITICAL, L"In function %ls\n", stack->frame->function->name );
		anna_bc_print(stack->frame->function->code);
		debug(D_CRITICAL, L"Offset %d\n", stack->frame->code - stack->frame->function->code);

		debug(D_CRITICAL, L"Error: Tried to call something that is not a function with %d params. Stack contents:\n", param);
		anna_frame_print(stack->frame);
		CRASH;
	    }
#endif
	    
	    stack->frame->code += sizeof(*op);
	    stack->function_object = wrapped;
	    fun->native(stack);
	}

	OP_LEAVE(stack);	
    }
    
  ANNA_LAB_CONSTRUCT:
    {
	OP_ENTER(stack);	
	anna_op_null_t *op = (anna_op_null_t *)stack->frame->code;
	anna_object_t *wrapped = anna_context_pop_object_fast(stack);
	
	anna_type_t *tp = anna_type_unwrap(wrapped);
	
	anna_object_t *result = anna_object_create(tp);
	
	anna_entry_t **constructor_ptr = anna_entry_get_addr_static(
	    tp,
	    ANNA_MID_INIT_PAYLOAD);
	anna_context_push_object(stack, anna_as_obj_fast(*constructor_ptr));
	anna_context_push_object(stack, result);
	stack->frame->code += sizeof(*op);
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_RETURN:
    {
	OP_ENTER(stack);	
	anna_entry_t *val = anna_context_peek_entry(stack, 0);
	anna_context_frame_return(stack);
	anna_context_push_entry(stack, val);
//		wprintf(L"Pop frame\n");
	OP_LEAVE(stack);	
    }
    
    ANNA_LAB_RETURN_COUNT:
    {
	OP_ENTER(stack);	
	anna_op_count_t *cb = (anna_op_count_t *)stack->frame->code;
	anna_entry_t *val = anna_context_peek_entry(stack, 0);
	int i;
		
	for(i=0; i<cb->param; i++)
	{
	    anna_context_frame_return_static(stack);
	}
	anna_context_frame_return(stack);
	anna_context_push_entry(stack, val);
	OP_LEAVE(stack);	
    }

    ANNA_LAB_RETURN_COUNT_BREAK:
    {
	OP_ENTER(stack);	
	anna_op_count_t *cb = (anna_op_count_t *)stack->frame->code;
	anna_entry_t *val = anna_context_peek_entry(stack, 0);
	int i;
		
	for(i=0; i<cb->param; i++)
	{
	    anna_context_frame_return_static(stack);
	}
	anna_context_frame_return(stack);
	anna_context_push_entry(stack, val);
	stack->frame->flags |= ANNA_ACTIVATION_FRAME_BREAK;
	OP_LEAVE(stack);	
    }

    ANNA_LAB_CHECK_BREAK:
    {
	OP_ENTER(stack);	
	anna_context_push_entry( 
	    stack, 
	    stack->frame->flags & ANNA_ACTIVATION_FRAME_BREAK ? anna_from_int(1) : null_entry);
	// Clear the break flag on check
	stack->frame->flags  = stack->frame->flags & ~ANNA_ACTIVATION_FRAME_BREAK;
	stack->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(stack);	
    }
    

  ANNA_LAB_NATIVE_CALL:
    {
	OP_ENTER(stack);	
	anna_op_native_call_t *cb = (anna_op_native_call_t *)stack->frame->code;
	stack->frame->code += sizeof(*cb);

	cb->function(stack);
	OP_LEAVE(stack);
    }

  ANNA_LAB_STOP:
    {
	OP_ENTER(stack);	
//		wprintf(L"Pop last frame\n");
	anna_object_t *val = anna_context_peek_object(stack, 0);
//	free(stack->frame->code);
	anna_context_frame_return(stack);
//	stack = stack->caller;
	anna_slab_free(stack, stack->size);
	return val;
    }
	    
  ANNA_LAB_VAR_GET:
    {
	OP_ENTER(stack);	
	anna_op_var_t *op = (anna_op_var_t *)stack->frame->code;
	int i;
	anna_activation_frame_t *s = stack->frame;
	for(i=0; i<op->frame_count; i++)
	{
	    s = s->static_frame;
#ifdef ANNA_CHECK_VM
	    if(!s)
	    {
		wprintf(
		    L"Error: Var get op of to invalid stack frame: %d %d %ls\n",
		    op->frame_count, op->offset,
		    stack->frame->function->name);
		anna_context_print_parent(stack);
		anna_stack_describe(stack);
		CRASH;
	    }
#endif
	}
#ifdef ANNA_CHECK_VM
	if(!s->slot[op->offset])
	{
	    wprintf(
		L"Var get op on unassigned var: %d %d\n",
		op->frame_count, op->offset);
		    
	    CRASH;
	}
#endif
/*
	wprintf(
	    L"Var get in function %ls, pos %d %d. Result: Object of type %ls\n",
	    stack->frame->function->name,
	    op->frame_count, op->offset,
	    anna_as_obj(s->base[op->offset])->type->name);
*/
	anna_context_push_entry(stack, s->slot[op->offset]);
	stack->frame->code += sizeof(*op);
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_VAR_SET:
    {
	OP_ENTER(stack);	
	anna_op_var_t *op = (anna_op_var_t *)stack->frame->code;
	int i;
	anna_activation_frame_t *s = stack->frame;
	for(i=0; i<op->frame_count; i++)
	    s = s->static_frame;
	s->slot[op->offset] = anna_context_peek_entry(stack, 0);
/*
	wprintf(
	    L"Var set in function %ls, pos %d %d. Value: Object of type %ls\n",
	    stack->frame->function->name,
	    op->frame_count, op->offset,
	    anna_as_obj(s->base[op->offset])->type->name);
*/
	stack->frame->code += sizeof(*op);
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_MEMBER_GET:
    {
	OP_ENTER(stack);	
	anna_op_member_t *op = (anna_op_member_t *)stack->frame->code;
	anna_object_t *obj = anna_context_pop_object(stack);
	anna_member_t *m = obj->type->mid_identifier[op->mid];

#ifdef ANNA_CHECK_VM
	if(!m)
	{
	    debug(
		D_CRITICAL,
		L"Error in function %ls, offset %d: Object of type %ls does not have a member of type %ls\n",
		stack->frame->function->name,
		stack->frame->code - stack->frame->function->code,
		obj->type->name,
		anna_mid_get_reverse(op->mid));
	    anna_context_print_parent(stack);
	    
	    CRASH;
	}
#endif 
	
	if(anna_member_is_property(m))
	{
	    if(unlikely(obj == null_object))
	    {
		anna_context_push_object(stack, null_object);	    
		stack->frame->code += sizeof(*op);		    
	    }
	    else
	    {
		anna_object_t *method = anna_as_obj_fast(obj->type->static_member[m->getter_offset]);
		anna_function_t *fun = anna_function_unwrap(method);
		
		anna_context_push_object(stack, method);
		anna_context_push_object(stack, obj);
		stack->frame->code += sizeof(*op);
		stack->function_object = method;
		fun->native(stack);
	    }
	}
	else
	{
	    if(unlikely(obj == null_object))
	    {
		anna_context_push_object(stack, null_object);
	    }
	    else
	    {
		anna_entry_t *res = obj->member[m->offset];		    
		anna_context_push_entry(stack, res);
	    }
	    stack->frame->code += sizeof(*op);
	}
	
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_STATIC_MEMBER_GET:
    {
	OP_ENTER(stack);	
	anna_op_member_t *op = (anna_op_member_t *)stack->frame->code;
	anna_object_t *obj = anna_context_pop_object(stack);
	
	anna_type_t *type = anna_type_unwrap(obj);
	anna_member_t *m = type->mid_identifier[op->mid];

	//wprintf(L"Static member get of %ls\n", anna_mid_get_reverse(op->mid));
	
#ifdef ANNA_CHECK_VM
	if(!m)
	{
	    debug(
		D_CRITICAL,
		L"Object of type %ls does not have a member of type %ls\n",
		type->name,
		anna_mid_get_reverse(op->mid));    
	    CRASH;
	}
#endif 
	anna_context_push_entry(stack, type->static_member[m->offset]);
	stack->frame->code += sizeof(*op);
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_PROPERTY_GET:
    {
	OP_ENTER(stack);	
	anna_op_member_t *op = (anna_op_member_t *)stack->frame->code;
	anna_object_t *obj = anna_context_pop_object(stack);
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
	    anna_context_push_object(stack, null_object);	    
	    stack->frame->code += sizeof(*op);		    
	}
	else
	{
	    anna_object_t *method = anna_as_obj_fast(obj->type->static_member[m->getter_offset]);
	    anna_function_t *fun = anna_function_unwrap(method);
	    
	    anna_context_push_object(stack, method);
	    anna_context_push_object(stack, obj);
	    stack->frame->code += sizeof(*op);		    
	    stack->function_object = method;
	    fun->native(stack);
	}
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_STATIC_PROPERTY_GET:
    {
	OP_ENTER(stack);	
	anna_op_member_t *op = (anna_op_member_t *)stack->frame->code;
	anna_object_t *obj = anna_context_pop_object(stack);

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
	
	anna_context_push_object(stack, method);
	anna_context_push_object(stack, obj);
	stack->frame->code += sizeof(*op);
	stack->function_object = method;
	fun->native(stack);

	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_MEMBER_GET_THIS:
    {
	OP_ENTER(stack);

	anna_op_member_t *op = (anna_op_member_t *)stack->frame->code;
//	wprintf(L"Get method member %d, %ls\n", op->mid, anna_mid_get_reverse(op->mid));
	anna_object_t *obj = anna_context_pop_object(stack);
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
		D_CRITICAL,L"Object of type %ls does not have a member named %ls\n",
		obj->type->name, anna_mid_get_reverse(op->mid));
	    if(stack->frame->function)
	    {
		
		debug(
		    D_CRITICAL,
		    L"Function %ls\n",
		    stack->frame->function->name);	
	    }
	    anna_frame_print(stack->frame);	    
	    CRASH;
	}
#endif 
	anna_entry_t *res = obj->type->static_member[m->offset];
	
	anna_context_push_entry(stack, res);
	anna_context_push_object(stack, obj);
		
	stack->frame->code += sizeof(*op);

	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_MEMBER_SET:
    {
	OP_ENTER(stack);	
	anna_op_member_t *op = (anna_op_member_t *)stack->frame->code;
	anna_object_t *obj = anna_context_pop_object(stack);
	anna_entry_t *value = anna_context_peek_entry(stack, 0);
	
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

	if(anna_member_is_property(m))
	{
	    anna_object_t *method = anna_as_obj_fast(obj->type->static_member[m->setter_offset]);
	    anna_function_t *fun = anna_function_unwrap(method);
	    
	    anna_context_pop_object(stack);
	    anna_context_push_object(stack, method);
	    anna_context_push_object(stack, obj);
	    anna_context_push_entry(stack, value);

	    stack->frame->code += sizeof(*op);
	    stack->function_object = method;
	    fun->native(stack);
	}
	else
	{
	    if(anna_member_is_static(m)) {
		obj->type->static_member[m->offset] = value;
	    } else {
		obj->member[m->offset] = value;
	    }
		    
	    stack->frame->code += sizeof(*op);
	}
	OP_LEAVE(stack);	
    }

  ANNA_LAB_STATIC_MEMBER_SET:
    {
	OP_ENTER(stack);	
	anna_op_member_t *op = (anna_op_member_t *)stack->frame->code;
	anna_object_t *obj = anna_context_pop_object(stack);
	anna_entry_t *value = anna_context_peek_entry(stack, 0);
	anna_type_t * type = anna_type_unwrap(obj);
	anna_member_t *m = type->mid_identifier[op->mid];

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

	if(anna_member_is_property(m))
	{
	    debug(
		D_CRITICAL,
		L"Set property through bad call error\n");
	    CRASH;
	}

#endif
	
	type->static_member[m->offset] = value;
	stack->frame->code += sizeof(*op);
	OP_LEAVE(stack);	
    }

  ANNA_LAB_LIST:
    {
	OP_ENTER(stack);	
	anna_op_type_t *op = (anna_op_type_t *)stack->frame->code;
	anna_context_push_object(stack, anna_list_create2(op->value));
	stack->frame->code += sizeof(*op);
	OP_LEAVE(stack);
    }

  ANNA_LAB_TYPE_OF:
    {
	OP_ENTER(stack);	
	anna_op_null_t *op = (anna_op_null_t *)stack->frame->code;
	anna_context_push_object(
	    stack,
	    anna_type_wrap(
		anna_context_pop_object(stack)->type)
	    );
	stack->frame->code += sizeof(*op);
/*	    int i;
	    wprintf(L"Type of stack content:\n");
	    for(i=0; i < 3 && &stack->base[i] < stack->top; i++)
	    {
		anna_entry_t *e = anna_context_peek_entry(stack, i);
		wprintf(L"Object of type %ls\n", anna_is_obj(e)?(anna_context_peek_object(stack, i)->type->name): L"???");
		
	    }
	    wprintf(L"\n");
*/
	OP_LEAVE(stack);	
    }

  ANNA_LAB_FOLD:
    {
	OP_ENTER(stack);	
	anna_entry_t *val = anna_context_pop_entry(stack);
#ifdef ANNA_CHECK_VM
	anna_object_t *lst = anna_context_peek_object(stack, 0);
	if(lst->type->mid_identifier[ANNA_MID_LIST_PAYLOAD] == 0)
	{
	    if(stack->frame->function)
	    {
		debug(D_CRITICAL, L"In function %ls\n", stack->frame->function->name );
		anna_bc_print(stack->frame->function->code);
		debug(D_CRITICAL, L"Offset %d\n", stack->frame->code - stack->frame->function->code);
	    }
	    
	    debug(
		D_CRITICAL,
		L"Tried to fold value into something that is not a list.\n");
	    debug(
		D_CRITICAL,
		L"Non-list:\n");
	    
	    int i;
	    
	    for(i=0; &stack->stack[i] < stack->top; i++)
	    {
		wprintf(L"Object of type %ls\n", anna_context_peek_object(stack, i)->type->name);		
	    }
	    
/*
	    anna_object_print(lst);
	    
	    if(anna_is_obj(val))
	    {
		debug(
		    D_CRITICAL,
 		    L"Value:\n");
		anna_object_print(anna_as_obj(val));
	    }
*/	    
	    CRASH;
	}
#endif	
	anna_list_add(anna_context_peek_object(stack, 0), val);
	stack->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_POP:
    {
	OP_ENTER(stack);	
	anna_context_pop_entry(stack);
	stack->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_NOT:
    {
	OP_ENTER(stack);	
	*(stack->top-1) = anna_entry_null(*(stack->top-1))?anna_from_int(1):null_entry;
	stack->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(stack);	
    }

  ANNA_LAB_DUP:
    {
	OP_ENTER(stack);	
	anna_context_push_entry(stack, anna_context_peek_entry(stack, 0));
	stack->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(stack);	
    }

  ANNA_LAB_JMP:
    {
	OP_ENTER(stack);	
	anna_op_off_t *op = (anna_op_off_t *)stack->frame->code;
	stack->frame->code += op->offset;
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_COND_JMP:
    {
	OP_ENTER(stack);	
	anna_op_off_t *op = (anna_op_off_t *)stack->frame->code;
	stack->frame->code += !anna_entry_null(anna_context_pop_entry(stack)) ? op->offset:sizeof(*op);
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_NCOND_JMP:
    {
	OP_ENTER(stack);	
	anna_op_off_t *op = (anna_op_off_t *)stack->frame->code;
	stack->frame->code += anna_entry_null(anna_context_pop_entry(stack)) ? op->offset:sizeof(*op);
	OP_LEAVE(stack);	
    }
	    
  ANNA_LAB_TRAMPOLENE:
    {
	OP_ENTER(stack);	
	stack->frame = anna_frame_to_heap(stack->frame);
	anna_object_t *base = anna_context_pop_object_fast(stack);
	anna_object_t *tramp = anna_vm_trampoline(anna_function_unwrap(base), stack);
	anna_context_push_object(stack, tramp);
	
	stack->frame->code += sizeof(anna_op_null_t);
	OP_LEAVE(stack);	
    }

  ANNA_LAB_NEXT_ASSIGN_INT:
    {
	OP_ENTER(stack);	
	anna_entry_t *iv = anna_context_pop_entry(stack);
	stack->frame->code += sizeof(anna_op_null_t);
	if(likely(anna_is_int_small(iv)))
	{
	    int res = anna_as_int(iv)+1;

            if(likely(abs(res)<=ANNA_INT_FAST_MAX))
  	        anna_context_push_int(stack, (long)res);
            else
	    {
  	        anna_context_push_object(stack, anna_int_create(res));
            }
	}
	else
	{
	    anna_object_t *o1 = anna_as_obj(iv);
	    
	    if(o1 == null_object)
	    {
		anna_context_push_object(stack, null_object);		
	    }
	    else
	    {
  //          wprintf(L\"Fallback for int $name \n\");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_NEXT_ASSIGN_INT];
		anna_object_t *wrapped = anna_as_obj_fast(o1->type->static_member[m->offset]);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_context_push_object(stack,wrapped);
		anna_context_push_object(stack,o1);
		stack->function_object = wrapped;
		fun->native(stack);
	    }
	}
	
	OP_LEAVE(stack);	
    }

  ANNA_LAB_PREV_ASSIGN_INT:
    {
	OP_ENTER(stack);	
	anna_entry_t *iv = anna_context_pop_entry(stack);
	stack->frame->code += sizeof(anna_op_null_t);
	if(likely(anna_is_int_small(iv)))
	{
	    int res = anna_as_int(iv)-1;

            if(likely(abs(res)<=ANNA_INT_FAST_MAX))
  	        anna_context_push_int(stack, (long)res);
            else
	    {
  	        anna_context_push_object(stack, anna_int_create(res));
            }
	}
	else
	{
	    anna_object_t *o1 = anna_as_obj(iv);
	    
	    if(o1 == null_object)
	    {
		anna_context_push_object(stack, null_object);		
	    }
	    else
	    {
  //          wprintf(L\"Fallback for int $name \n\");
		anna_member_t *m = o1->type->mid_identifier[ANNA_MID_PREV_ASSIGN_INT];
		anna_object_t *wrapped = anna_as_obj_fast(o1->type->static_member[m->offset]);
		anna_function_t *fun = anna_function_unwrap(wrapped);
		anna_context_push_object(stack,wrapped);
		anna_context_push_object(stack,o1);
		stack->function_object = wrapped;
		fun->native(stack);
	    }
	}	
	
	OP_LEAVE(stack);	
    }

#include "autogen/vm_short_circut.c"

}

size_t anna_bc_op_size(char instruction)
{
    if(anna_instr_is_short_circut(instruction))
    {
	return sizeof(anna_op_null_t);	    
    }
    
    switch(instruction)
    {
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
	case ANNA_INSTR_TYPE_OF:
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
	case ANNA_INSTR_STATIC_MEMBER_SET:
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
		    break;
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
	    
		case ANNA_INSTR_TYPE_OF:
		{
		    wprintf(L"Type of object\n\n");
		    break;
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

		case ANNA_INSTR_STATIC_MEMBER_SET:
		{
		    anna_op_member_t *op = (anna_op_member_t *)code;
		    wprintf(L"Set static member %ls\n\n", anna_mid_get_reverse(op->mid));
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
	    
		case ANNA_INSTR_CHECK_BREAK:
		{
		    wprintf(L"Check break flag to detect loop termination\n\n");
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

		case ANNA_INSTR_STOP:
		{
		    return;
		}
	    
		case ANNA_INSTR_RETURN:
		case ANNA_INSTR_RETURN_COUNT:
		case ANNA_INSTR_RETURN_COUNT_BREAK:
		case ANNA_INSTR_TRAMPOLENE:
		case ANNA_INSTR_NATIVE_CALL:
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
		case ANNA_INSTR_STATIC_MEMBER_SET:
		case ANNA_INSTR_POP:
		case ANNA_INSTR_NOT:
		case ANNA_INSTR_DUP:
		case ANNA_INSTR_JMP:
		case ANNA_INSTR_COND_JMP:
		case ANNA_INSTR_NCOND_JMP:
		case ANNA_INSTR_CHECK_BREAK:
		case ANNA_INSTR_TYPE_OF:
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
void anna_vm_null_function(anna_context_t *stack)
{
    char *code = stack->frame->code;
    /* We rewind the code pointr one function call to get to the
     * instruction that called us. If the previous instruction was a
     * member access, this function call is in fact a getter/setter so
     * we pop the correct number of arguments for that. Otherwise, we
     * check how many parameters we were called with, and drop that
     * number of parameters+1 (The extra pop is the function
     * itself). Regardless of how we where called, we finally push a
     * null value to the stack */
    code -= sizeof(anna_op_count_t);
    anna_op_count_t *op = (anna_op_count_t *)code;
    switch(op->instruction)
    {
	case ANNA_INSTR_CALL:
	    anna_context_drop(stack,op->param+1);	
	    break;
	    
	case ANNA_INSTR_MEMBER_SET:
	    anna_context_drop(stack,3);
	    break;
	    
	case ANNA_INSTR_MEMBER_GET:
	    anna_context_drop(stack,2);
	    break;
	    
	default:
	  wprintf(L"Unknown null fun opcode %d at byte offset %d (started rollback at offset %d)\n", op->instruction, code - stack->frame->function->code, stack->frame->code-stack->frame->function->code);
	    CRASH;
    }
    anna_context_push_object(stack, null_object);
}

anna_object_t *anna_as_obj(anna_entry_t *entry)
{
    ANNA_ENTRY_JMP_TABLE;
    long type = ((long)entry) & ANNA_STACK_ENTRY_FILTER;
    goto *jmp_table[type];
  LAB_ENTRY_OBJ:
    return (anna_object_t *)entry;
  LAB_ENTRY_CHAR:
    {
	long res = (long)entry;
	res >>= 2;
	return anna_char_create((int)res);
    }
  LAB_ENTRY_FLOAT:
    {
	double *res = (double *)((long)entry & ~ANNA_STACK_ENTRY_FILTER);
	return anna_float_create(*res);
    }
  LAB_ENTRY_INT:
    {
	long res = (long)entry;
	res >>= 2;
	return anna_int_create(res);
    }
  LAB_ENTRY_BLOB:
    CRASH;
}

